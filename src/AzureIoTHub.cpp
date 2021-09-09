#include "AzureIoTHub.h"
#include "Azure_CA.h"
#include "GSM_LOG.h"

// Ref: https://github.com/MicrosoftDocs/azure-docs/blob/master/articles/iot-hub/iot-hub-mqtt-support.md

StaticJsonDocument<JSON_DOC_BUFFER_SIZE> messageJsonDoc;

void mqttSubscribeCallbackHub(char* topic, byte* payload, unsigned int length) ;

typedef struct {
    String command;
    CommandHandlerFunction callback;
    void *next;
} CommandInfo;

typedef struct {
    int rid;
    int status;
} CommandReplay;

CommandInfo *_startCommand_p = NULL;
QueueHandle_t commandReplyQueue = NULL;

uint32_t last_get_time = 0;
uint32_t last_epoch = 0;
time_t gsmGetTime() {
    if ((last_epoch != 0) && ((millis() - last_get_time) < 60000)) { // less then 60s of last update
        return last_epoch + ((millis() - last_get_time) / 1000);
    }

    GSMUdp Udp;
    Udp.begin(2390);

    uint8_t packetBuffer[48];
    memset(packetBuffer, 0, sizeof(packetBuffer));
    packetBuffer[0] = 0b11100011;
    packetBuffer[1] = 0;
    packetBuffer[2] = 6;
    packetBuffer[3] = 0xEC;

    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;

    for (int i=0;i<5;i++) {
        Udp.beginPacket("th.pool.ntp.org", 123);
        Udp.write(packetBuffer, sizeof(packetBuffer));
        if (Udp.endPacket() == 0) {
            delay(50);
            continue;
        }

        // Wait server reply
        bool found_reply = false;
        for (int i=0;i<10;i++) {
            if (Udp.parsePacket()) {
                found_reply = true;
                break;
            }
            delay(100);
        }
        if (found_reply) {
            break;
        }
    }
    Udp.read(packetBuffer, 48);

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    const unsigned long seventyYears = 2208988800UL;
    last_epoch = secsSince1900 - seventyYears;
    last_get_time = millis();

    return last_epoch;
}

AzureIoTHub::AzureIoTHub() {
    GSMClientSecure *client = new GSMClientSecure;
    client->setCACert(AZURE_ROOT_CA);

    this->client = client;
    this->getTime = gsmGetTime;

    // MQTT lib setup
    this->mqtt = new PubSubClient(*client);
    this->mqtt->setBufferSize(MQTT_PACKET_SIZE);

    // Queue setup
    if (!commandReplyQueue) {
        commandReplyQueue = xQueueCreate(10, sizeof(CommandReplay*));
    }
}

AzureIoTHub::AzureIoTHub(Client &c, GetTimeHandlerFunction get_time_fn) {
    this->client = &c;
    this->getTime = get_time_fn;

    // MQTT lib setup
    this->mqtt = new PubSubClient(c);
    this->mqtt->setBufferSize(MQTT_PACKET_SIZE);

    // Queue setup
    if (!commandReplyQueue) {
        commandReplyQueue = xQueueCreate(10, sizeof(CommandReplay*));
    }
}

bool AzureIoTHub::configs(String host, String deviceId, String symmetricKey) {
    this->host = host;
    this->deviceId = deviceId;
    this->symmetricKey = symmetricKey;

    return true;
}

void AzureIoTHub::setGetTime(GetTimeHandlerFunction fn) {
    this->getTime = fn;
}

