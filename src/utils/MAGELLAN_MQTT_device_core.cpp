/*
Copyright (c) 2020, Advanced Wireless Network
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
Magellan_4GBoard v2.5.3 AIS 4G Board.
support SIMCOM SIM7600E(AIS 4G Board)
 
Author:Worawit Sayned (POC Device Magellan team)      
Create Date: 25 April 2022. 
Modified: 1 september 2022.
*/

#include "MAGELLAN_MQTT_device_core.h"
StaticJsonDocument<512> intern_docJSON;
 int Attribute_MQTT_core::clientNetInterface;
 Client *Attribute_MQTT_core::ClientNET = NULL;
 PubSubClient *Attribute_MQTT_core::mqtt_client = NULL; //MQTT Client
 boolean Attribute_MQTT_core::ctrl_regis_key = false;
 boolean Attribute_MQTT_core::ctrl_regis_pta = false;
 boolean Attribute_MQTT_core::ctrl_regis_json = false;
 boolean Attribute_MQTT_core::conf_regis_key = false;
 boolean Attribute_MQTT_core::conf_regis_pta = false;
 boolean Attribute_MQTT_core::conf_regis_json = false;
 boolean Attribute_MQTT_core::resp_regis = false;
 boolean Attribute_MQTT_core::ctrl_jsonOBJ = false;
 boolean Attribute_MQTT_core::conf_jsonOBJ = false;
 boolean Attribute_MQTT_core::useAdvanceCallback = false;
 String Attribute_MQTT_core::ext_Token;
 String Attribute_MQTT_core::ext_EndPoint;
 boolean Attribute_MQTT_core::useBuiltInSensor = false;
 boolean Attribute_MQTT_core::triggerRemainSub = true;

 StaticJsonDocument<512> Attribute_MQTT_core::docClientConf;
 DynamicJsonDocument *Attribute_MQTT_core::adjDoc = new DynamicJsonDocument(256);
 DynamicJsonDocument *Attribute_MQTT_core::docSensor = new DynamicJsonDocument(1024);


String b2str(byte* payload, unsigned int length) // convert byte* to String
{
  char buffer_payload[length+1] = {0};
  memcpy(buffer_payload, (char*)payload, length);
  return String(buffer_payload);
}

typedef struct 
{
  String registerKey; 
  ctrl_handleCallback ctrl_key_callback;
  ctrl_Json_handleCallback ctrl_Json_callback;
  ctrl_PTAhandleCallback ctrl_pta_callback;
  ctrl_JsonOBJ_handleCallback ctrl_obj_callback;

  conf_handleCallback conf_key_callback;
  conf_Json_handleCallback conf_json_callback;
  conf_PTAhandleCallback conf_pta_callback;
  conf_JsonOBJ_handleCallback conf_obj_callback;
  resp_callback resp_h_callback;
  
  void *next;
  unsigned int Event;
  unsigned int RESP_Events;

} regisAPI;

regisAPI *_startRegis = NULL; //buffer callback <void String payload>
regisAPI *_startRegisPTA = NULL; //buffer callback <void String key, String value>
regisAPI *_startRegisJSON = NULL; //buffer callback <void String key, String value>
regisAPI *_startRegisConf = NULL; //buffer callback <void String payload>
regisAPI *_startRegisPTAConf = NULL; //buffer callback <void String key, String value>
regisAPI *_startRegisJSONConf = NULL; //buffer callback <void String key, String value>
regisAPI *_startRESP = NULL;

regisAPI *_startOBJ_CTRL = NULL;
regisAPI *_startOBJ_CONF = NULL;

boolean ext_useAdvanceCallback = false;


Centric centric;

void (*cb_internal)(EVENTS events, char*);

JsonObject deJson(String jsonContent)
{
  JsonObject buffer;
  intern_docJSON.clear();
  if(jsonContent != NULL && jsonContent != "clear")
  {
    DeserializationError error = deserializeJson(intern_docJSON, jsonContent);
    buffer = intern_docJSON.as<JsonObject>();
    if(error)
      Serial.println("# Error to DeserializeJson Control");
  }
  return buffer;
}

String deControl(String jsonContent)
{
  String content = "40300";
  JsonObject buffdoc = deJson(jsonContent);
  String statusCode = buffdoc["Code"];
  String buffDelta ;
  if( statusCode == "20000")
  {
    if(jsonContent.indexOf("Delta") != -1)
    {
      buffDelta = buffdoc["Delta"].as<String>();
      content = buffDelta;
    }
    else if(jsonContent.indexOf("Sensor") != -1)
    {
      buffDelta = buffdoc["Sensor"].as<String>();
      content = buffDelta;
    }
  }
  return content;
}

String deConfig(String jsonContent)
{
  String content = "40300";
  JsonObject buffdoc = deJson(jsonContent);
  String statusCode = buffdoc["Code"];
  String buffDelta ;
  if( statusCode == "20000")
  {
    if(jsonContent.indexOf("Config") != -1)
    {
      buffDelta = buffdoc["Config"].as<String>();
      content = buffDelta;
    }
  }
  return content;
}

