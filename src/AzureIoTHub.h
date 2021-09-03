#ifndef __AZURE_IOT_HUB_H__
#define __AZURE_IOT_HUB_H__

// Force use AIS 4G Dev Kit
#ifndef AIS_4G_DEV_KIT
#define AIS_4G_DEV_KIT
#endif

#include "Arduino.h"
#include "ArduinoJson-v6.18.3.h"
#include "PubSubClient.h"
#include "url_encode_decode.h"
#include "mbedtls/base64.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"
#include "SIM76xx.h"
#include "GSMClientSecure.h"

#define JSON_DOC_BUFFER_SIZE 1024
#define TOKEN_LIFESPAN       60 // In min
#define MQTT_PACKET_SIZE     1024

typedef std::function<void(String payload)> CommandHandlerFunction;

class AzureIoTHub {
    protected:
        PubSubClient *mqtt;

        String host = "<>.azure-devices.net";
        String deviceId = "";
        String symmetricKey = ""; // Shared Access Signatures (SAS) : Primary key OR Secondary key
        
        // Option
        String modelId = "";

        uint32_t getTime();
        String generateSasToken(String resourceUri) ;

    public:
#if defined(ESP32) || defined(ESP8266)
        AzureIoTHub();
#endif
        AzureIoTHub(Client &c);

        bool configs(String host, String deviceId, String symmetricKey) ;
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