// Ref: https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-dev-guide-sas?tabs=node
String AzureIoTHub::generateSasToken(String resourceUri) {
    String signingKey = this->symmetricKey;
    String policyName = "";
    uint32_t expiresInMins = TOKEN_LIFESPAN;

    resourceUri = urlEncode(resourceUri);

    if (!this->getTime) {
        return String();
    }

    // Set expiration in seconds
    uint32_t expires = this->getTime() + (expiresInMins * 60);
    String toSign = resourceUri + "\n" + String(expires);

    // Use crypto
    /*
    var hmac = crypto.createHmac('sha256', Buffer.from(signingKey, 'base64'));
    hmac.update(toSign);
    var base64UriEncoded = encodeURIComponent(hmac.digest('base64'));
    */
    String signature = "";
    { 
        // Base64 decode
        unsigned char *symmetricKeyDecodeBuffer = (unsigned char*)malloc(this->symmetricKey.length() + 1);
        uint32_t symmetricKeyDecodeLength = 0;
        mbedtls_base64_decode(
            symmetricKeyDecodeBuffer, 
            this->symmetricKey.length() + 1, 
            &symmetricKeyDecodeLength, 
            (const unsigned char*)this->symmetricKey.c_str(), 
            this->symmetricKey.length()
        );

        // SHA-256
        uint8_t *encryptedSignature = (uint8_t*)malloc(32);
        mbedtls_md_context_t ctx;
        const mbedtls_md_type_t mdType{ MBEDTLS_MD_SHA256 };
        mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(mdType), 1);
        mbedtls_md_hmac_starts(&ctx, (const unsigned char*)symmetricKeyDecodeBuffer, symmetricKeyDecodeLength);
        mbedtls_md_hmac_update(&ctx, (const unsigned char*)toSign.c_str(), toSign.length());
        mbedtls_md_hmac_finish(&ctx, encryptedSignature);

        // Base64 encode
        unsigned char *signatureEncodeBuffer = (unsigned char*)malloc((32 * 1.5f) + 1);
        uint32_t signatureEncodeLength = 0;
        mbedtls_base64_encode(
            signatureEncodeBuffer, 
            (32 * 1.5f) + 1,
            &signatureEncodeLength,
            encryptedSignature,
            32
        );

        signature = "";
        for (int i=0;i<signatureEncodeLength;i++) {
            signature += (char)signatureEncodeBuffer[i];
        }
        signature = urlEncode(signature);

        free(symmetricKeyDecodeBuffer);
        free(encryptedSignature);
        free(signatureEncodeBuffer);
    }

    // Construct authorization string
    String token = "SharedAccessSignature sr=" + resourceUri + "&sig=" + signature + "&se=" + expires;
    if (policyName.length() > 0) token += "&skn=" + policyName;
    return token;
};

bool AzureIoTHub::connect() {
    /*
    MQTT client id = 2bamrk3mvn
    MQTT username = iotc-240dc81d-aa36-4b2f-bd10-0080fbe73471.azure-devices.net/2bamrk3mvn/?api-version=2020-09-30&DeviceClientType=c%2F1.1.0&model-id=dtmi%3Aseeedkk%3Awioterminal%3Awioterminal_aziot_example%3B5
    MQTT password = SharedAccessSignature sr=iotc-240dc81d-aa36-4b2f-bd10-0080fbe73471.azure-devices.net%2Fdevices%2F2bamrk3mvn&sig=ZYKQ93PxFjWmlx8iHX9SBL0c%2BDRwaAlsqZfjw3HasF4%3D&se=1628368610
    */

    String mqttUsername = "", mqttPassword = "";

    // MQTT Username
    mqttUsername = this->host + "/" + this->deviceId + "/?api-version=2020-09-30&DeviceClientType=c%2F1.1.0";
    if (this->modelId.length() > 0) {
        mqttUsername += "&model-id=" + urlEncode(this->modelId);
    }

    // MQTT Password
    mqttPassword = this->generateSasToken(this->host + "/devices/" + this->deviceId);

    GSM_LOG_I("MQTT Connection");
    GSM_LOG_I(" MQTT Host: %s", this->host.c_str());
    GSM_LOG_I(" MQTT ClientId: %s", this->deviceId.c_str());
    GSM_LOG_I(" MQTT Username: %s", mqttUsername.c_str());
    GSM_LOG_I(" MQTT Password: %s", mqttPassword.c_str());
    
    this->mqtt->setServer(this->host.c_str(), 8883);
    this->mqtt->setCallback(mqttSubscribeCallbackHub);
    GSM_LOG_I("MQTT Connect... ");
    if (!this->mqtt->connect(this->deviceId.c_str(), mqttUsername.c_str(), mqttPassword.c_str())) {
        GSM_LOG_E("MQTT Connect fail");
        return false;
    }
    GSM_LOG_I("Connected");

    this->mqtt->subscribe("$iothub/methods/POST/#");
    this->mqtt->subscribe("devices/+/messages/devicebound/#");

    return true;
}