void msgCallback_internalHandler(char * topic, byte* payload, unsigned int length)
{
  String action = "ERROR";
  unsigned int buffEvent = ERROR;
  String b_topic = String(topic);
  String _payload = b2str(payload, length);
  String key = "null"; //if this topic is'nt PLAINTEXT 
  String code = "0";
  
  EVENTS intern_EVENT;
  intern_EVENT.RESP = "EMPTY";

  regisAPI *handleRegisPTA = _startRegisPTA;
  regisAPI *handleRegisJSON = _startRegisJSON;
  regisAPI *handleRegisJSON_CTRL_OBJ = _startOBJ_CTRL;
  regisAPI *handleRegisKEY = _startRegis;

  regisAPI *handleRegisPTAConf = _startRegisPTAConf;
  regisAPI *handleRegisJSONConf = _startRegisJSONConf;
  regisAPI *handleRegisJSON_CONF_OBJ = _startOBJ_CONF;
  regisAPI *handleRegisKEYConf = _startRegisConf;

  regisAPI *handleRESP = _startRESP;

  char * b_payload = (char *)_payload.c_str(); //payload for advance_cb and endpoint centric

  if(b_topic.indexOf("/auth/resp/") != -1)
  {
    buffEvent = TOKEN; 
    action = "TOKEN";
    attr.ext_Token = *&_payload;
  }
  if(b_topic.indexOf("/delta/resp/pta") != -1)
  {
    int indexfound2 = String(b_topic).indexOf("=");
    String keyOnTopic = b_topic.substring(indexfound2+1);
    key = keyOnTopic;
    buffEvent = CONTROL_PLAINTEXT; 
    action = "CONTROL_PLAINTEXT";

    if((_payload == "40300" || (_payload =="40400") && (_payload.length() == 5)))
    {
      code = _payload;
      intern_EVENT.RESP ="FAIL";
    }
    else{
      code = "20000";
      intern_EVENT.RESP ="SUCCESS";
    }

    if(attr.ctrl_regis_key)
    {
      while (handleRegisKEY != NULL)
      {
        if(handleRegisKEY->registerKey == key){
          break;
        }
        else{
          handleRegisKEY = (regisAPI *)handleRegisKEY->next;
        }
      }
      if(handleRegisKEY != NULL)
      {
        if(handleRegisKEY->registerKey == key)
        {
          handleRegisKEY->ctrl_key_callback(_payload);
        }
      }
    }
    if(attr.ctrl_regis_pta)
    {
      while (handleRegisPTA != NULL)
      {
        if(handleRegisPTA->Event == buffEvent){
          break;
        }
        else{
          handleRegisPTA = (regisAPI *)handleRegisPTA->next;
        }
      }
      if(handleRegisPTA != NULL)
      {
        handleRegisPTA->ctrl_pta_callback(key, _payload);
      }
    }
  }

  else if(b_topic.indexOf("/delta/resp") != -1)
  { 
    buffEvent = CONTROL_JSON;
    action = "CONTROL_JSON";

    if(_payload.indexOf("20000") != -1)
    {   
      code = _payload.substring(_payload.indexOf("\":\"")+3, _payload.indexOf("\":\"")+8);
      intern_EVENT.RESP ="SUCCESS";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }
    else
    {
      code = _payload.substring(_payload.indexOf("\":\"")+3, _payload.length()+8);
      intern_EVENT.RESP ="FAIL";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }  

    if(attr.ctrl_regis_json)
    {
      while (handleRegisJSON != NULL)
      {
        if(handleRegisJSON->Event == buffEvent){
          break;
        }
        else{
          handleRegisJSON = (regisAPI *)handleRegisJSON->next;
        }
      }
      if(handleRegisJSON != NULL)
      {
        handleRegisJSON->ctrl_Json_callback(_payload);
      }
    }

    if(attr.ctrl_jsonOBJ)
    {
      while (handleRegisJSON_CTRL_OBJ != NULL)
      {
        if(handleRegisJSON_CTRL_OBJ->Event == buffEvent){
          break;
        }
        else{
          handleRegisJSON_CTRL_OBJ = (regisAPI *)handleRegisJSON_CTRL_OBJ->next;
        }
      }
      if(handleRegisJSON_CTRL_OBJ != NULL)
      {
        // Serial.println(_payload);
        String buffDocs = deControl(_payload);
        // Serial.println(buffDocs);
        JsonObject Docs = deJson(buffDocs);
        handleRegisJSON_CTRL_OBJ->ctrl_obj_callback(Docs);
      }
    }
  }   

  if(b_topic.indexOf("/config/resp/pta/?config=") != -1)
  {
    int indexfound2 = String(b_topic).indexOf("=");
    String keyOnTopic = b_topic.substring(indexfound2+1);
    key = keyOnTopic;
    buffEvent = CONFIG_PLAINTEXT;
    action = "CONFIG_PLAINTEXT";

    if((_payload == "40300" || (_payload =="40400") && (_payload.length() == 5)))
    {
      code = _payload;
      intern_EVENT.RESP ="FAIL";
    }
    else{
      code = "20000";
      intern_EVENT.RESP ="SUCCESS";
    }
    
    if(attr.conf_regis_key)
    {
      while (handleRegisKEYConf != NULL)
      {
        if(handleRegisKEYConf->registerKey == key){
          break;
        }
        else{
          handleRegisKEYConf = (regisAPI *)handleRegisKEYConf->next;
        }
      }
      if(handleRegisKEYConf != NULL)
      {
        if(handleRegisKEYConf->registerKey == key)
        {
          handleRegisKEYConf->conf_key_callback(_payload);
        }
      }
    }

    if(attr.conf_regis_pta)
    {
      while (handleRegisJSON != NULL)
      {
        if(handleRegisPTAConf->Event == buffEvent){
          break;
        }
        else{
          handleRegisPTAConf = (regisAPI *)handleRegisPTAConf->next;
        }
      }
      if(handleRegisPTAConf != NULL)
      {
        handleRegisPTAConf->conf_pta_callback(key, _payload);
      }
    }

  }
  else if(b_topic.indexOf("/config/resp") != -1)
  {
    buffEvent = CONFIG_JSON; 
    action = "CONFIG_JSON";
    
    if(_payload.indexOf("20000") != -1)
    {   
      code = _payload.substring(_payload.indexOf("\":\"")+3, _payload.indexOf("\":\"")+8);
      intern_EVENT.RESP ="SUCCESS";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }
    else
    {
      code = _payload.substring(_payload.indexOf("\":\"")+3, _payload.length()+8);
      intern_EVENT.RESP ="FAIL";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }

    if(attr.conf_regis_json)
    {
      while (handleRegisJSONConf != NULL)
      {
        if(handleRegisJSONConf->Event == buffEvent){
          break;
        }
        else{
          handleRegisJSONConf = (regisAPI *)handleRegisJSONConf->next;
        }
      }
      if(handleRegisJSONConf != NULL)
      {
        handleRegisJSONConf->conf_json_callback(_payload);
      }
    }

    if(attr.conf_jsonOBJ)
    {
      while (handleRegisJSON_CONF_OBJ != NULL)
      {
        if(handleRegisJSON_CONF_OBJ->Event == buffEvent){
          break;
        }
        else{
          handleRegisJSON_CONF_OBJ = (regisAPI *)handleRegisJSON_CONF_OBJ->next;
        }
      }
      if(handleRegisJSON_CONF_OBJ != NULL)
      {
        String buffDocs = deConfig(_payload);
        JsonObject Docs = deJson(buffDocs);
        handleRegisJSON_CONF_OBJ->conf_obj_callback(Docs);
      }
    }
  } 

  if(b_topic.indexOf("dateTime") != -1)
  {
    buffEvent = UNIXTIME; 
    action = "UNIXTIME";
  }
  if(b_topic.indexOf("/report/resp/pta/?") != -1)
  {
    buffEvent = RESP_REPORT_PLAINTEXT; 
    action = "RESP_REPORT_PLAINTEXT";
    int indexfound2 = String(b_topic).indexOf("=");
    String keyOnTopic = b_topic.substring(indexfound2+1);
    key = keyOnTopic;
    if(_payload.indexOf("20000") != -1)
    {
      code = _payload;
      intern_EVENT.RESP ="SUCCESS";
    }
    else{
      code = _payload;
      intern_EVENT.RESP ="FAIL";
    }    
  }
  else if (b_topic.indexOf("/report/resp") != -1)
  {
    buffEvent = RESP_REPORT_JSON; 
    action = "RESP_REPORT_JSON";

    if(_payload.indexOf("20000") != -1)
    {   
      code = _payload.substring(_payload.indexOf("\":\"")+3, _payload.length()+8);
      intern_EVENT.RESP ="SUCCESS";
     }
    else
    {
      code = _payload.substring(_payload.indexOf("\":\"")+3, _payload.length()+8);
      intern_EVENT.RESP ="FAIL";
    }
  }

  if(b_topic.indexOf("/heartbeat/resp/pta") != -1)
  {
    buffEvent = RESP_HEARTBEAT_PLAINTEXT;
    action = "RESP_HEARTBEAT_PLAINTEXT";

    if(_payload.indexOf("20000") != -1)
    {
      code = _payload;
      intern_EVENT.RESP ="SUCCESS";
    }
    else{
      code = _payload;
      intern_EVENT.RESP ="FAIL";
    }

  
  }
  else if(b_topic.indexOf("/heartbeat/resp") != -1)
  {
    buffEvent = RESP_HEARTBEAT_JSON; 
    action = "RESP_HEARTBEAT_JSON";

    if(_payload.indexOf("20000") != -1)
    {   
      code = _payload.substring(_payload.indexOf("\":\"")+3, _payload.length()-2);
      intern_EVENT.RESP ="SUCCESS";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }
    else
    {
      code = _payload.substring(_payload.indexOf("\":\"")+3, _payload.length()-2);
      intern_EVENT.RESP ="FAIL";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }
  }
  if(b_topic.indexOf("/report/timestamp/resp") != -1)
  {
    buffEvent = RESP_REPORT_TIMESTAMP;
    action = "RESP_REPORT_TIMESTAMP";
    if(_payload.indexOf("20000") != -1)
    {   
      code = _payload.substring(_payload.indexOf("\":\"")+3, _payload.length()-2);
      intern_EVENT.RESP ="SUCCESS";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }
    else
    {
      code = _payload.substring(_payload.indexOf("\":\"")+3, _payload.length()-2);
      intern_EVENT.RESP ="FAIL";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }
  }  
  if(b_topic.indexOf("/server/destination/response") != -1)
  {
    buffEvent = GET_ENDPOINT;
    action = "GET_ENDPOINT";
    attr.ext_EndPoint = b_payload;
  }
  // Serial.println("#INSIDE ");
  // Serial.println(action);
  // Serial.println(_payload);
  // Serial.println("#INSIDE ");

  intern_EVENT.CODE = code.toInt();
  intern_EVENT.Topic = b_topic;
  intern_EVENT.Payload = _payload;
  intern_EVENT.PayloadLength = length;
  intern_EVENT.Action = action;
  intern_EVENT.Key = key;

  if(ext_useAdvanceCallback)  //routing to onMessage Callback when use setMessageListener(Callback)
  {
    cb_internal(intern_EVENT, b_payload);
  }

  if(attr.resp_regis)
    {
      while (handleRESP != NULL)
      {
        if(handleRESP->Event == buffEvent){
          break;
        }
        else{
          handleRESP = (regisAPI *)handleRESP->next;
        }
      }
      // Serial.println("# IN RESP :"+ String(handleRESP->Event));
      // Serial.println("# IN RESP2 :"+ String(buffEvent));
      if(handleRESP != NULL)
      {
        if(handleRESP->Event == buffEvent)
        {
          handleRESP->resp_h_callback(intern_EVENT);
        }
      }
    }
  // Serial.println("#DEBUG INSIDE :"+intern_EVENT.Action);
  // Serial.println("#DEBUG INSIDE :"+intern_EVENT.Payload);
}

