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
Magellan_4GBoard v2.6.1 AIS 4G Board.
support SIMCOM SIM7600E(AIS 4G Board)
 
Author:(POC Device Magellan team)      
Create Date: 25 April 2022. 
Modified: 9 december 2022.
Released for private usage.
*/

#include "MAGELLAN_SIM7600E_MQTT.h"

MAGELLAN_MQTT_device_core *MAGELLAN_SIM7600E_MQTT::coreMQTT = NULL;
Setting setting;
MAGELLAN_SIM7600E_MQTT::MAGELLAN_SIM7600E_MQTT()
{
  this->coreMQTT = new MAGELLAN_MQTT_device_core();
}

MAGELLAN_SIM7600E_MQTT::MAGELLAN_SIM7600E_MQTT(Client& client)
{
  this->coreMQTT = new MAGELLAN_MQTT_device_core(client);
}
void MAGELLAN_SIM7600E_MQTT::begin(Setting _setting)
{
  if(_setting.clientBufferSize > _default_OverBufferSize)
  {
    Serial.print(F("# You have set a buffer size greater than 8192, adjusts to: "));
    Serial.println(_default_OverBufferSize);
    this->setMQTTBufferSize(_default_OverBufferSize);   
    attr.calculate_chunkSize = _default_OverBufferSize/2;
  }
  else{
    this->setMQTTBufferSize(_setting.clientBufferSize);
    attr.calculate_chunkSize = _setting.clientBufferSize/2;
  }

  size_t revertPartToBufferSize = attr.calculate_chunkSize * 2;
  // validate credential when user forget set ICCID and IMSI and useGSM getInfo from 4G board
  if((_setting.ThingIdentifier == "null" && _setting.ThingSecret  == "null") && attr.clientNetInterface == useGSMClient)
  {
    Serial.println(F("# Not set credential information [ICCID, IMSI] in \"setting\""));
    Serial.println(F("# Intiailizing information board"));
    while(!GSM.begin())
    {
      Serial.println(F("GSM setup fail"));
    }
    _setting.ThingIdentifier = GSM.getICCID();
    _setting.ThingSecret = GSM.getIMSI();
    _setting.IMEI = GSM.getIMEI();
  }
  _setting.ThingIdentifier.trim();
  _setting.ThingSecret.trim();
  _setting.IMEI.trim();
  // second validate after inital Info from 4G board or user set incorrect value
  if (coreMQTT->CheckString_isDigit(_setting.ThingIdentifier) && coreMQTT->CheckString_isDigit(_setting.ThingSecret))
  {
    Serial.println(F("# ==========================="));
    Serial.println("# ICCID: "+_setting.ThingIdentifier);
    Serial.println("# IMSI : "+_setting.ThingSecret);
    if(_setting.IMEI != "null")
      Serial.println("# IMEI : "+_setting.IMEI);
    Serial.println(F("# ==========================="));
    beginCustom(_setting.ThingIdentifier, _setting.ThingSecret, _setting.IMEI, _setting.endpoint, mgPort, revertPartToBufferSize, _setting.builtInSensor);
  }
  else{
    Serial.println(F("# ICCID or IMSI invalid value please check again"));
    Serial.println("# ThingIdentifier(ICCID)=> "+_setting.ThingIdentifier);
    Serial.println("# ThingSecret(IMSI)=> "+_setting.ThingSecret);
    Serial.println(F("# ==========================="));
    Serial.println(F("# Restart board"));
    delay(5000);
    ESP.restart();
  }
}

void MAGELLAN_SIM7600E_MQTT::begin(uint16_t bufferSize, boolean builtInSensor)
{
  if(bufferSize > _default_OverBufferSize)
  {
    Serial.print(F("# You have set a buffer size greater than 8192, adjusts to: "));
    Serial.println(_default_OverBufferSize);
    this->setMQTTBufferSize(_default_OverBufferSize);   
    attr.calculate_chunkSize = _default_OverBufferSize/2;
  }
  else
  {
    this->setMQTTBufferSize(bufferSize);
    attr.calculate_chunkSize = bufferSize/2;
  }
  this->coreMQTT->begin(builtInSensor);
  coreMQTT->activeOTA(attr.calculate_chunkSize, true);
}

