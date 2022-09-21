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
 
Author:(POC Device Magellan team)      
Create Date: 25 April 2022. 
Modified: 15 september 2022.
*/

#include "MAGELLAN_SIM7600E_MQTT.h"

MAGELLAN_MQTT_device_core *MAGELLAN_SIM7600E_MQTT::coreMQTT = NULL;

MAGELLAN_SIM7600E_MQTT::MAGELLAN_SIM7600E_MQTT()
{
  this->coreMQTT = new MAGELLAN_MQTT_device_core();
}

MAGELLAN_SIM7600E_MQTT::MAGELLAN_SIM7600E_MQTT(Client& client)
{
  this->coreMQTT = new MAGELLAN_MQTT_device_core(client);
}

void MAGELLAN_SIM7600E_MQTT::begin(uint16_t bufferSize, boolean builtInSensor)
{
  if(bufferSize > _default_OverBufferSize)
  {
    Serial.print(F("# You have set a buffer size greater than 8192, adjusts to: "));
    Serial.println(_default_OverBufferSize);
    this->setMQTTBufferSize(_default_OverBufferSize);   
  }
  else
  {
    this->setMQTTBufferSize(bufferSize);
  }

  this->coreMQTT->begin(builtInSensor);
}

void MAGELLAN_SIM7600E_MQTT::begin(String _thingIden, String _thingSencret, String _imei, unsigned int Zone, uint16_t bufferSize, boolean builtinSensor)
{
  this->coreMQTT->begin(_thingIden, _thingSencret, _imei, Zone, bufferSize, builtinSensor);
}

void MAGELLAN_SIM7600E_MQTT::beginCustom(String _client_id, boolean buildinSensor, String _host, int _port, uint16_t bufferSize)
{
  this->coreMQTT->beginCustom(_client_id, buildinSensor, _host, _port, bufferSize);
}

void MAGELLAN_SIM7600E_MQTT::loop()
{
  this->coreMQTT->loop();
}

void MAGELLAN_SIM7600E_MQTT::heartbeat(unsigned int second)
{
  this->coreMQTT->heartbeat(second * 1000);
}

String MAGELLAN_SIM7600E_MQTT::deserializeControl(String payload)
{
  return this->coreMQTT->deserialControlJSON(payload);
}

void MAGELLAN_SIM7600E_MQTT::Centric::begin(uint16_t setBufferSize)
{
  if(setBufferSize > _default_OverBufferSize)
  {
    Serial.print(F("# You have set a buffer size greater than 8192, adjusts to: "));
    Serial.println(_default_OverBufferSize); 
    coreMQTT->setMQTTBufferSize(_default_OverBufferSize);
  }
  else
  {
    coreMQTT->setMQTTBufferSize(setBufferSize);
  }
  coreMQTT->beginCentric();
}

boolean MAGELLAN_SIM7600E_MQTT::Report::send(String payload)
{
  return coreMQTT->report(payload);
}

boolean MAGELLAN_SIM7600E_MQTT::Report::send(String key, String value)
{
  return coreMQTT->report(key, value);
}

boolean MAGELLAN_SIM7600E_MQTT::Report::send(int UnixtsTimstamp, String payload)
{
  String u_timestamp = String(UnixtsTimstamp);
  return coreMQTT->reportTimestamp(u_timestamp, payload, SET_UNIXTS);
}

float MAGELLAN_SIM7600E_MQTT::BuiltinSensor::readTemperature()
{
  return mySensor.readTemperature();
}

float MAGELLAN_SIM7600E_MQTT::BuiltinSensor::readHumidity()
{
  return mySensor.readHumidity();
}

boolean MAGELLAN_SIM7600E_MQTT::GPSmodule::available()
{
  return mySensor.GPSavailable();
}

float MAGELLAN_SIM7600E_MQTT::GPSmodule::readLatitude()
{
  return mySensor.readLatitude();
}

float MAGELLAN_SIM7600E_MQTT::GPSmodule::readLongitude()
{
  return mySensor.readLongitude();
}

float MAGELLAN_SIM7600E_MQTT::GPSmodule::readAltitude()
{
  return mySensor.readAltitude();
}

float MAGELLAN_SIM7600E_MQTT::GPSmodule::readSpeed()
{
  return mySensor.readSpeed();
}