MAGELLAN_MQTT_device_core::MAGELLAN_MQTT_device_core(Client& client)
{
  prev_time = millis();
  now_time  = millis();
  HB_prev_time = millis();
  HB_now_time = millis();

  attr.clientNetInterface = useExternalClient;
  Client *newClient = &client;
  attr.ClientNET = *&newClient;
  attr.mqtt_client = new PubSubClient(*attr.ClientNET);
  this->client = *&attr.mqtt_client;
}

MAGELLAN_MQTT_device_core::MAGELLAN_MQTT_device_core()
{
  prev_time = millis();
  now_time  = millis();
  HB_prev_time = millis();
  HB_now_time = millis();

  attr.clientNetInterface = useGSMClient;
  this->gsm_client  = new GSMClient;
  attr.ClientNET= *&gsm_client;
  attr.mqtt_client = new PubSubClient(*attr.ClientNET);
  this->client = *&attr.mqtt_client;
}

String MAGELLAN_MQTT_device_core::getHostName()
{
  return this->host;
}

void MAGELLAN_MQTT_device_core::setMQTTBufferSize(uint16_t sizeBuffer)
{
  Serial.println(F("# SetBufferSize: "));
  Serial.println(sizeBuffer);
  this->_default_bufferSize = sizeBuffer;
}

void MAGELLAN_MQTT_device_core::setAuthMagellan(String _thingIden, String _thingSecret, String _imei)
{
  Serial.println(F("#====== Setting Magellan Authentication ======="));
  if(!(CheckString_isDigit(_thingIden) && CheckString_isDigit(_thingSecret)))
  {
    Serial.print(F("# ERROR Can't connect to Magellan"));
    Serial.print(F("# Parameter from your setting is invalid \n [thingIdentify]=> "));
    Serial.print(_thingIden);
    Serial.print(F("   [thingSecret]=> "));
    Serial.println(_thingSecret);
    Serial.println(F("# Invalid Parameter!! Please check [thingIdentify] and [thingSecret]"));
    while (true)
    {
      Serial.print(".");
      delay(300);
      this->cnt_fail++;
      if(cnt_fail >= 100) //timeout Restart board 30 sec
      {
        ESP.restart();
      }
    }
  }
  this->thingIden = _thingIden;
  this->thingSecret = _thingSecret;
  this->imei = _imei;
  // Serial.println(F("#Set Auth Success"));
  // Serial.println(thingIden);
  // Serial.println(thingSecret);
  // Serial.println(imei);
}

void getRadio()
{
  if(attr.clientNetInterface == useGSMClient)
  {
    Serial.println(F("#========= Radio Quality information =========="));
    Serial.println("Signal Strength: "+String(Network.getSignalStrength()));
    Serial.println(F("#=============================================="));
  }
}

void MAGELLAN_MQTT_device_core::initialBoard()
{
  delay(1000);
  setAuthMagellan(GSM.getICCID(), GSM.getIMSI(), GSM.getIMEI());
  Serial.println(F("#====== Initializing Board ======="));
  Serial.println("ICCID: "+String(thingIden));
  Serial.println("IMSI: "+String(thingSecret));
  Serial.println("IMEI: "+String(imei));
  Serial.println(F("#================================="));
}

void MAGELLAN_MQTT_device_core::getBoardInfo()
{
  Serial.println(F("#====== Board information ========="));
  Serial.println("ICCID: "+String(thingIden));
  Serial.println("IMSI: "+String(thingSecret));
  Serial.println("IMEI: "+String(imei));
  getRadio();
  Serial.println(F("#================================="));
}

String MAGELLAN_MQTT_device_core::getICCID()
{
  return this->thingIden;
}

String MAGELLAN_MQTT_device_core::getIMEI()
{
  return this->imei;
}

String MAGELLAN_MQTT_device_core::getIMSI()
{
  return this->thingSecret;
}

String MAGELLAN_MQTT_device_core::readToken()
{
  return this->token;
}

boolean MAGELLAN_MQTT_device_core::CheckString_isDigit(String valid_payload)
{
  for(byte i = 0; i < valid_payload.length(); i++)
  {
    if(!isDigit(valid_payload.charAt(i))) return false;
  }
  return true;
}

boolean MAGELLAN_MQTT_device_core::CheckString_isDouble(String valid_payload)
{
  char* input = (char*)valid_payload.c_str();
  char* end;
  strtod(input, &end);
  if(* input == '\0')
  {
    return false;
  }
  if(end == input || *end != '\0')
  {
    return false;
  }
  return true;
}

void MAGELLAN_MQTT_device_core::setCallback_msgHandle()
{
  this->client->setCallback(msgCallback_internalHandler);
}

void MAGELLAN_MQTT_device_core::setMessageListener(void(* callback)(EVENTS, char*))
{  
  attr.useAdvanceCallback = true;
  if(callback)
      cb_internal = callback;
}

void MAGELLAN_MQTT_device_core::reconnect()
{
  while (!isConnected())
  {
    Serial.print(F("Device Disconected from Magellan..."));
      checkConnection();
      if(flagToken)
      {   
        Serial.print(F("# Remain Subscribes list\n"));
        attr.triggerRemainSub = true;
      }
  }
}

boolean MAGELLAN_MQTT_device_core::acceptEndPoint(String payload)
{
  boolean acceptStatus = false;
  if(payload.length() >= 10)
  {
    const char* buff_payload = payload.c_str();
    JsonObject getCetric = deJson(buff_payload);

    String buf1 = getCetric["ServerDestinationInfo"];
    String buf2 = getCetric["OperationStatus"];
    // Serial.println("buf1 :"+ buf1);
    if(buf2.indexOf("20000") != -1)
    {
      JsonObject getCetric2 = deJson(buf1);
      String buff_ip = getCetric2["ServerIP"];
      String buff_domain = getCetric2["ServerDomain"];
      String buff_port = getCetric2["ServerPort"];
      centric.endPoint_DOMAIN = buff_domain;
      centric.endPoint_IP = buff_ip;
      centric.endPoint_PORT = buff_port;
      acceptStatus = true;
      Serial.println(F("## NEW ZONE AVAILABLE #######"));
      Serial.println("# Centric IP >>: " + centric.endPoint_IP); 
      Serial.println("# Centric Domain >>: " + centric.endPoint_DOMAIN); 
      Serial.println("# Centric Port >>: " + String(centric.endPoint_PORT)); 
      Serial.println(F("#############################"));
      cnt_attempt = 0;
    }
    else
    {
      Serial.println(F("# Fail to Get Endpoint form centric"));
      Serial.println(F("# Please check the thing device is activated"));
      Serial.print(F("# response: "));
      Serial.println(payload);
    }       
  }
  return acceptStatus;
}


void MAGELLAN_MQTT_device_core::acceptToken(String payload)
{
    if(payload.length() >= 36)
    {
      this->flagToken = true;
      this->token = payload;
      Serial.println("# Thingtoken: " + token);      
    }
}

void MAGELLAN_MQTT_device_core::acceptToken(EVENTS event)
{
  String _payload = event.Payload;
  if((event.Topic == "api/v2/thing/" + String(thingIden) + "/" + String(thingSecret) + "/auth/resp/pta") && !flagToken)
  {
    if(_payload.length() >= 36)
    {
      this->flagToken = true;
      this->token = _payload;
      Serial.println("# Token >> :" + token);      
    }
  }
}