void MAGELLAN_SIM7600E_MQTT::begin(String _thingIden, String _thingSencret, String _imei, unsigned int Zone, uint16_t bufferSize, boolean builtinSensor)
{ 
  _thingIden.trim();
  _thingSencret.trim();
  _imei.trim();
  this->coreMQTT->begin(_thingIden, _thingSencret, _imei, Zone, bufferSize, builtinSensor);
  coreMQTT->activeOTA(attr.calculate_chunkSize, true);
}

void MAGELLAN_SIM7600E_MQTT::beginCustom(String _client_id, boolean builtinSensor, String _host, int _port, uint16_t bufferSize)
{
  this->coreMQTT->beginCustom(_client_id, builtinSensor, _host, _port, bufferSize);
  coreMQTT->activeOTA(attr.calculate_chunkSize, true);
}

void MAGELLAN_SIM7600E_MQTT::beginCustom(String _thingIden, String _thingSencret, String _imei, String _host, int _port, uint16_t bufferSize, boolean builtinSensor)
{
  _thingIden.trim();
  _thingSencret.trim();
  _imei.trim();
  String genClientID = _thingIden + "_"+String(random(1000,9999));
  this->coreMQTT->setAuthMagellan(_thingIden, _thingSencret, _imei);
  this->coreMQTT->beginCustom(genClientID, builtinSensor, _host, _port, bufferSize);
  coreMQTT->activeOTA(attr.calculate_chunkSize, true);
}

void MAGELLAN_SIM7600E_MQTT::loop()
{
  this->coreMQTT->loop();
  if(attr.flagAutoOTA)
    this->coreMQTT->handleOTA(true);
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
    attr.calculate_chunkSize = _default_OverBufferSize/2;
  }
  else
  {
    coreMQTT->setMQTTBufferSize(setBufferSize);
    attr.calculate_chunkSize = setBufferSize/2;
  }
  coreMQTT->beginCentric();
  coreMQTT->activeOTA(attr.calculate_chunkSize, true);
}

boolean MAGELLAN_SIM7600E_MQTT::isConnected()
{
  return this->coreMQTT->isConnected();
}

void MAGELLAN_SIM7600E_MQTT::Centric::begin(Setting _setting)
{
  if(_setting.clientBufferSize > _default_OverBufferSize)
  {
    Serial.print(F("# You have set a buffer size greater than 8192, adjusts to: "));
    Serial.println(_default_OverBufferSize); 
    coreMQTT->setMQTTBufferSize(_default_OverBufferSize);
    attr.calculate_chunkSize = _default_OverBufferSize/2;
  }
  else
  {
    coreMQTT->setMQTTBufferSize(_setting.clientBufferSize);
    attr.calculate_chunkSize = _setting.clientBufferSize/2;
  }

  size_t revertPartToBufferSize = attr.calculate_chunkSize * 2;
  // validate credential when user forget set ICCID and IMSI and useGSM getInfo from 4G board
  if((_setting.ThingIdentifier == "null" && _setting.ThingSecret  == "null") && attr.clientNetInterface == useGSMClient)
  {
    Serial.println(F("# Not set credential information [ICCID, IMSI]"));
    Serial.println(F("# Intiailizing information board"));
    while(!GSM.begin())
    {
      Serial.println(F("GSM setup fail"));
    }
    _setting.ThingIdentifier = GSM.getICCID();
    _setting.ThingSecret = GSM.getIMSI();
    _setting.IMEI = GSM.getIMEI();
  }
  _setting.ThingIdentifier.trim();
  _setting.ThingSecret.trim();
  _setting.IMEI.trim();

  // second validate after inital Info from 4G board or user set incorrect value
  if (coreMQTT->CheckString_isDigit(_setting.ThingIdentifier) && coreMQTT->CheckString_isDigit(_setting.ThingSecret))
  {
    coreMQTT->setAuthMagellan(_setting.ThingIdentifier, _setting.ThingSecret, _setting.IMEI);
    Serial.println(F("# ==========================="));
    Serial.println("# ICCID: "+_setting.ThingIdentifier);
    Serial.println("# IMSI : "+_setting.ThingSecret);
    if(_setting.IMEI != "null")
      Serial.println("# IMEI : "+_setting.IMEI);
    Serial.println(F("# ==========================="));
    coreMQTT->beginCentric();
    coreMQTT->activeOTA(attr.calculate_chunkSize, true);
  }
  else{
    Serial.println(F("# ICCID or IMSI invalid value please check again"));
    Serial.println("# ThingIdentifier(ICCID)=> "+_setting.ThingIdentifier);
    Serial.println("# ThingSecret(IMSI)=> "+_setting.ThingSecret);
    Serial.println(F("# ==========================="));
    Serial.println(F("# Restart board"));
    delay(5000);
    ESP.restart();
  }
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
  return coreMQTT->getIMSI();
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
    Serial.println("# add [Key] \""+sensorKey+"\" failed, this function does not allow set value \"null\"");
  }
  else{
    coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
  }

}

