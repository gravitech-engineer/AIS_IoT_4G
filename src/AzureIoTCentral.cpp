#include "AzureIoTCentral.h"
#include "GSM_LOG.h"

void mqttSubscribeCallbackDPS(char* topic, byte* payload, unsigned int length) ;

AzureIoTCentral::AzureIoTCentral() : AzureIoTHub() {
    AzureIoTHub::modelId = "";
}

AzureIoTCentral::AzureIoTCentral(Client &c, GetTimeHandlerFunction get_time_fn) : AzureIoTHub(c, get_time_fn) {
    AzureIoTHub::modelId = "";
}

bool AzureIoTCentral::configs(String idScopt, String deviceId, String symmetricKey, String modelId) {
    this->idScopt = idScopt;
    this->deviceId = deviceId;
    AzureIoTHub::symmetricKey = symmetricKey;
    AzureIoTHub::modelId = modelId;

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

        GSM_LOG_I("MQTT Connection (DPS)");
        GSM_LOG_I(" MQTT Host: global.azure-devices-provisioning.net");
        GSM_LOG_I(" MQTT ClientId: %s", this->deviceId.c_str());
        GSM_LOG_I(" MQTT Username: %s", mqttUsername.c_str());
        GSM_LOG_I(" MQTT Password: %s", mqttPassword.c_str());

        AzureIoTHub::mqtt->setServer("global.azure-devices-provisioning.net", 8883);
        AzureIoTHub::mqtt->setCallback(mqttSubscribeCallbackDPS);
        GSM_LOG_I("(DPS) MQTT Connect... ");
        if (!AzureIoTHub::mqtt->connect(this->deviceId.c_str(), mqttUsername.c_str(), mqttPassword.c_str())) {
            GSM_LOG_E("(DPS) MQTT Connect Fail!");
            return false;
        }
        GSM_LOG_I("Connected");

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
            if (!AzureIoTHub::mqtt->connected()) {
                GSM_LOG_E("(DPS) MQTT disconnect !!!");
                return false;
            }
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

        GSM_LOG_I("Device provisioned:");
        GSM_LOG_I(" Hub host = %s", dpsRegistrationsAssignedHub.c_str());
        GSM_LOG_I(" Device id = %s", dpsRegistrationsDeviceId.c_str());

        AzureIoTHub::host = dpsRegistrationsAssignedHub;
        AzureIoTHub::deviceId = dpsRegistrationsDeviceId;

        dpsGotInfo = true;
    }

    
    bool connect = AzureIoTHub::connect();
    if (!connect) {
        dpsGotInfo = false;
    }

    return connect;
}

void mqttSubscribeCallbackDPS(char* topic, byte* payload, unsigned int length) {
    GSM_LOG_I("mqttSubscribeCallbackDPS");
    GSM_LOG_I("  Topic: %s", topic);
    GSM_LOG_I("  Payload: %.*s", length, payload);

    if (String(topic).indexOf("$dps/registrations/res/") >= 0) {
        GSM_LOG_I("Found topic");
        int responsesCode = 0;
        sscanf(topic, "$dps/registrations/res/%d/", &responsesCode);
        GSM_LOG_I("Code: %d", responsesCode);
        if (responsesCode == 202) { // 202 Accepted
            dpsRegistrationsRetryAfter = -1;
            sscanf(topic, "$dps/registrations/res/202/?$rid=%*d&retry-after=%d", &dpsRegistrationsRetryAfter);
            if (dpsRegistrationsRetryAfter >= 0) {
                GSM_LOG_I("Set state 1");
                dpsRegistrationsState = 1;
            } else {
                GSM_LOG_I("Error retry-after not found");
            }
        } else if (responsesCode == 200) {
            GSM_LOG_I("Set state 3");
            dpsRegistrationsState = 3;
        } else {
            GSM_LOG_I("Error unknow responses code");
        }
        
        // JSON parser payload
        char payload_str[length + 1];
        memset(payload_str, 0, sizeof(payload_str));
        memcpy(payload_str, payload, length);

        StaticJsonDocument<4 * 1024> doc;
        DeserializationError error = deserializeJson(doc, payload_str);
        if (!error) {
            dpsRegistrationsOperationId = String(doc["operationId"].as<const char*>());
            GSM_LOG_I("dpsRegistrationsOperationId: %s", dpsRegistrationsOperationId.c_str());
            if (!doc["registrationState"]["assignedHub"].isNull()) {
                dpsRegistrationsAssignedHub = String(doc["registrationState"]["assignedHub"].as<const char*>());
            }
            if (!doc["registrationState"]["deviceId"].isNull()) {
                dpsRegistrationsDeviceId = String(doc["registrationState"]["deviceId"].as<const char*>());
            }
        } else {
            GSM_LOG_E("deserializeJson() failed: %s", String(error.f_str()).c_str());
        }
    } else {
        GSM_LOG_E("Error unknow topic");
    }
}