// String MAGELLAN_MQTT_device_core::byteToString(byte* payload, unsigned int length_payload)
// {
//   char buffer_payload[length_payload+1] = {0};
//   memcpy(buffer_payload, (char*)payload, length_payload);
//   return String(buffer_payload);
// }

String MAGELLAN_MQTT_device_core::byteToString(byte* payload, unsigned int length_payload)
{
  return b2str(payload, length_payload);
}


void MAGELLAN_MQTT_device_core::loop()
{
  this->client->loop();
  reconnect();
}

void MAGELLAN_MQTT_device_core::reconnectMagellan()
{
  while (!isConnected())
  {
    srand( time(NULL) );
    int randnum = rand() % 10000; //generate number concat in Client id
    int randnum_2 = rand() % 10000; //generate number concat in Client id
    String client_idBuff = client_id +"_"+String(randnum)+"_"+String(randnum_2);
    Serial.println(F("Attempting MQTT connection ..."));
    this->client->setServer(this->host.c_str(), this->port);
    this->client->setCallback(msgCallback_internalHandler);
    Serial.println("Connecting Magellan on: "+ String(this->host) + ", Port: "+ String(this->port));
    if(this->client->connect(client_idBuff.c_str(), this->thingIden.c_str(), this->thingSecret.c_str()))
    {
      Serial.println("Client id: "+ client_idBuff +" is connected");
      recon_attempt = 0;
    }
     else 
    {
      Serial.print(F("failed, reconnect ="));
      Serial.print(this->client->state());
      Serial.println(F(" try again in 5 seconds"));
      if(!flagToken)
      {
        Serial.println(F("# Please check the thing device is activated "));      
      }
      delay(5000);
      recon_attempt++;
      Serial.print(F("# attempt connect on :"));
      Serial.println(String(recon_attempt)+ " times");
      if(recon_attempt >= MAXrecon_attempt)
      {
        Serial.println(" attempt to connect more than "+String(MAXrecon_attempt)+ " Restart Board");
        ESP.restart();
      }
    }
  }
  thingRegister();
}

void MAGELLAN_MQTT_device_core::checkConnection()
{
  if(!isConnected())
  {
    reconnectMagellan();
  }
}


void MAGELLAN_MQTT_device_core::magellanCentric(const char* _host, int _port)
{
  if(!isConnected())
  {
    while (!isConnected())
    {
      srand( time(NULL) );
      int randnum = rand() % 10000; //generate number concat in Client id
      int randnum_2 = rand() % 10000; //generate number concat in Client id
      String client_idBuff = client_id +"_"+String(randnum)+"_"+String(randnum_2);
      Serial.println(F("#Attempting connection ..."));
      this->client->setServer(_host, _port);
      this->client->setCallback(msgCallback_internalHandler);
      Serial.println("Connecting Magellan on: "+ String(this->host) + ", Port: "+ String(this->port));
      String thisIdenCentric = "Centric."+thingIden;
      if(this->client->connect(client_idBuff.c_str(), thisIdenCentric.c_str(), this->thingSecret.c_str()))
      {
        Serial.println("Client id : "+ client_idBuff +" is connected");
        recon_attempt = 0;
      }

      else 
      {
        Serial.print(F("failed, reconnect ="));
        Serial.print(this->client->state());
        Serial.println(F(" try again in 5 seconds"));
        Serial.print(F("Count Attemp Reconnect: "));
        recon_attempt++;
        Serial.println(recon_attempt);
        delay(5000);
        if(recon_attempt >= MAXrecon_attempt)
        {
          Serial.println(" attempt to connect more than: "+String(MAXrecon_attempt)+ " Restart Board");
          ESP.restart();
        }
      }  
    }
    getEndPoint();
  }
}

void MAGELLAN_MQTT_device_core::getEndPoint()
{
  Serial.println(F("# REQUEST ENDPOINT"));
  while (!flagRegisterEndPoint )
  {
    String topic = "api/v2/things/"+this->thingIden+"/"+this->thingSecret+"/server/destination/response";
    this->flagRegisterEndPoint = this->client->subscribe(topic.c_str());
    Serial.println("# Register destination server: "+String(flagRegisterEndPoint));
  }
  while (!flagGetEndPoint)
  {
    this->client->loop();
    // Serial.println("Pub");
    this->requestEndpoint();
    if(attr.ext_EndPoint.length() >= 10)
    {
      // Serial.println(ext_EndPoint);
      flagGetEndPoint = acceptEndPoint(attr.ext_EndPoint);
    }
    delay(5000);
  }
  if(flagGetEndPoint)
  {
    Serial.println(F("# Disconnect from Centric"));
    this->client->disconnect();
    Serial.println(F("# Connect to new zone"));
    srand( time(NULL) );
    int randnum = rand() % 10000; //generate number concat in Client id
    int randnum_2 = rand() % 10000; //generate number concat in Client id
    String client_idBuff = client_id +"_"+String(randnum)+"_"+String(randnum_2);
    this->beginCustom(client_idBuff, true, centric.endPoint_IP, (centric.endPoint_PORT).toInt(), this->_default_bufferSize);
  }
}

bool MAGELLAN_MQTT_device_core::isConnected()
{
  return this->client->connected();
}

void MAGELLAN_MQTT_device_core::beginCustom(String _client_id, boolean builtInSensor, String _host, int _port, uint16_t bufferSize)
{
  Serial.println("=================== Begin MAGELLAN Library [AIS 4G Board] "+ String(lib_version) +" ===================");
  if(attr.clientNetInterface == useGSMClient)
  {
    Serial.println("# AIS 4G Board");
    while(!GSM.begin()) 
    {
      Serial.println(F("GSM setup fail"));
      delay(2000);
    } 
    if((this->thingIden == NULL) && (this->thingSecret == NULL))
    {
      initialBoard();
      // getRadio();
    }
  }
  else if(attr.clientNetInterface == useExternalClient)
  {
    Serial.println("# External Client");
  }
  if(builtInSensor)
  {
    Serial.println(F("# Using Builtin SENSOR"));
    attr.useBuiltInSensor = builtInSensor;
    mySensor.begin();
  }
  delay(5000);
  getRadio();
  this->host = _host;
  this->port = _port;
  this->client_id = _client_id;
  if(bufferSize > _default_OverBufferSize)
  {
    Serial.print(F("# Buffer size from you set over than 8192 set buffer to: "));
    Serial.println();
    this->setBufferSize(_default_OverBufferSize);   
  }
  else
  {
    this->setBufferSize(bufferSize);
  }
  checkConnection();

}

void MAGELLAN_MQTT_device_core::beginCentric()
{
  Serial.println("=================== Begin MAGELLAN Library [AIS 4G Board] "+ String(lib_version) +" ===================");
  if(attr.clientNetInterface == useGSMClient)
  {
    Serial.println(F("# AIS 4G Board"));
    while(!GSM.begin()) 
    {
      Serial.println(F("GSM setup fail"));
      delay(2000);
    } 
    if((this->thingIden == NULL) && (this->thingSecret == NULL))
    {
      initialBoard();
      // getRadio();
    }
  }
  else if(attr.clientNetInterface == useExternalClient)
  {
    Serial.println(F("# External Client"));
  }
  Serial.println(F("# Connect to Centric"));
  delay(5000);
  getRadio();
  this->host = hostCentric;
  this->port = mgPort;
  this->client_id = this->getICCID(); //auto_assigned Client ID with ThingIdent
  setBufferSize(_default_bufferSize);
  magellanCentric();
}

void MAGELLAN_MQTT_device_core::begin(boolean builtInSensor)
{
  Serial.println("=================== Begin MAGELLAN Library [AIS 4G Board] "+ String(lib_version) +" ===================");
  if(attr.clientNetInterface == useGSMClient)
  {
    Serial.println(F("# AIS 4G Board"));
    while(!GSM.begin()) 
    {
      Serial.println(F("GSM setup fail"));
      delay(2000);
    } 
    if((this->thingIden == NULL) && (this->thingSecret == NULL))
    {
      initialBoard();
      // getRadio();
    }
  }
  else if(attr.clientNetInterface == useExternalClient)
  {
    Serial.println(F("# External Client"));
  }
  if(builtInSensor)
  {
    Serial.println(F("# Using Builtin SENSOR"));
    attr.useBuiltInSensor = builtInSensor;
    mySensor.begin();
  }
  getRadio();
  delay(5000);
  String _host = _host_production;

  this->host = _host;
  this->port = mgPort;
  this->client_id = getICCID(); //auto_assigned Client ID with ICCID
  setBufferSize(_default_bufferSize);
  checkConnection();
  
}