void MAGELLAN_SIM7600E_MQTT::Sensor::add(String sensorKey, const char* sensorValue)
{
  if(sensorValue == "null")
  {
    Serial.println("# add [Key] \""+sensorKey+"\" failed, this function does not allow set value \"null\"");
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
    Serial.println(F("# Can't sensor.report Because Not set function \" sensor.add(key,value)\" before sensor.report or Overload Memory toJSONString"));
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
    Serial.println("# add [Key] "+clientConfigKey+" failed, this function does not allow to set value \"null\"");
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
    Serial.println(F("# Can't clientConfig.save Because Not set function \" client.add(key,value)\" before clientConfig.save"));
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
  if(!attr.inProcessOTA)
  {
    coreMQTT->interval_ms(second * 1000, cb_interval);
  }
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

// OTA Feature //////
void MAGELLAN_SIM7600E_MQTT::OnTheAir::begin()
{
  if(attr.calculate_chunkSize > 4096)
  {
    Serial.print(F("#[Warning] activeOTA can't set part size: "));
    Serial.print(attr.calculate_chunkSize);
    Serial.print(F(" Part size Maximum is 4096 adjust part size to: "));
    attr.calculate_chunkSize = 4096;
    Serial.println(attr.calculate_chunkSize);
    coreMQTT->activeOTA(attr.calculate_chunkSize, true);
  }
  else
  {
    coreMQTT->activeOTA(attr.calculate_chunkSize, true);
  }
}

void MAGELLAN_SIM7600E_MQTT::OnTheAir::handle(boolean OTA_after_getInfo)
{
  coreMQTT->handleOTA(OTA_after_getInfo);
}

void MAGELLAN_SIM7600E_MQTT::OnTheAir::setChecksum(String md5Checksum)
{
  coreMQTT->setChecksum(md5Checksum);
}

boolean MAGELLAN_SIM7600E_MQTT::OnTheAir::Subscribe::firmwareInfo()
{
  return coreMQTT->registerInfoOTA();
}

boolean MAGELLAN_SIM7600E_MQTT::OnTheAir::Subscribe::firmwareDownload()
{
  return coreMQTT->registerDownloadOTA();
}
boolean MAGELLAN_SIM7600E_MQTT::OnTheAir::Unsubscribe::firmwareInfo()
{
  return coreMQTT->unregisterInfoOTA();
}

boolean MAGELLAN_SIM7600E_MQTT::OnTheAir::Unsubscribe::firmwareDownload()
{
  return coreMQTT->unregisterDownloadOTA();
}

boolean MAGELLAN_SIM7600E_MQTT::OnTheAir::getFirmwareInfo()
{
  return coreMQTT->requestFW_Info();
}

boolean  MAGELLAN_SIM7600E_MQTT::OnTheAir::updateProgress(String FOTAstate, String description)
{
  return coreMQTT->updateProgressOTA(FOTAstate, description);
}


boolean flag_startOTA = false;
boolean MAGELLAN_SIM7600E_MQTT::OnTheAir::downloadFirmware(unsigned int fw_part, size_t part_size)
{
  boolean statusDL = false;

  if(coreMQTT->OTA_info.firmwareTotalSize <= 0 && !coreMQTT->OTA_info.isReadyOTA)
  {
    Serial.println(F("# [Warning] Can't downloadFirmware"));
    Serial.println(F("# Don't have firmware OTA information in save or the thing don't have firmware OTA"));
    Serial.println(F("# Make sure you get firmware Information first"));
    statusDL = false;
  }
  else{
    if(part_size <= 0)
    {
      statusDL = coreMQTT->requestFW_Download(fw_part, attr.calculate_chunkSize);
    }
    else
    {
      statusDL = coreMQTT->requestFW_Download(fw_part, part_size);
    }
    flag_startOTA = true;
  }
  return statusDL;
}
OTA_INFO MAGELLAN_SIM7600E_MQTT::OnTheAir::utility()
{
  return coreMQTT->OTA_info;
}


boolean exc_until_info_fwReady = true;
int MaxIfUnknownVersion = 15;
int countIfUnknownVersion = 0;
unsigned long exc_prvMillis = 0;
void MAGELLAN_SIM7600E_MQTT::OnTheAir::executeUpdate()
{
  OTA_info.firmwareIsUpToDate = UNKNOWN; // back to Unknown for recieve new firmware status
  
  if(!exc_until_info_fwReady)
  {
    Serial.println(F("# Dubug protect debounce spam function execute"));
    return;
  }
  countIfUnknownVersion = 0;
  exc_until_info_fwReady = false;
  attr.usingCheckUpdate = false;
  Serial.println(F("# Execute Update!!!"));
  coreMQTT->registerDownloadOTA();
  coreMQTT->registerInfoOTA();
  attr.flagAutoOTA = true;

  while(true)
  {
    coreMQTT->loop();
    coreMQTT->handleOTA(true);
    if(millis() - exc_prvMillis > 5000 && !exc_until_info_fwReady)  // get fw info every 5sec until fwReady
    {  

      exc_prvMillis = millis();
      if (OTA_info.firmwareIsUpToDate == UNKNOWN)
      {
        countIfUnknownVersion++;
        attr.usingCheckUpdate = false;
        coreMQTT->requestFW_Info(); //getFirmwareInfo
        if(countIfUnknownVersion > MaxIfUnknownVersion)
        {
          Serial.println(F(""));
          Serial.println(F("# ====================================="));
          Serial.println(F("# No response from request firmware information"));
          Serial.println(F("# ====================================="));
          Serial.println(F(""));
          countIfUnknownVersion = 0;
          if(attr.isBypassAutoUpdate)
          {
            attr.flagAutoOTA = false;
          }
          else
          {
            attr.flagAutoOTA = true;
          }
          exc_until_info_fwReady = true;
          break;
        }
      }

      else if(OTA_info.firmwareIsUpToDate == UP_TO_DATE)
      {
        Serial.println(F(""));
        Serial.println(F("# ====================================="));
        Serial.println(F("# Firmware is up to date execute cancel"));
        Serial.println(F("# ====================================="));
        Serial.println(F(""));
        exc_until_info_fwReady = true;
        if(attr.isBypassAutoUpdate){
          attr.flagAutoOTA = false;
        }
        else{
          attr.flagAutoOTA = true;
        }
        break;
      }
      
      else if (OTA_info.firmwareIsUpToDate == OUT_OF_DATE )
      {
        exc_until_info_fwReady = attr.startReqDownloadOTA;
        if(!attr.inProcessOTA)
        {
          Serial.println(F(""));
          Serial.println(F("# ====================================="));
          Serial.println(F("# Execute start"));
          Serial.println(F("# ====================================="));
          Serial.println(F(""));
        }
      }
    }
    if(OTA_info.firmwareIsUpToDate == UP_TO_DATE && exc_until_info_fwReady) 
    {
      exc_until_info_fwReady = true;
      Serial.println(F("# Debug Uptodate but infinity loop [UP_TO_DATE]"));
      break;
    }  
    else if(OTA_info.firmwareIsUpToDate == UNKNOWN && exc_until_info_fwReady) 
    {
      exc_until_info_fwReady = true;
      Serial.println(F("# Debug Uptodate but infinity loop [UNKNOWN]"));
      break;
    } 
    if(!coreMQTT->isConnected())
    {
      exc_until_info_fwReady = true;
      Serial.println(F("# Debug client disconnect while OTA"));
      break;
    }
  }
}
void MAGELLAN_SIM7600E_MQTT::OnTheAir::autoUpdate(boolean flagSetAuto)
{
  attr.flagAutoOTA = flagSetAuto;
  if(!attr.flagAutoOTA)
  {
    attr.isBypassAutoUpdate = true;
    coreMQTT->unregisterDownloadOTA();
  }
  else{
    attr.isBypassAutoUpdate = false;
    coreMQTT->registerDownloadOTA();
  }
  Serial.println("# Set auto update mode: " + String((attr.flagAutoOTA == true) ? "ENABLE" : "DISABLE"));
}

boolean MAGELLAN_SIM7600E_MQTT::OnTheAir::getAutoUpdate()
{
  // boolean mode = attr.flagAutoOTA;
  // Serial.print(F("# Auto update mode: "));
  // Serial.println((mode)? "ENABLE":"DISABLE");
  return attr.flagAutoOTA;
}

int maxCheckUpdate = 10;
int countCheckUpdate = 0;
boolean checkUntil_end = false;
unsigned long check_prvMillis = 0;
unsigned long diff_timeMillis = 0;
int MAGELLAN_SIM7600E_MQTT::OnTheAir::checkUpdate()
{   
  if(attr.usingCheckUpdate)
  {
    Serial.println(F("# Debug protect debounce using checkUpdate"));
    return coreMQTT->OTA_info.firmwareIsUpToDate;
  }
  Serial.println(F("# Check Update"));
  Serial.println(F("# Waiting for response"));
  coreMQTT->OTA_info.firmwareIsUpToDate = UNKNOWN;
  checkUntil_end = false;
  attr.usingCheckUpdate = true;
  countCheckUpdate = 0;
  check_prvMillis = millis();

  while(true)
  {
    coreMQTT->loop();
    coreMQTT->handleOTA(false);
    diff_timeMillis = millis() - check_prvMillis;
    if(diff_timeMillis > 3000 && !checkUntil_end)  // get fw info every 5sec until fwReady
    {
      if (coreMQTT->OTA_info.firmwareIsUpToDate == UNKNOWN)
      {
        attr.usingCheckUpdate = true;
        coreMQTT->requestFW_Info();
        countCheckUpdate++;
        if(countCheckUpdate > maxCheckUpdate)
        {         
          checkUntil_end = true;
          Serial.println(F(""));
          Serial.println(F("# ====================================="));
          Serial.println(F("# No response from request firmware information"));
          Serial.println(F("# ====================================="));
          Serial.println(F(""));
          countCheckUpdate = 0;
          break;
        }
      }
      else if(coreMQTT->OTA_info.firmwareIsUpToDate == UP_TO_DATE)
      {
        countCheckUpdate = 0;
        // checkUntil_end = true;
        // break;
      }
      else if (coreMQTT->OTA_info.firmwareIsUpToDate == OUT_OF_DATE )
      {
        countCheckUpdate = 0;
        // checkUntil_end = true;
        // break;
      }
      if(!attr.usingCheckUpdate)
      {
        checkUntil_end = true;
        Serial.println(F("# ====================================="));
        Serial.println(F("# Debug Timeout check update when get response"));
        Serial.println(F("# Debug already get response"));
        Serial.println(F("# ====================================="));
        break;
      }
      check_prvMillis = millis();
    }
    if(!attr.usingCheckUpdate && checkUntil_end)
    {
      checkUntil_end = true;
      Serial.println(F("# ====================================="));
      Serial.println(F("# Debug Timeout when loop infinity from spam"));
      Serial.println(F("# ====================================="));
      break;
    }
  }
  return coreMQTT->OTA_info.firmwareIsUpToDate;
}

String MAGELLAN_SIM7600E_MQTT::OnTheAir::readDeviceInfo()
{
  return configOTAFile.readLastedOTA();
}

boolean MAGELLAN_SIM7600E_MQTT::OnTheAir::start()
{
  if(!flag_startOTA)
  {
    Serial.println(F("# Start OTA!"));
    return downloadFirmware(0, attr.calculate_chunkSize);
  }
  return false;
}
/////////////////////
  String MAGELLAN_SIM7600E_MQTT::Utility::toDateTimeString(unsigned long unixtTime, int timeZone)
  {
    return utls.toDateTimeString(unixtTime, timeZone);
  }
  String MAGELLAN_SIM7600E_MQTT::Utility::toUniversalTime(unsigned long unixtTime, int timeZone)
  {
    return utls.toUniversalTime(unixtTime, timeZone);
  }
  unsigned long MAGELLAN_SIM7600E_MQTT::Utility::toUnix(tm time_)
  {
    return utls.toUnix(time_);
  }
  tm MAGELLAN_SIM7600E_MQTT::Utility::convertUnix(unsigned long unix, int timeZone)
  {
    return utls.convertUnix(unix, timeZone);
  } 

/////////////////////