float MAGELLAN_SIM7600E_MQTT::GPSmodule::readCourse()
{
  return mySensor.readCourse();
}

String MAGELLAN_SIM7600E_MQTT::GPSmodule::readLocation()
{
  return mySensor.readLocation();
}

unsigned long MAGELLAN_SIM7600E_MQTT::GPSmodule::getUnixTime()
{
  return mySensor.getUnixTime();
} 

boolean MAGELLAN_SIM7600E_MQTT::Subscribe::Report::response(unsigned int format)
{
  return coreMQTT->registerResponseReport(format);
}

boolean MAGELLAN_SIM7600E_MQTT::Subscribe::ReportWithTimestamp::response()
{
  return coreMQTT->registerResponseReportTimestamp();
}

boolean MAGELLAN_SIM7600E_MQTT::Subscribe::Heartbeat::response(unsigned int format)
{
  return coreMQTT->registerResponseHeartbeat(format);
}

boolean MAGELLAN_SIM7600E_MQTT::Subscribe::control(unsigned int format)
{
  return coreMQTT->registerControl(format);
}

boolean MAGELLAN_SIM7600E_MQTT::Subscribe::control(String controlKey)
{
  return coreMQTT->registerControl(controlKey);
}

boolean MAGELLAN_SIM7600E_MQTT::Subscribe::serverConfig(unsigned int format)
{
  return coreMQTT->registerConfig(format);
}

boolean MAGELLAN_SIM7600E_MQTT::Subscribe::serverConfig(String controlKey)
{
  return coreMQTT->registerConfig(controlKey);
}

boolean MAGELLAN_SIM7600E_MQTT::Subscribe::getServerTime(unsigned int format)
{
  return coreMQTT->registerTimestamp(format);
}

////////////////////////////////////////////////////////////////////////////////
boolean MAGELLAN_SIM7600E_MQTT::Unsubscribe::Report::response(unsigned int format)
{
  return coreMQTT->unregisterResponseReport(format);
}

boolean MAGELLAN_SIM7600E_MQTT::Unsubscribe::ReportWithTimestamp::response()
{
  return coreMQTT->unregisterResponseReportTimestamp();
}

boolean MAGELLAN_SIM7600E_MQTT::Unsubscribe::Heartbeat::response(unsigned int format)
{
  return coreMQTT->unregisterResponseHeartbeat(format);
}

boolean MAGELLAN_SIM7600E_MQTT::Unsubscribe::control(unsigned int format)
{
  return coreMQTT->unregisterControl(format);
}

boolean MAGELLAN_SIM7600E_MQTT::Unsubscribe::control(String controlKey)
{
  return coreMQTT->unregisterControl(controlKey);
}

boolean MAGELLAN_SIM7600E_MQTT::Unsubscribe::serverConfig(unsigned int format)
{
  return coreMQTT->unregisterConfig(format);
}

boolean MAGELLAN_SIM7600E_MQTT::Unsubscribe::serverConfig(String controlKey)
{
  return coreMQTT->unregisterConfig(controlKey);
}

boolean MAGELLAN_SIM7600E_MQTT::Unsubscribe::getServerTime(unsigned int format)
{
  return coreMQTT->unregisterTimestamp(format);
}
////////////////////////////////////////////////////////////////////////////////

void MAGELLAN_SIM7600E_MQTT::Information::getBoardInfo()
{
  coreMQTT->getBoardInfo();
}

String MAGELLAN_SIM7600E_MQTT::Information::getHostName()
{
  return coreMQTT->getHostName();
}

String MAGELLAN_SIM7600E_MQTT::Information::getThingToken()
{
  return coreMQTT->readToken();
}

String MAGELLAN_SIM7600E_MQTT::Information::getICCID()
{
  return coreMQTT->getICCID();
}

String MAGELLAN_SIM7600E_MQTT::Information::getIMSI()
{
  return coreMQTT->getIMEI();
}

String MAGELLAN_SIM7600E_MQTT::Information::getIMEI()
{
  return coreMQTT->getIMEI();
}

boolean MAGELLAN_SIM7600E_MQTT::Control::ACK(String controlKey, String controlValue)
{
  return coreMQTT->ACKControl(controlKey, controlValue);
}

boolean MAGELLAN_SIM7600E_MQTT::Control::ACK(String payload)
{
  return coreMQTT->ACKControl(payload);
}