void MAGELLAN_MQTT_device_core::begin(String _thingIden, String _thingSencret, String _imei, unsigned int Zone, uint16_t bufferSize, boolean builtInSensor)
{
  setAuthMagellan(_thingIden, _thingSencret, _imei);
  Serial.print(F("ThingIdentify: "));
  Serial.println(_thingIden);
  Serial.print(F("ThingSecret: "));
  Serial.println(_thingSencret);
  Serial.print(F("IMEI: "));
  Serial.println(_imei);
  begin(_thingIden, builtInSensor, Zone, bufferSize);
}

void MAGELLAN_MQTT_device_core::begin(String _client_id, boolean builtInSensor, unsigned int Zone, uint16_t bufferSize)
{
  Serial.println("=================== Begin MAGELLAN Library [AIS 4G Board] "+ String(lib_version) +" ===================");
  if(attr.clientNetInterface == useGSMClient)
  {
    Serial.println(F("# AIS 4G Board"));
    while(!GSM.begin()) 
    {
      Serial.println(F("GSM setup fail"));
      delay(2000);
    } 
    if((this->thingIden == NULL) && (this->thingSecret == NULL))
    {
      initialBoard();
      // getRadio();
    }
  }
  
  else if(attr.clientNetInterface == useExternalClient)
  {
    Serial.println(F("# External Client"));
  }

  if(builtInSensor)
  {
    Serial.println(F("# Using Builtin SENSOR"));
    attr.useBuiltInSensor = builtInSensor;
    mySensor.begin();
  }
  getRadio();
  delay(5000);
  String _host = _host_production;
  switch (Zone)
  {
  case Production:
    _host = _host_production;
    break;
  default:
    _host = _host_production;
    Serial.println(F("# OUT OF LENGTH ZONE MOVE TO Production"));
    break;
  }
  this->host = _host;
  this->port = mgPort;
  this->client_id = _client_id;
  if(bufferSize > _default_OverBufferSize)
  {
    Serial.print(F("# Buffer size from you set over than 8192 set buffer to: "));
    Serial.println();
    this->setBufferSize(_default_OverBufferSize);   
  }
  else
  {
    this->setBufferSize(bufferSize);
  }
  checkConnection();
  
}

boolean MAGELLAN_MQTT_device_core::registerToken()
{
  String topic = "api/v2/thing/" + this->thingIden + "/" + this->thingSecret + "/auth/resp/pta";
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  Serial.println(F("-------------------------------"));
  Serial.println(F("# Register Token to magellan"));
  Serial.println("# Register Token Status: "+ _debug);
  return Sub_status;
}

