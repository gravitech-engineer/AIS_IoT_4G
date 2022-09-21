/*
library Dev version: v2.5.3
Author:(POC Device Magellan team)      
Create Date: 25 April 2022. 
Modified: 15 september 2022.
*/
#ifndef ATTRIBUTE_CORE_H
#define ATTRIBUTE_CORE_H
#include <Arduino.h>
#include <Client.h>
#include "../PubSubClient.h"
#include "../ArduinoJson-v6.18.3.h"
#define useGSMClient  0
#define useExternalClient 1

class Attribute_MQTT_core
{
public:
    static boolean triggerRemainSub;
    static boolean ctrl_regis_key;
    static boolean ctrl_regis_pta;
    static boolean ctrl_regis_json;
    static boolean conf_regis_key;
    static boolean conf_regis_pta;
    static boolean conf_regis_json;
    static boolean resp_regis;
    static boolean ctrl_jsonOBJ;
    static boolean conf_jsonOBJ;
    static boolean useAdvanceCallback;
    static String ext_Token;
    static String ext_EndPoint;
    static int clientNetInterface;
    static Client *ClientNET;
    static PubSubClient *mqtt_client; //MQTT Client
    static String sensorJSON_str;
    static String clientConfigJSON_str;
    static boolean useBuiltInSensor;
    static StaticJsonDocument<512> docClientConf;
    static DynamicJsonDocument *adjDoc;
    static DynamicJsonDocument *docSensor;
};
extern Attribute_MQTT_core attr;
#endif
