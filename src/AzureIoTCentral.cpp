#include "AzureIoTCentral.h"

void mqttSubscribeCallbackDPS(char* topic, byte* payload, unsigned int length) ;

#if defined(ESP32) || defined(ESP8266)
AzureIoTCentral::AzureIoTCentral() : AzureIoTHub() {
    AzureIoTHub::modelId = "";
}
#endif

AzureIoTCentral::AzureIoTCentral(Client &c) : AzureIoTHub(c) {
    AzureIoTHub::modelId = "";
}

bool AzureIoTCentral::configs(String idScopt, String deviceId, String symmetricKey) {
    this->idScopt = idScopt;
    this->deviceId = deviceId;
    AzureIoTHub::symmetricKey = symmetricKey;

    return true;
}

int dpsRegistrationsState;
int dpsRegistrationsRetryAfter;
String dpsRegistrationsOperationId;
String dpsRegistrationsAssignedHub;
String dpsRegistrationsDeviceId;

bool AzureIoTCentral::connect() {
    if (!dpsGotInfo) {
        String resourceUri = this->idScopt + "/registrations/" + this->deviceId;

        String mqttUsername = "", mqttPassword = "";

        // MQTT Username
        mqttUsername = resourceUri + "/api-version=2019-03-31";

        // MQTT Password
        mqttPassword = AzureIoTHub::generateSasToken(resourceUri);

        Serial.println("MQTT Connection (DPS)");
        Serial.println(" MQTT Host: global.azure-devices-provisioning.net");
        Serial.println(" MQTT ClientId: " + this->deviceId);
        Serial.println(" MQTT Username: " + mqttUsername);
        Serial.println(" MQTT Password: " + mqttPassword);

        AzureIoTHub::mqtt->setServer("global.azure-devices-provisioning.net", 8883);
        AzureIoTHub::mqtt->setCallback(mqttSubscribeCallbackDPS);
        Serial.print("(DPS) MQTT Connect... ");
        if (!AzureIoTHub::mqtt->connect(this->deviceId.c_str(), mqttUsername.c_str(), mqttPassword.c_str())) {
            Serial.println("Fail!");
            return false;
        }
        Serial.println("Connected");

        // Ref: https://docs.microsoft.com/th-th/azure/iot-dps/iot-dps-mqtt-support
        AzureIoTHub::mqtt->subscribe("$dps/registrations/res/#");
        AzureIoTHub::mqtt->publish("$dps/registrations/PUT/iotdps-register/?$rid=1", String("{payload:{\"modelId\":\"" + AzureIoTHub::modelId + "\"}}").c_str());

        /*
            Flow: (<-- = out, --> = in)
                <-- $dps/registrations/PUT/iotdps-register/?$rid={request_id}
                --> $dps/registrations/res/202/?$rid={request_id}&retry-after=x
                [WAIT x]
                <-- $dps/registrations/GET/iotdps-get-operationstatus/?$rid={request_id}&operationId={operationId}
                --> $dps/registrations/res/200/?$rid={request_id}
        */

        dpsRegistrationsState = 0;
        while(1) {
            AzureIoTHub::mqtt->loop();
            if (dpsRegistrationsState == 0) { // Wait responses 202
                delay(1);
            } else if (dpsRegistrationsState == 1) { // Receive responses 202
                delay(dpsRegistrationsRetryAfter * 1000); // Wait retry
                AzureIoTHub::mqtt->publish(String("$dps/registrations/GET/iotdps-get-operationstatus/?$rid=1&operationId=" + dpsRegistrationsOperationId).c_str(), "");
                dpsRegistrationsState = 2;
            } else if (dpsRegistrationsState == 2) { // Wait responses 200
                delay(1);
            } else if (dpsRegistrationsState == 3) { // Receive responses 200
                break;
            }
        }

        AzureIoTHub::mqtt->disconnect();

        Serial.println("Device provisioned:");
        Serial.println(" Hub host = " + dpsRegistrationsAssignedHub);
        Serial.println(" Device id = " + dpsRegistrationsDeviceId);

        AzureIoTHub::host = dpsRegistrationsAssignedHub;
        AzureIoTHub::deviceId = dpsRegistrationsDeviceId;

        dpsGotInfo = true;
    }

    return AzureIoTHub::connect();
}

void mqttSubscribeCallbackDPS(char* topic, byte* payload, unsigned int length) {
    Serial.println("mqttSubscribeCallbackDPS");
    Serial.println("  Topic: " + String(topic));
    Serial.print("  Payload: ");
    Serial.write(payload, length);
    Serial.println();

    if (String(topic).indexOf("$dps/registrations/res/") >= 0) {
        Serial.println("Found topic");
        int responsesCode = 0;
        sscanf(topic, "$dps/registrations/res/%d/", &responsesCode);
        Serial.println("Code: " + String(responsesCode));
        if (responsesCode == 202) { // 202 Accepted
            dpsRegistrationsRetryAfter = -1;
            sscanf(topic, "$dps/registrations/res/202/?$rid=%*d&retry-after=%d", &dpsRegistrationsRetryAfter);
            if (dpsRegistrationsRetryAfter >= 0) {
                Serial.println("Set state 1");
                dpsRegistrationsState = 1;
            } else {
                Serial.println("Error retry-after not found");
            }
        } else if (responsesCode == 200) {
            Serial.println("Set state 3");
            dpsRegistrationsState = 3;
        } else {
            Serial.println("Error unknow responses code");
        }
        
        // JSON parser payload
        char payload_str[length + 1];
        memset(payload_str, 0, sizeof(payload_str));
        memcpy(payload_str, payload, length);

        StaticJsonDocument<4 * 1024> doc;
        DeserializationError error = deserializeJson(doc, payload_str);
        if (!error) {
            dpsRegistrationsOperationId = String(doc["operationId"].as<const char*>());
            Serial.println("dpsRegistrationsOperationId: " + dpsRegistrationsOperationId);
            if (!doc["registrationState"]["assignedHub"].isNull()) {
                dpsRegistrationsAssignedHub = String(doc["registrationState"]["assignedHub"].as<const char*>());
            }
            if (!doc["registrationState"]["deviceId"].isNull()) {
                dpsRegistrationsDeviceId = String(doc["registrationState"]["deviceId"].as<const char*>());
            }
        } else {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
        }
    } else {
        Serial.println("Error unknow topic");
    }
}