void MAGELLAN_SIM7600E_MQTT::Control::request()
{
  coreMQTT->reqControlJSON();
}

void MAGELLAN_SIM7600E_MQTT::Control::request(String controlKey)
{
  coreMQTT->reqControl(controlKey);
}


void MAGELLAN_SIM7600E_MQTT::ServerConfig::request()
{
  coreMQTT->reqConfigJSON();
}

void MAGELLAN_SIM7600E_MQTT::ServerConfig::request(String serverConfigKey)
{
  coreMQTT->reqConfig(serverConfigKey);
}

void MAGELLAN_SIM7600E_MQTT::Sensor::add(String sensorKey, String sensorValue)
{
  if(sensorValue == "null")
  {
    Serial.println("# add [Key] \""+sensorKey+"\" failed, this function does not allow to set value \"null\"");
  }
  else{
    coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
  }

}

void MAGELLAN_SIM7600E_MQTT::Sensor::add(String sensorKey, const char* sensorValue)
{
  if(sensorValue == "null")
  {
    Serial.println("# add [Key] \""+sensorKey+"\" failed, this function does not allow to set value \"null\"");
  }
  else{
    coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::add(String sensorKey, int sensorValue)
{
  coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
}

void MAGELLAN_SIM7600E_MQTT::Sensor::add(String sensorKey, float sensorValue)
{
  coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
}

void MAGELLAN_SIM7600E_MQTT::Sensor::add(String sensorKey, boolean sensorValue)
{
  coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
}

String MAGELLAN_SIM7600E_MQTT::Sensor::toJSONString()
{
  return coreMQTT->buildSensorJSON(*attr.docSensor);
}

void MAGELLAN_SIM7600E_MQTT::Sensor::report()
{
  String bufferPlayload = coreMQTT->buildSensorJSON(*attr.docSensor);
  if(bufferPlayload.indexOf("null") == -1)
  {
    coreMQTT->report(bufferPlayload);
    coreMQTT->clearSensorBuffer(*attr.docSensor);
  }
  else
  {
    Serial.println(F("# Can't sensor.report Because Not set function \"sensor.add(key,value)\" before sensor.report or Overload Memory toJSONString"));
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::remove(String sensorKey)
{
  if(findKey(sensorKey))
  {
    coreMQTT->remove(sensorKey, *attr.docSensor);
  }
  else{
    Serial.println("Not found [Key]: \""+ sensorKey+"\" to Remove");
  }
}

boolean MAGELLAN_SIM7600E_MQTT::Sensor::findKey(String sensorKey)
{
  return coreMQTT->findKey(sensorKey, *attr.docSensor);
}

void MAGELLAN_SIM7600E_MQTT::Sensor::update(String sensorKey, String sensorValue)
{
  if(findKey(sensorKey))
  {
    Serial.println("Updated [Key]: "+ sensorKey);
    coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
  }
  else{
    Serial.println("Not found [Key]: \""+ sensorKey+"\" to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::update(String sensorKey, const char* sensorValue)
{
  if(findKey(sensorKey))
  {
    Serial.println("Updated [Key]: "+ sensorKey);
    coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
  }
  else{
    Serial.println("Not found [Key]: "+ sensorKey+" to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::update(String sensorKey, int sensorValue)
{
  if(findKey(sensorKey))
  {
    Serial.println("Updated [Key]: "+ sensorKey);
    coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
  }
  else{
    Serial.println("Not found [Key]: "+ sensorKey+" to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::update(String sensorKey, float sensorValue)
{
  if(findKey(sensorKey))
  {
    Serial.println("Updated [Key]: "+ sensorKey);
    coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
  }
  else{
    Serial.println("Not found [Key]: "+ sensorKey+" to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::update(String sensorKey, boolean sensorValue)
{
  if(findKey(sensorKey))
  {
    Serial.println("Updated [Key]: "+ sensorKey);
    coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
  }
  else{
    Serial.println("Not found [Key]: "+ sensorKey+" to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::Location::add(String LocationKey, double latitude, double longtitude)
{
  char b_lat[25];
  char b_lng[25]; 
  sprintf(b_lat, "%f", latitude);
  sprintf(b_lng, "%f", longtitude);
  String location = String(b_lat)+","+String(b_lng);
  coreMQTT->addSensor(LocationKey, location, *attr.docSensor);
}

void MAGELLAN_SIM7600E_MQTT::Sensor::Location::add(String LocationKey, String latitude, String longtitude)
{
  if((coreMQTT->CheckString_isDouble(latitude)) && (coreMQTT->CheckString_isDouble(longtitude)))
  {
    String location = latitude +","+ longtitude;
    coreMQTT->addSensor(LocationKey, location, *attr.docSensor);
  }
  else{
    Serial.println("# ["+LocationKey+"] Can't add Location latitude or longtitude is invalid (not number)");
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::Location::update(String LocationKey, double latitude, double longtitude)
{
    if(coreMQTT->findKey(LocationKey, *attr.docSensor))
    {
      char b_lat[25];
      char b_lng[25]; 
      sprintf(b_lat, "%f", latitude);
      sprintf(b_lng, "%f", longtitude);
      String location = String(b_lat)+","+String(b_lng);
      Serial.println("Updated [Key]: "+ LocationKey);
      coreMQTT->updateSensor(LocationKey, location, *attr.docSensor);
    }
    else
    {
      Serial.println("Not found [Key]: "+ LocationKey+" to update");      
    }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::Location::update(String LocationKey, String latitude, String longtitude)
{
  if((coreMQTT->CheckString_isDouble(latitude)) && (coreMQTT->CheckString_isDouble(longtitude)))
  {
    if(coreMQTT->findKey(LocationKey, *attr.docSensor))
    {
      String location = String(latitude)+","+String(longtitude);
      Serial.println("Updated [Key]: "+ LocationKey);
      coreMQTT->updateSensor(LocationKey, location, *attr.docSensor);
    }
    else
    {
      Serial.println("Not found [Key]: \""+ LocationKey+"\" to update");      
    }
  }
  else{
    Serial.println("# ["+LocationKey+"] Can't update Location latitude or longtitude is invalid (not number)");
  }

}

void MAGELLAN_SIM7600E_MQTT::Sensor::clear()
{
  coreMQTT->clearSensorBuffer(*attr.docSensor);
}

void MAGELLAN_SIM7600E_MQTT::Sensor::setJSONBufferSize(size_t JsonBuffersize){
  Serial.print("# Set JSON buffer size: "+String(JsonBuffersize));
  coreMQTT->adjustBufferSensor(JsonBuffersize);
  Serial.println(" Status: " + String((readJSONBufferSize() == (int)JsonBuffersize)? "Success":"Fail"));
}
int MAGELLAN_SIM7600E_MQTT::Sensor::readJSONBufferSize()
{
  return coreMQTT->readBufferSensor(*attr.docSensor);
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::add(String clientConfigKey, String clientConfigValue)
{
  if(clientConfigValue == "null")
  {
    Serial.println("# add [Key] \""+clientConfigKey+"\" failed, this function does not allow to set value \"null\"");
  }
  else{
    coreMQTT->addSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
  }
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::add(String clientConfigKey, const char* clientConfigValue)
{
  if(clientConfigValue == "null")
  {
    Serial.println("# add [Key] "+clientConfigKey+" failed, this function not allow to set value \"null\"");
  }
  else{
    coreMQTT->addSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
  }
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::add(String clientConfigKey, int clientConfigValue)
{
  coreMQTT->addSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::add(String clientConfigKey, float clientConfigValue)
{
  coreMQTT->addSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::add(String clientConfigKey, boolean clientConfigValue)
{
  coreMQTT->addSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
}

String MAGELLAN_SIM7600E_MQTT::ClientConfig::toJSONString()
{
  return coreMQTT->buildSensorJSON(attr.docClientConf);
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::save()
{
  String bufferPlayload = coreMQTT->buildSensorJSON(attr.docClientConf);
  boolean Pub_status = false;
  if(bufferPlayload.indexOf("null") == -1)
  {
    coreMQTT->reportClientConfig(bufferPlayload);
    coreMQTT->clearSensorBuffer(attr.docClientConf);
  }
  else
  {
    Serial.println(F("# Can't clientConfig.save Because Not set function \"clientConfig.add(key,value)\" before clientConfig.save"));
  }
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::save(String payload)
{
  coreMQTT->reportClientConfig(payload);
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::remove(String clientConfigKey)
{
  if(coreMQTT->findKey(clientConfigKey, attr.docClientConf))
  {
    coreMQTT->remove(clientConfigKey, attr.docClientConf);
  }
  else{
    Serial.println("Not found [Key]: \""+ clientConfigKey+"\" to Remove");
  }
}

boolean MAGELLAN_SIM7600E_MQTT::ClientConfig::findKey(String clientConfigKey)
{
  return coreMQTT->findKey(clientConfigKey, attr.docClientConf);
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::update(String clientConfigKey, String clientConfigValue)
{
  if(coreMQTT->findKey(clientConfigKey, attr.docClientConf))
  {
    Serial.println("Updated [Key]: "+ clientConfigKey);
    coreMQTT->updateSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
  }
  else{
    Serial.println("Not found [Key]: "+ clientConfigKey+" to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::update(String clientConfigKey, const char* clientConfigValue)
{
  if(findKey(clientConfigKey))
  {
    Serial.println("Updated [Key]: "+ clientConfigKey);
    coreMQTT->updateSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
  }
  else{
    Serial.println("Not found [Key]: "+ clientConfigKey+" to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::update(String clientConfigKey, int clientConfigValue)
{
  if(findKey(clientConfigKey))
  {
    Serial.println("Updated [Key]: "+ clientConfigKey);
    coreMQTT->updateSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
  }
  else{
    Serial.println("Not found [Key]: "+ clientConfigKey+" to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::update(String clientConfigKey, float clientConfigValue)
{
  if(findKey(clientConfigKey))
  {
    Serial.println("Updated [Key]: "+ clientConfigKey);
    coreMQTT->updateSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
  }
  else{
    Serial.println("Not found [Key]: "+ clientConfigKey+" to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::update(String clientConfigKey, boolean clientConfigValue)
{
  if(findKey(clientConfigKey))
  {
    Serial.println("Updated [Key]: "+ clientConfigKey);
    coreMQTT->updateSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
  }
  else{
    Serial.println("Not found [Key]: "+ clientConfigKey+" to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::clear()
{
  coreMQTT->clearSensorBuffer(attr.docClientConf);
}

void MAGELLAN_SIM7600E_MQTT::subscribes(func_callback_registerList cb_subscribe_list)
{
  coreMQTT->registerList(cb_subscribe_list);
}

void MAGELLAN_SIM7600E_MQTT::interval(unsigned long second, func_callback_ms cb_interval)
{
  coreMQTT->interval_ms(second * 1000, cb_interval);
}

boolean MAGELLAN_SIM7600E_MQTT::getServerTime()
{
  return coreMQTT->getTimestamp();
}

void MAGELLAN_SIM7600E_MQTT::getControl(String focusOnKey, ctrl_handleCallback ctrl_callback)
{
  coreMQTT->getControl(focusOnKey, ctrl_callback);
}

void MAGELLAN_SIM7600E_MQTT::getControl(ctrl_PTAhandleCallback ctrl_pta_callback)
{
  coreMQTT->getControl(ctrl_pta_callback);
}

void MAGELLAN_SIM7600E_MQTT::getControlJSON(ctrl_Json_handleCallback ctrl_json_callback)
{
  coreMQTT->getControlJSON(ctrl_json_callback);
}

void MAGELLAN_SIM7600E_MQTT::getControlJSON(ctrl_JsonOBJ_handleCallback jsonOBJ_cb)
{
  coreMQTT->getControlJSON(jsonOBJ_cb);  
}

void MAGELLAN_SIM7600E_MQTT::getServerConfig(String focusOnKey, conf_handleCallback _conf_callback)
{
  coreMQTT->getConfig(focusOnKey, _conf_callback);
}

void MAGELLAN_SIM7600E_MQTT::getServerConfig(conf_PTAhandleCallback conf_pta_callback)
{
  coreMQTT->getConfig(conf_pta_callback);
}

void MAGELLAN_SIM7600E_MQTT::getServerConfigJSON(conf_Json_handleCallback conf_json_callback)
{
  coreMQTT->getConfigJSON(conf_json_callback);
}

void MAGELLAN_SIM7600E_MQTT::getServerConfigJSON(conf_JsonOBJ_handleCallback jsonOBJ_cb)
{
  coreMQTT->getConfigJSON(jsonOBJ_cb);
}

void MAGELLAN_SIM7600E_MQTT::getResponse(unsigned int eventResponse ,resp_callback resp_cb)
{
  coreMQTT->getRESP(eventResponse, resp_cb);
}
