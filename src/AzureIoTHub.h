#ifndef __AZURE_IOT_HUB_H__
#define __AZURE_IOT_HUB_H__

#include "Arduino.h"
#include "ArduinoJson-v6.18.3.h"
#include "PubSubClient.h"
#include "url_encode_decode.h"
#include "mbedtls/base64.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"
#include "SIM76xx.h"
#include "GSMClientSecure.h"
#include "GSMUdp.h"

#define JSON_DOC_BUFFER_SIZE 1024
#define TOKEN_LIFESPAN       60 // In min
#define MQTT_PACKET_SIZE     1024

typedef std::function<void(String payload)> CommandHandlerFunction;
typedef std::function<time_t()> GetTimeHandlerFunction;

class AzureIoTHub {
    protected:
        Client *client = NULL;
        PubSubClient *mqtt = NULL;

        String host = "<>.azure-devices.net";
        String deviceId = "";
        String symmetricKey = ""; // Shared Access Signatures (SAS) : Primary key OR Secondary key
        
        // Option
        String modelId = "";

        GetTimeHandlerFunction getTime = NULL;
        String generateSasToken(String resourceUri) ;

    public:
        AzureIoTHub();
        AzureIoTHub(Client &c, GetTimeHandlerFunction get_time_fn);
        ~AzureIoTHub();
        
        bool configs(String host, String deviceId, String symmetricKey) ;
        void setGetTime(GetTimeHandlerFunction fn) ;
        bool connect();
        bool isConnected();

        void setTelemetryValue(String name, int value) ;
        void setTelemetryValue(String name, float value) ;
        void setTelemetryValue(String name, double value) ;
        void setTelemetryValue(String name, bool value) ;
        void setTelemetryValue(String name, const String& value) ;
        void setTelemetryValue(String name, JsonVariant value) ;
        bool sendMessage() ;

        void addCommandHandle(String command, CommandHandlerFunction callback) ;

        void loop() ;

};

#endif