bool AzureIoTHub::isConnected() {
    return this->mqtt->connected();
}

void AzureIoTHub::setTelemetryValue(String name, int value) {
  messageJsonDoc.getOrAddMember(name).set(value);
}

void AzureIoTHub::setTelemetryValue(String name, float value) {
  messageJsonDoc.getOrAddMember(name).set(value);
}

void AzureIoTHub::setTelemetryValue(String name, double value) {
  messageJsonDoc.getOrAddMember(name).set(value);
}

void AzureIoTHub::setTelemetryValue(String name, bool value) {
  messageJsonDoc.getOrAddMember(name).set(value);
}

void AzureIoTHub::setTelemetryValue(String name, const String& value) {
  messageJsonDoc.getOrAddMember(name).set(value);
}

void AzureIoTHub::setTelemetryValue(String name, JsonVariant value) {
    messageJsonDoc.getOrAddMember(name).set(value);
}

bool AzureIoTHub::sendMessage() {
    String payload = "";
    serializeJson(messageJsonDoc, payload);
    GSM_LOG_I("Publish %s", payload.c_str());
    return this->mqtt->publish(
        String("devices/" + this->deviceId + "/messages/events/").c_str(), 
        payload.c_str()
    );
}

void AzureIoTHub::addCommandHandle(String command, CommandHandlerFunction callback) {
    CommandInfo *newInfo = new CommandInfo;
    newInfo->command = command;
    newInfo->callback = callback;
    newInfo->next = NULL;

    if (_startCommand_p != NULL) {
        CommandInfo *focusInfo = _startCommand_p;
        while (focusInfo->next != NULL) {
            focusInfo = (CommandInfo *)focusInfo->next;
        }
        focusInfo->next = newInfo;
    } else {
        _startCommand_p = newInfo;
    }
}

void AzureIoTHub::loop() {
    this->mqtt->loop();

    CommandReplay *reply;
    while(xQueueReceive(commandReplyQueue, &reply, 0) == pdPASS) {
        if (reply) {
            GSM_LOG_I("RID: %d, Replay: %d", reply->rid, reply->status);
            this->mqtt->publish(
                (const char*) String("$iothub/methods/res/" + String(reply->status) + "/?$rid=" + String(reply->rid)).c_str(), 
                "{}",
                false
            );
            free(reply);
        }
    }
}

AzureIoTHub::~AzureIoTHub() {
    if (this->mqtt) {
        delete this->mqtt;
    }

    if (this->client) {
        delete this->client;
    }
}

void mqttSubscribeCallbackHub(char* topic, byte* payload, unsigned int length) {
    GSM_LOG_I("Topic: %s", topic);
    GSM_LOG_I("Payload: %.*s", length, payload);

    /*
      Topic:$iothub/methods/POST/light/?$rid=1
      Payload: 14568
    */

    if (String(topic).indexOf("$iothub/methods/POST/") >= 0) {
        char command_buff[50];
        memset(command_buff, 0, sizeof(command_buff));
        int rid = -1;
        sscanf(topic, "$iothub/methods/POST/%[^/]/?$rid=%d", command_buff, &rid);
        String gotCommand = String(command_buff);
        GSM_LOG_I("Command: %s, rid: %d", command_buff, rid);
        CommandInfo *focusInfo = _startCommand_p;
        while (focusInfo != NULL) {
            if (focusInfo->command == gotCommand) {
                break;
            } else {
                focusInfo = (CommandInfo *)focusInfo->next;
            }
        }
        
        if (focusInfo != NULL) {
            if (focusInfo->command == gotCommand) {
                String payload_str = "";
                for (int i=0;i<length;i++) {
                    payload_str += (char)payload[i];
                }
                focusInfo->callback(payload_str);
            }
        }

        if (rid > 0) {
            CommandReplay *reply = (CommandReplay*)malloc(sizeof(CommandReplay));
            reply->rid = rid;
            reply->status = focusInfo != NULL ? 200 : 404;
            xQueueSend(commandReplyQueue, &reply, 0);
        }
    }
}