boolean MAGELLAN_MQTT_device_core::report(String payload)
{
  
  String topic = "api/v2/thing/" + token + "/report/persist";
  boolean Pub_status = client->publish(topic.c_str(), payload.c_str());
  _debug = (Pub_status == true)? "Success" : "Failure"; 
  Serial.println(F("-------------------------------"));
  Serial.println("# Report JSON: "+ _debug);
  Serial.println("# Payload: "+ payload);
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::report(String key, String value)
{
  String topic = "api/v2/thing/"+ token +"/report/persist/pta/?sensor="+ key;
  boolean Pub_status = client->publish(topic.c_str(), value.c_str());
  _debug = (Pub_status == true)? "Success" : "Failure"; 
  Serial.println(F("-------------------------------"));
  Serial.println("# Report Plaintext: "+ _debug);
  Serial.println("# [key]: "+ key);
  Serial.println("# [value]: "+ value);
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::reportSensor()
{
  String bufferPlayload = buildSensorJSON(*attr.docSensor);
  boolean Pub_status = false;
  if(bufferPlayload.indexOf("null") == -1)
  {
    Pub_status = report(bufferPlayload);
    clearSensorBuffer(*attr.docSensor);
  }
  else
  {
    Serial.println(F("# Can't reportSensor Because Not set function \"void addSensor(key,value)\" before reportSensor"));
  }
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::ACKControl(String key, String value)
{
  String topic = "api/v2/thing/"+ token +"/report/persist/pta/?sensor="+ key;
  boolean Pub_status = this->client->publish(topic.c_str(), value.c_str());
  _debug = (Pub_status == true)? "Success" : "Failure"; 
  Serial.println(F("-------------------------------"));
  Serial.println("# ACKNOWNLEDGE Control Plaintext: "+ _debug);
  Serial.println("# [key]: "+ key);
  Serial.println("# [value]: "+ value);
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::ACKControl(String payload)
{
  String topic = "api/v2/thing/" + token + "/report/persist";
  boolean Pub_status = this->client->publish(topic.c_str(), payload.c_str());
  _debug = (Pub_status == true)? "Success" : "Failure"; 
  Serial.println(F("-------------------------------"));
  Serial.println("# ACKNOWNLEDGE Control JSON: "+ _debug);
  Serial.println("# Payload: "+ payload);
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::reportTimestamp(String timestamp, String JSONpayload, unsigned int timestamp_type)
{
  boolean Pub_status = false;
  String topic_ = "api/v2/thing/"+ token +"/report/timestamp/persist";
  if(timestamp != NULL && JSONpayload != NULL)
  {
    String payload_ =  "[{\"UNIXTS\":"+ timestamp+ ",\"Sensor\":"+ JSONpayload +"}]";
    Pub_status = this->client->publish(topic_.c_str(), payload_.c_str());
    _debug = (Pub_status == true)? "Success" : "Failure"; 
    Serial.println("# Report with timestamp: "+ _debug);
    Serial.println("# Payload: "+ payload_);
    return Pub_status;
  }
  else{
    Serial.println(F("# Report with timestamp: Failure"));
    Serial.println(F("# Error Empty timestamp or payload"));
    
  }
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::reportClientConfig(String payload)
{
  String topic = "api/v2/thing/"+ token +"/config/persist";
  boolean Pub_status = this->client->publish(topic.c_str(), payload.c_str());
  _debug = (Pub_status == true)? "Success" : "Failure"; 
  Serial.println(F("-------------------------------"));
  Serial.println("# Save ClientConfig: "+ _debug);
  Serial.println("# Payload: "+ payload);
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::requestEndpoint()
{
  boolean Pub_status = false;
  if(!flagGetEndPoint)
  {
    if(cnt_attempt >= limit_attempt)
    {
      Serial.println("Device Attempt to request ENDPOINT more than "+String(limit_attempt)+" time. restart board");
      delay(1000);
      ESP.restart();
    }
    if(millis() - previouseMillis > 10000)
    {
      previouseMillis = millis();
      String topic = "api/v2/things/" + thingIden + "/" + thingSecret + "/server/destination/request";
      Pub_status = this->client->publish(topic.c_str(), " ");
      _debug = (Pub_status == true)? "Success" : "Failure";
      Serial.print("# Request Endpoint: "+ _debug);
      if(cnt_attempt > 0)
      {
        Serial.print(" Attempt >> " + String(cnt_attempt -1) + " time");
      }
      Serial.println();
      cnt_attempt++;
    }
  }
  return Pub_status;
}


boolean MAGELLAN_MQTT_device_core::requestToken()
{
  boolean Pub_status = false;
  if(!flagToken)
  {
    if(cnt_attempt >= limit_attempt)
    {
      Serial.println("Device Attempt to request token more than "+String(limit_attempt)+" time. restart board");
      delay(1000);
      ESP.restart();
    }
    if(millis() - previouseMillis > 10000)
    {
      previouseMillis = millis();
      String topic = "api/v2/thing/" + thingIden + "/" + thingSecret + "/auth/req";
      Pub_status = this->client->publish(topic.c_str(), " ");
      _debug = (Pub_status == true)? "Success" : "Failure";
      // Serial.println("topic :" + topic);
      Serial.print("# Request Token: "+ _debug);
      if(cnt_attempt > 0)
      {
        Serial.print(" Attempt >> " + String(cnt_attempt -1) + " time");
      }
      Serial.println();
      cnt_attempt++;
    }
  }
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::setBufferSize(uint16_t size)
{ 
  Serial.println("# set BufferSize: "+String(size));
  return this->client->setBufferSize(size);
}

boolean MAGELLAN_MQTT_device_core::heartbeat()
{
  String topic = "api/v2/thing/"+ token +"/heartbeat";
  boolean Pub_status = this->client->publish(topic.c_str(), " ");
  _debug = (Pub_status == true)? "Success" : "Failure"; 
  Serial.println(F("-------------------------------"));
  Serial.println("# Heartbeat Trigger: "+ _debug);
  return Pub_status;  
}

void MAGELLAN_MQTT_device_core::heartbeat(unsigned int triger_ms)
{
  HB_threshold_ms = triger_ms;
  HB_now_time = millis();
  unsigned long different_ms = HB_now_time - HB_prev_time;
  if(different_ms >= HB_threshold_ms)
  {
    heartbeat();
    HB_prev_time = HB_now_time;
  }
}

void MAGELLAN_MQTT_device_core::setManualToken(String _token)
{
  Serial.println(F("# SET MANUAL TOKEN ====="));
  Serial.println("#Token: "+_token);
  if(_token.length() >= 36)
  {
    token = _token;
    acceptToken(token);
  }
}

boolean MAGELLAN_MQTT_device_core::reqControlJSON()
{
  
  String topic = "api/v2/thing/"+ token +"/delta/req";
  boolean Pub_status = this->client->publish(topic.c_str(), " ");
  _debug = (Pub_status == true)? "Success" : "Failure"; 
  Serial.println(F("-------------------------------"));
  Serial.println("# Request Control [Json]: "+ _debug);
  return Pub_status;  
}

boolean MAGELLAN_MQTT_device_core::reqControl(String key)
{
  String topic = "api/v2/thing/"+ token +"/delta/req/?sensor="+key;
  boolean Pub_status = this->client->publish(topic.c_str(), " ");
  _debug = (Pub_status == true)? "Success" : "Failure"; 
  Serial.println(F("-------------------------------"));
  Serial.println("# Request Control Plaintext by [Key]: \""+key+"\": "+ _debug);
  return Pub_status;  
}

boolean MAGELLAN_MQTT_device_core::reqConfigJSON()
{
  String topic = "api/v2/thing/"+ token +"/config/req";
  boolean Pub_status = this->client->publish(topic.c_str(), " ");
  _debug = (Pub_status == true)? "Success" : "Failure"; 
  Serial.println(F("-------------------------------"));
  Serial.println("# Request Config [Json]: "+ _debug);
  return Pub_status;  
}

boolean MAGELLAN_MQTT_device_core::reqConfig(String key)
{
  String topic = "api/v2/thing/"+ token +"/config/req/?config="+ key; //fact C c
  boolean Pub_status = this->client->publish(topic.c_str(), " ");
  _debug = (Pub_status == true)? "Success" : "Failure"; 
  Serial.println(F("-------------------------------"));
  Serial.println("# Request Config Plaintext [Key]: \""+ key+"\": "+ _debug);
  return Pub_status;  
}

void MAGELLAN_MQTT_device_core::getControl(String key, ctrl_handleCallback ctrl_callback)
{
  attr.ctrl_regis_key = true;
  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONTROL_PLAINTEXT;
  newRegis->registerKey = key;
  newRegis->ctrl_key_callback = ctrl_callback;
  newRegis->next = NULL;

  if (_startRegis != NULL) {
        regisAPI *focusRegis = _startRegis;
        while (focusRegis->next != NULL) {
            focusRegis = (regisAPI *)focusRegis->next;
        }
        focusRegis->next = newRegis;
    } else {
        _startRegis = newRegis;
    }
}

void MAGELLAN_MQTT_device_core::getControl(ctrl_PTAhandleCallback ctrl_pta_callback)
{
  attr.ctrl_regis_pta = true;

  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONTROL_PLAINTEXT;
  newRegis->ctrl_pta_callback = ctrl_pta_callback;
  newRegis->next = NULL;

  if (_startRegisPTA != NULL) {
        regisAPI *focusRegis = _startRegisPTA;
        while (focusRegis->next != NULL) {
            focusRegis = (regisAPI *)focusRegis->next;
        }
        focusRegis->next = newRegis;
    } else {
        _startRegisPTA = newRegis;
    }
}


void MAGELLAN_MQTT_device_core::getControlJSON(ctrl_Json_handleCallback ctrl_json_callback)
{
  attr.ctrl_regis_json = true;

  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONTROL_JSON;
  newRegis->ctrl_Json_callback = ctrl_json_callback;
  newRegis->next = NULL;

  if (_startRegisJSON != NULL) {
        regisAPI *focusRegis = _startRegisJSON;
        while (focusRegis->next != NULL) {
            focusRegis = (regisAPI *)focusRegis->next;
        }
        focusRegis->next = newRegis;
    } else {
        _startRegisJSON = newRegis;
    }
}


void MAGELLAN_MQTT_device_core::getControlJSON(ctrl_JsonOBJ_handleCallback jsonOBJ_cb)
{
  attr.ctrl_jsonOBJ = true;
  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONTROL_JSON;
  newRegis->ctrl_obj_callback = jsonOBJ_cb;
  newRegis->next = NULL;

  if (_startOBJ_CTRL != NULL) {
        regisAPI *focusRegis = _startOBJ_CTRL;
        while (focusRegis->next != NULL) {
            focusRegis = (regisAPI *)focusRegis->next;
        }
        focusRegis->next = newRegis;
    } else {
        _startOBJ_CTRL = newRegis;
    }
}

void MAGELLAN_MQTT_device_core::getConfig(String key, conf_handleCallback _conf_callback)
{
  attr.conf_regis_key = true;

  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONFIG_PLAINTEXT;
  newRegis->registerKey = key;
  newRegis->conf_key_callback = _conf_callback;
  newRegis->next = NULL;

  if (_startRegisConf != NULL) {
        regisAPI *focusRegis = _startRegisConf;
        while (focusRegis->next != NULL) {
            focusRegis = (regisAPI *)focusRegis->next;
        }
        focusRegis->next = newRegis;
    } else {
        _startRegisConf = newRegis;
    }
}

void MAGELLAN_MQTT_device_core::getConfig(conf_PTAhandleCallback conf_pta_callback)
{
  attr.conf_regis_pta = true;
  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONFIG_PLAINTEXT;
  newRegis->conf_pta_callback = conf_pta_callback;
  newRegis->next = NULL;

  if (_startRegisPTAConf != NULL) {
        regisAPI *focusRegis = _startRegisPTAConf;
        while (focusRegis->next != NULL) {
            focusRegis = (regisAPI *)focusRegis->next;
        }
        focusRegis->next = newRegis;
    } else {
        _startRegisPTAConf = newRegis;
    }
}

void MAGELLAN_MQTT_device_core::getConfigJSON(conf_Json_handleCallback conf_json_callback)
{
  attr.conf_regis_json = true;
  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONFIG_JSON;
  newRegis->conf_json_callback = conf_json_callback;
  newRegis->next = NULL;

  if (_startRegisJSONConf != NULL) {
        regisAPI *focusRegis = _startRegisJSONConf;
        while (focusRegis->next != NULL) {
            focusRegis = (regisAPI *)focusRegis->next;
        }
        focusRegis->next = newRegis;
    } else {
        _startRegisJSONConf = newRegis;
    }
}

void MAGELLAN_MQTT_device_core::getConfigJSON(conf_JsonOBJ_handleCallback jsonOBJ_cb)
{
  attr.conf_jsonOBJ = true;
  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONFIG_JSON;
  newRegis->conf_obj_callback = jsonOBJ_cb;
  newRegis->next = NULL;

  if (_startOBJ_CONF != NULL) {
        regisAPI *focusRegis = _startOBJ_CONF;
        while (focusRegis->next != NULL) {
            focusRegis = (regisAPI *)focusRegis->next;
        }
        focusRegis->next = newRegis;
    } else {
        _startOBJ_CONF = newRegis;
    }
}

void MAGELLAN_MQTT_device_core::getRESP(unsigned int resp_event, resp_callback resp_cb)
{
  attr.resp_regis = true;
  regisAPI *newRegis = new regisAPI;
  newRegis->Event = resp_event;
  newRegis->resp_h_callback = resp_cb;
  newRegis->next = NULL;

  if (_startRESP != NULL) {
        regisAPI *focusRegis = _startRESP;
        while (focusRegis->next != NULL) {
            focusRegis = (regisAPI *)focusRegis->next;
        }
        focusRegis->next = newRegis;
    } else {
        _startRESP = newRegis;
    }
}

boolean MAGELLAN_MQTT_device_core:: registerResponseReport(int format)
{
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/"+ token +"/report/resp/pta/+";
    break;
  case 1:
    topic = "api/v2/thing/"+ token +"/report/resp";
    break;
  default:
    Serial.println(F("out of length resp args format support [\"0\" or PLAINTEXT] is Plaint text(default) and [\"1\" or JSON]"));
    topic = "api/v2/thing/"+ token +"/report/resp";
    break;
  }
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  String respType = (format == 0)? "Plaintext":"JSON";
  Serial.println(F("-------------------------------"));
  // Serial.println("# RegisterRESP Report: "+ _debug);
  Serial.println("# Subscribe Response Report: "+ _debug);
  Serial.println("# Response type: "+ respType);
  return Sub_status;
}


boolean MAGELLAN_MQTT_device_core:: registerResponseReportTimestamp()
{
  String topic = "api/v2/thing/"+ token +"/report/timestamp/resp";
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  Serial.println(F("-------------------------------"));
  // Serial.println("# RegisterRESP ReportTimestamp: "+ _debug);
  Serial.println("# Subscribe Response ReportTimestamp: "+ _debug);
  return Sub_status;
}


boolean MAGELLAN_MQTT_device_core:: registerResponseHeartbeat(int format)
{
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/"+ token +"/heartbeat/resp/pta";
    break;
  case 1:
    topic = "api/v2/thing/"+ token +"/heartbeat/resp";
    break;
  default:
    Serial.println(F("out of length resp args format support [\"0\" or PLAINTEXT] is Plaint text(default) and [\"1\" or JSON]"));
    topic = "api/v2/thing/"+ token +"/heartbeat/resp";
    break;
  }
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  String respType = (format == 0)? "Plaintext":"JSON";
  Serial.println(F("-------------------------------"));
  // Serial.println("# RegisterRESP Heartbeat: "+ _debug);
  Serial.println("# Subscribe Response Heartbeat: "+ _debug);
  Serial.println("# Response type: "+ respType);
  return Sub_status;
}

boolean MAGELLAN_MQTT_device_core::registerConfig(String key)
{
  String topic  = "api/v2/thing/"+ token +"/config/resp/pta/?config="+ key; //fact C c
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  Serial.println(F("-------------------------------"));
  // Serial.println("# Register Server Config [Key]: \""+key+"\" Register: "+ _debug);
  Serial.println("# Subscribe ServerConfig [Key]: \""+key+"\" Subscribe: "+ _debug);
  return Sub_status;
}

boolean MAGELLAN_MQTT_device_core::registerConfig(int format)
{
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/"+ token +"/config/resp/pta/+";
    break;
  case 1:
    topic = "api/v2/thing/"+ token +"/config/resp";
    break;
  default:
    Serial.println(F("out of length resp args format support [\"0\" or PLAINTEXT] is Plaint text(default) and [\"1\" or JSON]"));
    topic = "api/v2/thing/"+ token +"/config/resp";
    break;
  }
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  String respType = (format == 0)? "Plaintext":"JSON";
  Serial.println(F("-------------------------------"));
  // Serial.println("# Register Server Config: "+ _debug);
  Serial.println("# Subscribe ServerConfig: "+ _debug);
  Serial.println("# Response type: "+ respType);
  return Sub_status;
}

boolean MAGELLAN_MQTT_device_core::getTimestamp()
{
  String topic = "api/v2/server/dateTime/req";
  boolean Pub_status = this->client->publish(topic.c_str(), " ");
  _debug = (Pub_status == true)? "Success" : "Failure"; 
  Serial.println(F("-------------------------------"));
  Serial.println("# Get ServerTime Request: "+ _debug);
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::registerTimestamp(int format)
{
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/server/dateTime/resp/pta";
    break;
  case 1:
    topic = "api/v2/server/dateTime/resp";
    break;
  default:
    Serial.println(F("out of length resp args format support [\"0\" or PLAINTEXT] is Plaint text(default) and [\"1\" or JSON]"));
    topic = "api/v2/server/dateTime/resp";
    break;
  }
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  String respType = (format == 0)? "Plaintext":"JSON";
  Serial.println(F("-------------------------------"));
  Serial.println(F("# Subscribe Timestamp magellan"));
  // Serial.println(F("# RegisterTimestamp magellan"));
  Serial.println("# Subscribe ServerTime: "+ _debug);
  // Serial.println("# RegisterTimestamp: "+ _debug);
  Serial.println("# Response type: "+ respType);
  return Sub_status;
}

boolean MAGELLAN_MQTT_device_core::registerControl(int format)
{
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/"+ token +"/delta/resp/pta/+";
    break;
  case 1:
    topic = "api/v2/thing/"+ token +"/delta/resp";
    break;
  default:
    Serial.println(F("out of length resp args format support [\"0\" or PLAINTEXT] is Plaint text(default) and [\"1\" or JSON]"));
    topic = "api/v2/thing/"+ token +"/delta/resp";
    break;
  }
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  String respType = (format == 0)? "Plaintext":"JSON";
  Serial.println(F("-------------------------------"));
  // Serial.println("# RegisterControl: "+ _debug);
  Serial.println("# Subscribe Control: "+ _debug);
  Serial.println("# Response type: "+ respType);
  return Sub_status;
}

boolean MAGELLAN_MQTT_device_core::registerControl(String key)
{
  String topic  = "api/v2/thing/"+ token +"/delta/resp/pta/?sensor="+ key;  //fact S s
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  Serial.println(topic);
  Serial.println(F("-------------------------------"));
  // Serial.println("# RegisterControl [Key]: \""+key+"\" Register: "+ _debug);
  Serial.println("# Subscribe Control [Key]: \""+key+"\" Subscribe: "+ _debug);
  return Sub_status;
}

void MAGELLAN_MQTT_device_core::thingRegister()
{
  while (!flagRegisterToken)
  {
    this->flagRegisterToken = registerToken();
  }
  while (!flagToken)
  {
    loop();
    requestToken();
    if(attr.ext_Token.length() >= 30)
    {
      token = attr.ext_Token;
      acceptToken(token);
    }
  }
}

void MAGELLAN_MQTT_device_core::registerList(func_callback_registerList cb_regisList)
{
  if(attr.triggerRemainSub)
  {

    Serial.println(F("# Subscribes List"));
    cb_regisList();   
    attr.triggerRemainSub = false;
    Serial.println(F("#============================"));
  }
}

void MAGELLAN_MQTT_device_core::interval_ms(unsigned long ms,  func_callback_ms cb_ms)
{
  threshold_ms = ms;
  now_time = millis();
  unsigned long different_ms = now_time - prev_time;
  if(different_ms >= threshold_ms)
  {
    prev_time = millis();
    cb_ms();
  }
}

JsonObject MAGELLAN_MQTT_device_core::deserialJson(String jsonContent)
{
  JsonObject buffer;
  if(jsonContent != NULL && jsonContent != "clear")
  {
    DeserializationError error = deserializeJson(docJson, jsonContent);
    buffer = docJson.as<JsonObject>();
    if(error)
      Serial.println("# Error to DeserializeJson Control");
  }
  return buffer;
}

String MAGELLAN_MQTT_device_core::deserialControlJSON(String jsonContent)
{
  String content = "40300";
  JsonObject buffdoc = deserialJson(jsonContent);
  String statusCode = buffdoc["Code"];
  String buffDelta ;
  if( statusCode == "20000")
  {
    if(jsonContent.indexOf("Delta") != -1)
    {
      buffDelta = buffdoc["Delta"].as<String>();
      content = buffDelta;
    }
    else if(jsonContent.indexOf("Sensor") != -1)
    {
      buffDelta = buffdoc["Sensor"].as<String>();
      content = buffDelta;
    }
  }
  return content;
}

void MAGELLAN_MQTT_device_core::updateSensor(String key, String value, JsonDocument &ref_docs)
{
  int len = value.length();
  char * c_value = new char[len +1];
  std::copy(value.begin(), value.end(), c_value);
  c_value[len] = '\0';
  ref_docs[key] = c_value;
  delete[] c_value;
}

void MAGELLAN_MQTT_device_core::updateSensor(String key, const char* value, JsonDocument &ref_docs)
{
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::updateSensor(String key, int value, JsonDocument &ref_docs)
{
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::updateSensor(String key, float value, JsonDocument &ref_docs)
{
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::updateSensor(String key, boolean value, JsonDocument &ref_docs)
{
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::addSensor(String key, String value, JsonDocument &ref_docs)
{
  int len = value.length();
  char * c_value = new char[len +1];
  std::copy(value.begin(), value.end(), c_value);
  c_value[len] = '\0';
  ref_docs[key] = c_value;
  delete[] c_value;
}

void MAGELLAN_MQTT_device_core::addSensor(String key, const char* value, JsonDocument &ref_docs)
{
  // Serial.println("[Key]: "+key+" [Value]: "+value);
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::addSensor(String key, int value, JsonDocument &ref_docs)
{
  // Serial.println("[Key]: "+key+" [Value]: "+String(value));
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::addSensor(String key, float value, JsonDocument &ref_docs)
{
  // Serial.println("[Key]: "+key+" [Value]: "+String(value));
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::addSensor(String key, boolean value, JsonDocument &ref_docs)
{
  // Serial.println("[Key]: "+key+" [Value]: "+String(value));
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::remove(String key, JsonDocument &ref_docs)
{
  Serial.println("Remove [Key]: "+key);
  ref_docs.remove(key);
}

boolean MAGELLAN_MQTT_device_core::findKey(String key, JsonDocument &ref_docs)
{
  return ref_docs.containsKey(key);
}

String MAGELLAN_MQTT_device_core::buildSensorJSON(JsonDocument &ref_docs)
{
  String bufferJsonStr;
  // Serial.println("# [Build JSON Key is]: "+ String(ref_docs.size()) +" key");
  size_t mmr_usage = ref_docs.memoryUsage();
  size_t max_size = ref_docs.memoryPool().capacity();
  size_t safety_size = max_size * (0.97);
  // Serial.println("Safety size: "+String(safety_size));
  if(mmr_usage >= safety_size)
  {
    bufferJsonStr = "null";
    Serial.println("# [Overload memory toJSONString] *Maximum Safety Memory size to use is: "+String(safety_size));
  }
  else
  {
    serializeJson(ref_docs, bufferJsonStr);
    Serial.println("# [to JSON String Key is]: "+ String(ref_docs.size()) +" key");
  }


  Serial.println("# MemoryUsage: "+ String(mmr_usage)+"/"+String(safety_size)+" from("+String(ref_docs.memoryPool().capacity())+")");
  return bufferJsonStr;
}

void MAGELLAN_MQTT_device_core::adjustBufferSensor(size_t sizeJSONbuffer)
{
  attr.docSensor = new DynamicJsonDocument(sizeJSONbuffer);
}

int MAGELLAN_MQTT_device_core::readBufferSensor(JsonDocument &ref_docs)
{
  return ref_docs.memoryPool().capacity();
}

void MAGELLAN_MQTT_device_core::clearSensorBuffer(JsonDocument &ref_docs)
{
  Serial.println(F("# [Clear Json buffer]"));
  ref_docs.clear();
  Serial.println(F("-------------------------------"));
}

////////////////// Unsub ///////////
boolean MAGELLAN_MQTT_device_core::unregisterControl(int format){
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/"+ token +"/delta/resp/pta/+";
    break;
  case 1:
    topic = "api/v2/thing/"+ token +"/delta/resp";
    break;
  default:
    topic = "api/v2/thing/"+ token +"/delta/resp";
    break;
  }
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  String respType = (format == 0)? "Plaintext":"JSON";
  Serial.println(F("-------------------------------"));
  // Serial.println("# RegisterControl: "+ _debug);
  Serial.println("# Unsubscribe Control: "+ _debug);
  Serial.println("# Response type: "+ respType);
  return Sub_status;
} //
boolean MAGELLAN_MQTT_device_core::unregisterControl(String key){
  String topic  = "api/v2/thing/"+ token +"/delta/resp/pta/?sensor="+ key;  //fact S s
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  Serial.println(topic);
  Serial.println(F("-------------------------------"));
  Serial.println("# Unsubscribe Control [Key]: \""+key+"\" Unsubscribe: "+ _debug);
  return Sub_status;
} //
boolean MAGELLAN_MQTT_device_core::unregisterConfig(int format){
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/"+ token +"/config/resp/pta/+";
    break;
  case 1:
    topic = "api/v2/thing/"+ token +"/config/resp";
    break;
  default:
    topic = "api/v2/thing/"+ token +"/config/resp";
    break;
  }
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  String respType = (format == 0)? "Plaintext":"JSON";
  Serial.println(F("-------------------------------"));
  Serial.println("# Unsubscribe ServerConfig: "+ _debug);
  Serial.println("# Response type: "+ respType);
  return Sub_status;
} // 
boolean MAGELLAN_MQTT_device_core::unregisterConfig(String key){
  String topic  = "api/v2/thing/"+ token +"/config/resp/pta/?config="+ key; //fact C c
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  Serial.println(F("-------------------------------"));
  Serial.println("# Unsubscribe ServerConfig [Key]: \""+key+"\" Unsubscribe: "+ _debug);
  return Sub_status;
} //
boolean MAGELLAN_MQTT_device_core::unregisterTimestamp(int format){
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/server/dateTime/resp/pta";
    break;
  case 1:
    topic = "api/v2/server/dateTime/resp";
    break;
  default:
    topic = "api/v2/server/dateTime/resp";
    break;
  }
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  String respType = (format == 0)? "Plaintext":"JSON";
  Serial.println(F("-------------------------------"));
  Serial.println("# Unsubscribe ServerTime: "+ _debug);
  Serial.println("# Response type: "+ respType);
  return Sub_status;
} //
boolean MAGELLAN_MQTT_device_core::unregisterResponseReport(int format){
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/"+ token +"/report/resp/pta/+";
    break;
  case 1:
    topic = "api/v2/thing/"+ token +"/report/resp";
    break;
  default:
    topic = "api/v2/thing/"+ token +"/report/resp";
    break;
  }
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  String respType = (format == 0)? "Plaintext":"JSON";
  Serial.println(F("-------------------------------"));
  Serial.println("# Unsubscribe Response Report: "+ _debug);
  Serial.println("# Response type: "+ respType);
  return Sub_status;
}
boolean MAGELLAN_MQTT_device_core::unregisterResponseReportTimestamp(){
  String topic = "api/v2/thing/"+ token +"/report/timestamp/resp";
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  Serial.println(F("-------------------------------"));
  Serial.println("# Unsubscribe Response ReportTimestamp: "+ _debug);
  return Sub_status;
}
boolean MAGELLAN_MQTT_device_core::unregisterResponseHeartbeat(int format){
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/"+ token +"/heartbeat/resp/pta";
    break;
  case 1:
    topic = "api/v2/thing/"+ token +"/heartbeat/resp";
    break;
  default:
    topic = "api/v2/thing/"+ token +"/heartbeat/resp";
    break;
  }
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true)? "Success" : "Failure";
  String respType = (format == 0)? "Plaintext":"JSON";
  Serial.println(F("-------------------------------"));
  Serial.println("# Unsubscribe Response Heartbeat: "+ _debug);
  Serial.println("# Response type: "+ respType);
  return Sub_status;
}
////////////////// /Unsub //////////

