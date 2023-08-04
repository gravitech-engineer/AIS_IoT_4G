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
support SIMCOM SIM7600E(AIS 4G Board)

Author:(POC Device Magellan team)
Create Date: 25 April 2022.
Modified: 22 may 2023.
*/

#include "MAGELLAN_SIM7600E_MQTT.h"

void adjust_BufferForMedia(size_t len_payload);
void checkLimitationPayload(size_t len_payload, size_t limitation);
struct JsonDocUtils
{
  size_t used;
  size_t max_size;
  size_t safety_size;
};
JsonDocUtils readSafetyCapacity_Json_doc(JsonDocument &ref_docs);

MAGELLAN_MQTT_device_core *MAGELLAN_SIM7600E_MQTT::coreMQTT = NULL;
Setting setting;
MAGELLAN_SIM7600E_MQTT::MAGELLAN_SIM7600E_MQTT()
{
  this->coreMQTT = new MAGELLAN_MQTT_device_core();
}

MAGELLAN_SIM7600E_MQTT::MAGELLAN_SIM7600E_MQTT(Client &client)
{
  this->coreMQTT = new MAGELLAN_MQTT_device_core(client);
}
void MAGELLAN_SIM7600E_MQTT::begin(Setting _setting)
{
  if (_setting.clientBufferSize > _default_OverBufferSize)
  {
    Serial.print(F("# You have set a buffer size greater than 8192, adjusts to: "));
    Serial.println(_default_OverBufferSize);
    this->setMQTTBufferSize(_default_OverBufferSize);
    attr.calculate_chunkSize = _default_OverBufferSize / 2;
  }
  else
  {
    this->setMQTTBufferSize(_setting.clientBufferSize);
    attr.calculate_chunkSize = _setting.clientBufferSize / 2;
  }

  size_t revertPartToBufferSize = attr.calculate_chunkSize * 2;
  // validate credential when user forget set ICCID and IMSI and useGSM getInfo from 4G board
  if ((_setting.ThingIdentifier == "null" && _setting.ThingSecret == "null") && attr.clientNetInterface == useGSMClient)
  {
    Serial.println(F("# Not set credential information [ICCID, IMSI] in \"setting\""));
    Serial.println(F("# Intiailizing information board"));
    while (!GSM.begin())
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
    Serial.println("# ICCID: " + _setting.ThingIdentifier);
    Serial.println("# IMSI : " + _setting.ThingSecret);
    if (_setting.IMEI != "null")
      Serial.println("# IMEI : " + _setting.IMEI);
    Serial.println(F("# ==========================="));
    beginCustom(_setting.ThingIdentifier, _setting.ThingSecret, _setting.IMEI, _setting.endpoint, mgPort, revertPartToBufferSize, _setting.builtInSensor);
  }
  else
  {
    Serial.println(F("# ICCID or IMSI invalid value please check again"));
    Serial.println("# ThingIdentifier(ICCID)=> " + _setting.ThingIdentifier);
    Serial.println("# ThingSecret(IMSI)=> " + _setting.ThingSecret);
    Serial.println(F("# ==========================="));
    Serial.println(F("# Restart board"));
    delay(5000);
    ESP.restart();
  }
}

void MAGELLAN_SIM7600E_MQTT::begin(uint16_t bufferSize, boolean builtInSensor)
{
  if (bufferSize > _default_OverBufferSize)
  {
    Serial.print(F("# You have set a buffer size greater than 8192, adjusts to: "));
    Serial.println(_default_OverBufferSize);
    this->setMQTTBufferSize(_default_OverBufferSize);
    attr.calculate_chunkSize = _default_OverBufferSize / 2;
  }
  else
  {
    this->setMQTTBufferSize(bufferSize);
    attr.calculate_chunkSize = bufferSize / 2;
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
  String genClientID = _thingIden + "_" + String(random(1000, 9999));
  this->coreMQTT->setAuthMagellan(_thingIden, _thingSencret, _imei);
  this->coreMQTT->beginCustom(genClientID, builtinSensor, _host, _port, bufferSize);
  coreMQTT->activeOTA(attr.calculate_chunkSize, true);
}

void MAGELLAN_SIM7600E_MQTT::loop()
{
  this->coreMQTT->loop();
  if (attr.flagAutoOTA)
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
  if (setBufferSize > _default_OverBufferSize)
  {
    Serial.print(F("# You have set a buffer size greater than 8192, adjusts to: "));
    Serial.println(_default_OverBufferSize);
    coreMQTT->setMQTTBufferSize(_default_OverBufferSize);
    attr.calculate_chunkSize = _default_OverBufferSize / 2;
  }
  else
  {
    coreMQTT->setMQTTBufferSize(setBufferSize);
    attr.calculate_chunkSize = setBufferSize / 2;
  }
  coreMQTT->beginCentric();
  coreMQTT->activeOTA(attr.calculate_chunkSize, true);
  // mConf_OTA.beginFileSystem(true);
}

boolean MAGELLAN_SIM7600E_MQTT::isConnected()
{
  return this->coreMQTT->isConnected();
}

void MAGELLAN_SIM7600E_MQTT::Centric::begin(Setting _setting)
{
  if (_setting.clientBufferSize > _default_OverBufferSize)
  {
    Serial.print(F("# You have set a buffer size greater than 8192, adjusts to: "));
    Serial.println(_default_OverBufferSize);
    coreMQTT->setMQTTBufferSize(_default_OverBufferSize);
    attr.calculate_chunkSize = _default_OverBufferSize / 2;
  }
  else
  {
    coreMQTT->setMQTTBufferSize(_setting.clientBufferSize);
    attr.calculate_chunkSize = _setting.clientBufferSize / 2;
  }

  size_t revertPartToBufferSize = attr.calculate_chunkSize * 2;
  // validate credential when user forget set ICCID and IMSI and useGSM getInfo from 4G board
  if ((_setting.ThingIdentifier == "null" && _setting.ThingSecret == "null") && attr.clientNetInterface == useGSMClient)
  {
    Serial.println(F("# Not set credential information [ICCID, IMSI]"));
    Serial.println(F("# Intiailizing information board"));
    while (!GSM.begin())
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
    Serial.println("# ICCID: " + _setting.ThingIdentifier);
    Serial.println("# IMSI : " + _setting.ThingSecret);
    if (_setting.IMEI != "null")
      Serial.println("# IMEI : " + _setting.IMEI);
    Serial.println(F("# ==========================="));
    coreMQTT->beginCentric();
    coreMQTT->activeOTA(attr.calculate_chunkSize, true);
    // mConf_OTA.beginFileSystem(true);
  }
  else
  {
    Serial.println(F("# ICCID or IMSI invalid value please check again"));
    Serial.println("# ThingIdentifier(ICCID)=> " + _setting.ThingIdentifier);
    Serial.println("# ThingSecret(IMSI)=> " + _setting.ThingSecret);
    Serial.println(F("# ==========================="));
    Serial.println(F("# Restart board"));
    delay(5000);
    ESP.restart();
  }
}

boolean MAGELLAN_SIM7600E_MQTT::Report::send(String payload)
{
  int len = payload.length();
  size_t cal_limit = 1500 - 66; // max(1500) - topic
  checkLimitationPayload(len, cal_limit);
  adjust_BufferForMedia(len + 2000);
  return coreMQTT->report(payload);
}

boolean MAGELLAN_SIM7600E_MQTT::Report::send(String key, String value)
{
  int len = value.length();
  size_t cal_limit = 1500 - (79 + key.length()); // max(1500) - topic + key
  checkLimitationPayload(len, cal_limit);
  adjust_BufferForMedia(len + 2000);
  return coreMQTT->report(key, value);
}

boolean MAGELLAN_SIM7600E_MQTT::Report::send(int UnixtsTimstamp, String payload)
{
  int len = payload.length();
  size_t cal_limit = 1500 - (76 + 10); // max(1500) - topic + len Unixtime
  checkLimitationPayload(len, cal_limit);
  adjust_BufferForMedia(len + 2000);
  String u_timestamp = String(UnixtsTimstamp);
  return coreMQTT->reportTimestamp(u_timestamp, payload, SET_UNIXTS);
}

// ver.1.1.2

boolean MAGELLAN_SIM7600E_MQTT::Report::send(String payload, int msgId) // 1.1.2
{
  ResultReport result;
  int len = payload.length();
  adjust_BufferForMedia(len + 2000);
  result = this->sendWithMsgId(payload, msgId);
  return result.statusReport;
}

boolean MAGELLAN_SIM7600E_MQTT::Report::send(String key, String value, int msgId) // 1.1.2
{
  ResultReport result;
  int len = value.length();
  adjust_BufferForMedia(len + 2000);
  result = sendWithMsgId(key, value, msgId);
  return result.statusReport;
}

ResultReport MAGELLAN_SIM7600E_MQTT::Report::send(String payload, RetransmitSetting &retrans)
{
  ResultReport result;
  if (retrans.enabled)
  {
    if (retrans.msgId > 0)
    {
      result = this->sendRetransmit(payload, retrans);
    }
    else
    {
      retrans.msgId = this->generateMsgId();
      result = this->sendRetransmit(payload, retrans);
    }
    return result;
  }
  else // disable retransmit
  {
    if (retrans.msgId > 0)
    {
      result.statusReport = this->send(payload, retrans.msgId);
      result.msgId = retrans.msgId;
    }
    else
    {
      retrans.msgId = this->generateMsgId();
      result.statusReport = this->send(payload);
      result.msgId = retrans.msgId;
    }
    return result;
  }
  return result;
}

ResultReport MAGELLAN_SIM7600E_MQTT::Report::send(String reportKey, String reportValue, RetransmitSetting &retrans)
{
  ResultReport result;
  if (retrans.enabled)
  {
    if (retrans.msgId > 0)
    {
      result = this->sendRetransmit(reportKey, reportValue, retrans);
      return result;
    }
    else
    {
      retrans.msgId = this->generateMsgId();
      result = this->sendRetransmit(reportKey, reportValue, retrans);
      return result;
    }
  }
  else // disable retransmit
  {
    if (retrans.msgId > 0)
    {
      result.statusReport = this->send(reportKey, reportValue, retrans.msgId);
      result.msgId = retrans.msgId;
      return result;
    }
    else
    {
      retrans.msgId = this->generateMsgId();
      result.statusReport = this->send(reportKey, reportValue, retrans.msgId);
      result.msgId = retrans.msgId;
      return result;
    }
  }
  return result;
}

int MAGELLAN_SIM7600E_MQTT::Report::generateMsgId()
{
  return (int)random(9999, 9999999);
}

ResultReport MAGELLAN_SIM7600E_MQTT::Report::sendWithMsgId(String payload, int msgId)
{
  int len = payload.length();
  String _topic = "api/v2/thing/" + attr.ext_Token + "/report/persist/?id=" + String(msgId);
  ResultReport internalResult;
  size_t cal_limit = 1500 - _topic.length(); // max(1500) - topic
  checkLimitationPayload(len, cal_limit);
  adjust_BufferForMedia(len + 2000);
  bool result = attr.mqtt_client->publish(_topic.c_str(), payload.c_str());
  internalResult.statusReport = result;
  internalResult.msgId = msgId;
  String _debug = (result == true) ? "Success" : "Failure";
  Serial.println(F("-------------------------------"));
  Serial.println("# Report JSON with MsgId: " + String(msgId) + " is " + _debug);
  Serial.println("# [Sensors]: " + payload);
  return internalResult;
}
ResultReport MAGELLAN_SIM7600E_MQTT::Report::sendWithMsgId(String reportKey, String reportValue, int msgId)
{
  int len = reportValue.length();

  String _topic = "api/v2/thing/" + attr.ext_Token + "/report/persist/pta/?sensor=" + reportKey + "&id=" + String(msgId);
  ResultReport internalResult;
  size_t cal_limit = 1500 - (_topic.length()); // max(1500) - topic
  checkLimitationPayload(len, cal_limit);
  adjust_BufferForMedia(len + 2000);
  bool result = attr.mqtt_client->publish(_topic.c_str(), reportValue.c_str());
  internalResult.statusReport = result;
  internalResult.msgId = msgId;
  String _debug = (result == true) ? "Success" : "Failure";
  Serial.println(F("-------------------------------"));
  Serial.println("# Report Plaintext with MsgId: " + String(msgId) + " is " + _debug);
  Serial.println("# [Report value]: " + reportValue);
  return internalResult;
}

ResultReport MAGELLAN_SIM7600E_MQTT::Report::sendWithMsgId(String payload)
{
  int randomID = generateMsgId();
  return this->sendWithMsgId(payload, randomID);
}

ResultReport MAGELLAN_SIM7600E_MQTT::Report::sendWithMsgId(String reportKey, String reportValue)
{
  int randomID = generateMsgId();
  return this->sendWithMsgId(reportKey, reportValue, randomID);
}

unsigned long prev_millis_retrans = 0;
unsigned long prev_millis_timeout = 0;

ResultReport MAGELLAN_SIM7600E_MQTT::Report::sendRetransmit(String payload, RetransmitSetting retrans)
{
  attr.reqRetransmit = true;
  ResultReport result;
  int countRetransmit = 0;
  attr.matchMsgId_send = retrans.msgId;
  int Timeout = ((retrans.repeat * retrans.duration) + 2) * 1000;
  prev_millis_timeout = millis();
  while (true)
  {
    coreMQTT->loop();
    coreMQTT->registerList(duplicate_subs_list);
    if (millis() - prev_millis_retrans > (retrans.duration * 1000) && countRetransmit < (retrans.repeat + 1))
    {
      result = this->sendWithMsgId(payload, retrans.msgId);
      if (countRetransmit > 0)
      {
         Serial.print(F("\n#Retransmit count: "));
        Serial.print(countRetransmit);
        Serial.print(F(" on MsgId: "));
        Serial.println(retrans.msgId);
      }
      countRetransmit++;
      if (countRetransmit > retrans.repeat)
      {
        Serial.print(F("\n# Report retransmit fail timeout on MsgId: "));
        Serial.println(attr.matchMsgId_send);
        attr.reqRetransmit = false;
        break;
      }
      prev_millis_retrans = millis();
    }
    if (attr.isMatchMsgId)
    {
      Serial.print(F("# Finished report transmission MsgId: "));
      Serial.println(attr.matchMsgId_send);
      result.msgId = attr.matchMsgId_send; // assign msgId
      result.statusReport = true;

      attr.reqRetransmit = false;
      attr.isMatchMsgId = false;
      attr.matchMsgId_cb = -1;
      attr.matchMsgId_send = -1;
      break;
    }
    if (!coreMQTT->isConnected())
    {
      Serial.print(F("\n# Report retransmit fail connection lost on MsgId: "));
      Serial.println(attr.matchMsgId_send);
      break;
    }
    if (millis() - prev_millis_timeout > Timeout)
    {
      prev_millis_timeout = millis();
      Serial.print(F("\n# Triger timeout from send retransmit"));
      result.msgId = attr.matchMsgId_send; // assign msgId
      result.statusReport = false;
      attr.reqRetransmit = false;
      break;
    }
    if (attr.inProcessOTA && attr.reqRetransmit)
    {
      Serial.print(F("\n# In procress OTA cancel report with report retransmit"));
      result.msgId = attr.matchMsgId_send; // assign msgId
      result.statusReport = false;
      attr.reqRetransmit = false;
      break;
    }
  }
  return result;
}

ResultReport MAGELLAN_SIM7600E_MQTT::Report::sendRetransmit(String reportKey, String reportValue, RetransmitSetting retrans)
{
  attr.reqRetransmit = true;
  ResultReport result;
  int countRetransmit = 0;
  attr.matchMsgId_send = retrans.msgId;
  int Timeout = ((retrans.repeat * retrans.duration) + 2) * 1000;
  prev_millis_timeout = millis();
  while (true)
  {
    coreMQTT->loop();
    coreMQTT->registerList(duplicate_subs_list);
    if (millis() - prev_millis_retrans > (retrans.duration * 1000) && countRetransmit < (retrans.repeat + 1))
    {
      result = this->sendWithMsgId(reportKey, reportValue, retrans.msgId);
      if (countRetransmit > 0)
      {
        Serial.print(F("\n#Retransmit count: "));
        Serial.print(countRetransmit);
        Serial.print(F(" on MsgId: "));
        Serial.println(retrans.msgId);
      }
      countRetransmit++;
      if (countRetransmit > retrans.repeat)
      {
        Serial.print(F("\n# Report retransmit fail timeout on MsgId: "));
        Serial.println(attr.matchMsgId_send);
        attr.reqRetransmit = false;
        break;
      }
      prev_millis_retrans = millis();
    }
    if (attr.isMatchMsgId)
    {
      Serial.print(F("# Finished report transmission MsgId: "));
      Serial.println(attr.matchMsgId_send);
      result.msgId = attr.matchMsgId_send; // assign msgId
      result.statusReport = true;

      attr.reqRetransmit = false;
      attr.isMatchMsgId = false;
      attr.matchMsgId_cb = -1;
      attr.matchMsgId_send = -1;
      break;
    }
    if (!coreMQTT->isConnected())
    {
      Serial.print(F("\n# Report retransmit fail connection lost on MsgId: "));
      Serial.println(attr.matchMsgId_send);
      break;
    }
    if (millis() - prev_millis_timeout > Timeout)
    {
      prev_millis_timeout = millis();
      Serial.print(F("\n# Triger timeout from send retransmit"));
      result.msgId = attr.matchMsgId_send; // assign msgId
      result.statusReport = false;
      attr.reqRetransmit = false;
      break;
    }
    if (attr.inProcessOTA && attr.reqRetransmit)
    {
      Serial.print(F("\n# In procress OTA cancel report with report retransmit"));
      result.msgId = attr.matchMsgId_send; // assign msgId
      result.statusReport = false;
      attr.reqRetransmit = false;
      break;
    }
  }
  return result;
}

ResultReport MAGELLAN_SIM7600E_MQTT::Report::sendRetransmit(String payload)
{
  int randomID = this->generateMsgId();
  RetransmitSetting retrans;
  retrans.msgId = randomID;
  ResultReport result = sendRetransmit(payload, retrans);
  return result;
}

ResultReport MAGELLAN_SIM7600E_MQTT::Report::sendRetransmit(String reportKey, String reportValue)
{
  int randomID = this->generateMsgId();
  RetransmitSetting retrans;
  retrans.msgId = randomID;
  ResultReport result = sendRetransmit(reportKey, reportValue, retrans);
  return result;
}

boolean MAGELLAN_SIM7600E_MQTT::matchingMsgId(int sendingMsgId, int incomingMsgId)
{
  if (sendingMsgId == incomingMsgId)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/// @details v1.1.2

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

void MAGELLAN_SIM7600E_MQTT::GPSmodule::setLocalTimeZone(int timeZone)
{
  mySensor.setLocalTimeZone(timeZone);
}

int MAGELLAN_SIM7600E_MQTT::GPSmodule::getDay()
{
  return mySensor.getDay();
}

String MAGELLAN_SIM7600E_MQTT::GPSmodule::getDayToString()
{
  return mySensor.getDayToString();
}

int MAGELLAN_SIM7600E_MQTT::GPSmodule::getMonth()
{
  return mySensor.getMonth();
}

String MAGELLAN_SIM7600E_MQTT::GPSmodule::getMonthToString()
{
  return mySensor.getMonthToString();
}

int MAGELLAN_SIM7600E_MQTT::GPSmodule::getYear()
{
  return mySensor.getYear();
}

String MAGELLAN_SIM7600E_MQTT::GPSmodule::getYearToString()
{
  return mySensor.getYearToString();
}

int MAGELLAN_SIM7600E_MQTT::GPSmodule::getHour()
{
  return mySensor.getHour();
}

String MAGELLAN_SIM7600E_MQTT::GPSmodule::getHourToString()
{
  return mySensor.getHourToString();
}

int MAGELLAN_SIM7600E_MQTT::GPSmodule::getMinute()
{
  return mySensor.getMinute();
}

String MAGELLAN_SIM7600E_MQTT::GPSmodule::getMinuteToString()
{
  return mySensor.getMinuteToString();
}

int MAGELLAN_SIM7600E_MQTT::GPSmodule::getSecond()
{
  return mySensor.getSecond();
}

String MAGELLAN_SIM7600E_MQTT::GPSmodule::getSecondToString()
{
  return mySensor.getSecondToString();
}

String MAGELLAN_SIM7600E_MQTT::GPSmodule::getDateTimeString()
{
  return mySensor.getDateTimeString();
}

String MAGELLAN_SIM7600E_MQTT::GPSmodule::getUniversalTime()
{
  return mySensor.getUniversalTime();
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
  JsonDocUtils validateJSON_doc = readSafetyCapacity_Json_doc(*attr.docSensor);
  if (sensorValue == "null")
  {
    Serial.println("# add [Key] \"" + sensorKey + "\" failed, this function does not allow set value \"null\"");
    return;
  }
  else if (validateJSON_doc.used > validateJSON_doc.safety_size * 0.8f)
  {
    String bufJSON = this->toJSONString();
    // attr.docSensor->clear();
    attr.docSensor = new DynamicJsonDocument(validateJSON_doc.max_size + 2048); // offset size
    deserializeJson(*attr.docSensor, bufJSON);
    Serial.println("# add [Key] \"" + sensorKey + "\" JsonBuffer is full adjust to: " + String(coreMQTT->readBufferSensor(*attr.docSensor)));
    coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
  }
  else if (sensorValue.length() > 10000 && coreMQTT->readBufferSensor(*attr.docSensor) < sensorValue.length())
  {
    Serial.println(F("# Preparing large data to JSONbuffer"));
    String bufJSON = this->toJSONString();
    // attr.docSensor->clear();
    attr.docSensor = new DynamicJsonDocument(sensorValue.length() + bufJSON.length() + 3000); // offset size
    deserializeJson(*attr.docSensor, bufJSON);
    Serial.println("# add [Key] \"" + sensorKey + "\" JsonBuffer is full adjust to: " + String(coreMQTT->readBufferSensor(*attr.docSensor)));
    coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
  }
  else
  {
    coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::add(String sensorKey, const char *sensorValue)
{
  JsonDocUtils validateJSON_doc = readSafetyCapacity_Json_doc(*attr.docSensor);
  if (sensorValue == "null")
  {
    Serial.println("# add [Key] \"" + sensorKey + "\" failed, this function does not allow set value \"null\"");
    return;
  }
  else if (validateJSON_doc.used > validateJSON_doc.safety_size * 0.8f)
  {
    String bufJSON = this->toJSONString();
    // attr.docSensor->clear();
    attr.docSensor = new DynamicJsonDocument(validateJSON_doc.max_size + 2048); // offset size

    Serial.println("# add [Key] \"" + sensorKey + "\" JsonBuffer is full adjust to: " + String(coreMQTT->readBufferSensor(*attr.docSensor)));
    deserializeJson(*attr.docSensor, bufJSON);
    coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
  }
  else if (strlen(sensorValue) > 10000 && strlen(sensorValue) > coreMQTT->readBufferSensor(*attr.docSensor))
  {
    Serial.println(F("# Preparing large data to JSONbuffer"));
    String bufJSON = this->toJSONString();
    // attr.docSensor->clear();
    attr.docSensor = new DynamicJsonDocument(strlen(sensorValue) + bufJSON.length() + 3000); // offset size
    deserializeJson(*attr.docSensor, bufJSON);
    Serial.println("# add [Key] \"" + sensorKey + "\" JsonBuffer is full adjust to: " + String(coreMQTT->readBufferSensor(*attr.docSensor)));
    coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
  }
  else
  {
    coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::add(String sensorKey, int sensorValue)
{
  JsonDocUtils validateJSON_doc = readSafetyCapacity_Json_doc(*attr.docSensor);
  if (validateJSON_doc.used > validateJSON_doc.safety_size * 0.8f)
  {
    String bufJSON = this->toJSONString();
    // attr.docSensor->clear();
    attr.docSensor = new DynamicJsonDocument(validateJSON_doc.max_size + 2048); // offset size
    deserializeJson(*attr.docSensor, bufJSON);
    Serial.println("# add [Key] \"" + sensorKey + "\" JsonBuffer is full adjust to: " + String(coreMQTT->readBufferSensor(*attr.docSensor)));
    coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
  }
  else
  {
    coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::add(String sensorKey, float sensorValue)
{
  JsonDocUtils validateJSON_doc = readSafetyCapacity_Json_doc(*attr.docSensor);
  if (validateJSON_doc.used > validateJSON_doc.safety_size * 0.8f)
  {
    String bufJSON = this->toJSONString();
    // attr.docSensor->clear();
    attr.docSensor = new DynamicJsonDocument(validateJSON_doc.max_size + 2048); // offset size
    deserializeJson(*attr.docSensor, bufJSON);
    Serial.println("# add [Key] \"" + sensorKey + "\" JsonBuffer is full adjust to: " + String(coreMQTT->readBufferSensor(*attr.docSensor)));
    coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
  }
  else
  {
    coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::add(String sensorKey, boolean sensorValue)
{
  JsonDocUtils validateJSON_doc = readSafetyCapacity_Json_doc(*attr.docSensor);
  if (validateJSON_doc.used > validateJSON_doc.safety_size * 0.8f)
  {
    String bufJSON = this->toJSONString();
    // attr.docSensor->clear();
    attr.docSensor = new DynamicJsonDocument(validateJSON_doc.max_size + 2048); // offset size
    deserializeJson(*attr.docSensor, bufJSON);
    Serial.println("# add [Key] \"" + sensorKey + "\" JsonBuffer is full adjust to: " + String(coreMQTT->readBufferSensor(*attr.docSensor)));
    coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
  }
  else
  {
    coreMQTT->addSensor(sensorKey, sensorValue, *attr.docSensor);
  }
}

String MAGELLAN_SIM7600E_MQTT::Sensor::toJSONString()
{
  return coreMQTT->buildSensorJSON(*attr.docSensor);
}

void MAGELLAN_SIM7600E_MQTT::Sensor::report()
{
  String bufferPlayload = coreMQTT->buildSensorJSON(*attr.docSensor);
  int len = bufferPlayload.length();
  if (bufferPlayload.indexOf("null") == -1)
  {
    if (bufferPlayload.indexOf("null") == -1 && len < attr.max_payload_report)
    {
      adjust_BufferForMedia(len);
      size_t cal_limit = 1500 - 66; // max(1500) - topic
      checkLimitationPayload(len, cal_limit);
      coreMQTT->report(bufferPlayload);
      coreMQTT->clearSensorBuffer(*attr.docSensor);
    }
    else if (bufferPlayload.indexOf("null") == -1 && len > attr.max_payload_report)
    {
      Serial.println("# [ERORR] payload length : " + String(len));
      Serial.println("# [ERORR] Sensor.report() Failed payload is geather than: " + String(attr.max_payload_report));
      coreMQTT->clearSensorBuffer(*attr.docSensor);
      return;
    }
  }
  else
  {
    Serial.println(F("# Can't sensor.report Because Not set function \" sensor.add(key,value)\" before sensor.report or Overload Memory toJSONString"));
  }
}

// ver 1.1.2
ResultReport MAGELLAN_SIM7600E_MQTT::Sensor::report(RetransmitSetting &retrans)
{
  String bufferPlayload = coreMQTT->buildSensorJSON(*attr.docSensor);
  int len = bufferPlayload.length();
  ResultReport result;
  Report report;
  if (retrans.enabled)
  {
    if (retrans.msgId > 0)
    {
      if (bufferPlayload.indexOf("null") == -1)
      {
        if (bufferPlayload.indexOf("null") == -1 && len < attr.max_payload_report)
        {
          adjust_BufferForMedia(len);
          result = report.send(bufferPlayload, retrans);
          coreMQTT->clearSensorBuffer(*attr.docSensor);
        }
        else if (bufferPlayload.indexOf("null") == -1 && len > attr.max_payload_report)
        {
          Serial.println("# [ERROR] Current payload length : " + String(len));
          Serial.println("# [ERROR] Sensor.report() Failed payload is geather than: " + String(attr.max_payload_report));
          coreMQTT->clearSensorBuffer(*attr.docSensor);
          result.msgId = retrans.msgId;
          return result;
        }
      }
      else
      {
        Serial.println(F("# Can't sensor.report Because Not set function \" sensor.add(key,value)\" before sensor.report or Overload Memory toJSONString"));
      }
    }
    else
    {
      retrans.msgId = report.generateMsgId();
      result = this->sendRetransmit(bufferPlayload, retrans);
      coreMQTT->clearSensorBuffer(*attr.docSensor);
    }
    return result;
  }
  else // disable retransmit
  {
    if (retrans.msgId > 0)
    {
      result.statusReport = report.send(bufferPlayload, retrans.msgId);
      result.msgId = retrans.msgId;
      coreMQTT->clearSensorBuffer(*attr.docSensor);
    }
    else
    {
      retrans.msgId = report.generateMsgId();
      result.statusReport = report.send(bufferPlayload, retrans.msgId);
      result.msgId = retrans.msgId;
      coreMQTT->clearSensorBuffer(*attr.docSensor);
    }
    return result;
  }
  return result;
}



ResultReport MAGELLAN_SIM7600E_MQTT::Sensor::sendRetransmit(String payload, RetransmitSetting retrans)
{
  attr.reqRetransmit = true;
  ResultReport result;
  Report report;
  int countRetransmit = 0;
  attr.matchMsgId_send = retrans.msgId;
  int Timeout = ((retrans.repeat * retrans.duration) + 2) * 1000;
  prev_millis_timeout = millis();
  while (true)
  {
    coreMQTT->loop();
    coreMQTT->registerList(duplicate_subs_list);
    if (millis() - prev_millis_retrans > (retrans.duration * 1000) && countRetransmit < (retrans.repeat + 1))
    {
      if (countRetransmit > 0)
      {
        Serial.print(F("\n#Retransmit count: "));
        Serial.print(countRetransmit);
        Serial.print(F(" on MsgId: "));
        Serial.println(retrans.msgId);
      }
      result.statusReport = report.send(payload, retrans.msgId);
      result.msgId = retrans.msgId;
      countRetransmit++;
      if (countRetransmit > retrans.repeat)
      {
        Serial.print(F("\n# Report retransmit fail timeout on MsgId: "));
        Serial.println(attr.matchMsgId_send);

        attr.reqRetransmit = false;
        break;
      }
      prev_millis_retrans = millis();
    }
    if (attr.isMatchMsgId)
    {
      Serial.print(F("# Finished report transmission MsgId: "));
      Serial.println(attr.matchMsgId_send);
      // Serial.println(countRetransmit);
      result.msgId = attr.matchMsgId_send; // assign msgId
      result.statusReport = true;

      attr.isMatchMsgId = false;
      attr.matchMsgId_cb = -1;
      attr.matchMsgId_send = -1;
      attr.reqRetransmit = false;
      break;
    }
    // if (!coreMQTT->isConnected())
    // {
    //     Serial.print(F("\n# Report retransmit fail connection lost on MsgId: "));
    //     Serial.println(attr.matchMsgId_send);
    //     break;
    // }
    if (millis() - prev_millis_timeout > Timeout)
    {
      result.msgId = attr.matchMsgId_send; // assign msgId
      result.statusReport = false;
      attr.reqRetransmit = false;
      prev_millis_timeout = millis();
      Serial.print(F("\n# Triger timeout from send retransmit"));
      break;
    }
    if (attr.inProcessOTA && attr.reqRetransmit)
    {
      Serial.print(F("\n# In procress OTA cancel report with report retransmit"));
      result.msgId = attr.matchMsgId_send; // assign msgId
      result.statusReport = false;
      attr.reqRetransmit = false;
      break;
    }
  }
  return result;
}


// ver 1.1.2

void MAGELLAN_SIM7600E_MQTT::Sensor::remove(String sensorKey)
{
  if (findKey(sensorKey))
  {
    coreMQTT->remove(sensorKey, *attr.docSensor);
  }
  else
  {
    Serial.println("Not found [Key]: \"" + sensorKey + "\" to Remove");
  }
}

boolean MAGELLAN_SIM7600E_MQTT::Sensor::findKey(String sensorKey)
{
  return coreMQTT->findKey(sensorKey, *attr.docSensor);
}

void MAGELLAN_SIM7600E_MQTT::Sensor::update(String sensorKey, String sensorValue)
{

  if (findKey(sensorKey))
  {
    JsonDocUtils validateJSON_doc = readSafetyCapacity_Json_doc(*attr.docSensor);
    if (validateJSON_doc.used > validateJSON_doc.safety_size * 0.8)
    {
      String bufJSON = this->toJSONString();
      // attr.docSensor->clear();
      attr.docSensor = new DynamicJsonDocument(validateJSON_doc.max_size + 2048); // offset size
      Serial.println("# Update [Key] \"" + sensorKey + "\" JsonBuffer is full adjust to: " + String(coreMQTT->readBufferSensor(*attr.docSensor)));
      deserializeJson(*attr.docSensor, bufJSON);
      Serial.println("Updated [Key]: " + sensorKey);
      coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
    }
    else if (sensorValue.length() > 10000 && sensorValue.length() > coreMQTT->readBufferSensor(*attr.docSensor))
    {
      Serial.println(F("# Preparing large data to JSONbuffer"));
      String bufJSON = this->toJSONString();
      // attr.docSensor->clear();
      attr.docSensor = new DynamicJsonDocument(sensorValue.length() + bufJSON.length() + 3000); // offset size
      deserializeJson(*attr.docSensor, bufJSON);
      Serial.println("Updated [Key]: " + sensorKey);
      coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
    }
    else
    {
      Serial.println("Updated [Key]: " + sensorKey);
      coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
    }
  }
  else
  {
    Serial.println("Not found [Key]: \"" + sensorKey + "\" to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::update(String sensorKey, const char *sensorValue)
{
  if (findKey(sensorKey))
  {
    JsonDocUtils validateJSON_doc = readSafetyCapacity_Json_doc(*attr.docSensor);
    if (validateJSON_doc.used > validateJSON_doc.safety_size * 0.8)
    {
      String bufJSON = this->toJSONString();
      // attr.docSensor->clear();
      attr.docSensor = new DynamicJsonDocument(validateJSON_doc.max_size + 2048); // offset size
      Serial.println("# Update [Key] \"" + sensorKey + "\" JsonBuffer is full adjust to: " + String(coreMQTT->readBufferSensor(*attr.docSensor)));
      deserializeJson(*attr.docSensor, bufJSON);
      Serial.println("Updated [Key]: " + sensorKey);
      coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
    }
    else if (strlen(sensorValue) > 10000 && strlen(sensorValue) > coreMQTT->readBufferSensor(*attr.docSensor))
    {
      Serial.println(F("# Preparing large data to JSONbuffer"));
      String bufJSON = this->toJSONString();
      // attr.docSensor->clear();
      attr.docSensor = new DynamicJsonDocument(strlen(sensorValue) + bufJSON.length() + 3000); // offset size
      deserializeJson(*attr.docSensor, bufJSON);
      Serial.println("Updated [Key]: " + sensorKey);
      coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
    }
    else
    {
      Serial.println("Updated [Key]: " + sensorKey);
      coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
    }
  }
  else
  {
    Serial.println("Not found [Key]: " + sensorKey + " to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::update(String sensorKey, int sensorValue)
{
  if (findKey(sensorKey))
  {
    JsonDocUtils validateJSON_doc = readSafetyCapacity_Json_doc(*attr.docSensor);
    if (validateJSON_doc.used > validateJSON_doc.safety_size * 0.8)
    {
      String bufJSON = this->toJSONString();
      // attr.docSensor->clear();
      attr.docSensor = new DynamicJsonDocument(validateJSON_doc.max_size + 2048); // offset size
      Serial.println("# Update [Key] \"" + sensorKey + "\" JsonBuffer is full adjust to: " + String(coreMQTT->readBufferSensor(*attr.docSensor)));
      deserializeJson(*attr.docSensor, bufJSON);
      Serial.println("Updated [Key]: " + sensorKey);
      coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
    }
    else
    {
      Serial.println("Updated [Key]: " + sensorKey);
      coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
    }
  }
  else
  {
    Serial.println("Not found [Key]: " + sensorKey + " to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::update(String sensorKey, float sensorValue)
{
  if (findKey(sensorKey))
  {
    JsonDocUtils validateJSON_doc = readSafetyCapacity_Json_doc(*attr.docSensor);
    if (validateJSON_doc.used > validateJSON_doc.safety_size * 0.8)
    {
      String bufJSON = this->toJSONString();
      // attr.docSensor->clear();
      attr.docSensor = new DynamicJsonDocument(validateJSON_doc.max_size + 2048); // offset size
      Serial.println("# Update [Key] \"" + sensorKey + "\" JsonBuffer is full adjust to: " + String(coreMQTT->readBufferSensor(*attr.docSensor)));
      deserializeJson(*attr.docSensor, bufJSON);
      Serial.println("Updated [Key]: " + sensorKey);
      coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
    }
    else
    {
      Serial.println("Updated [Key]: " + sensorKey);
      coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
    }
  }
  else
  {
    Serial.println("Not found [Key]: " + sensorKey + " to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::update(String sensorKey, boolean sensorValue)
{
  if (findKey(sensorKey))
  {
    JsonDocUtils validateJSON_doc = readSafetyCapacity_Json_doc(*attr.docSensor);
    if (validateJSON_doc.used > validateJSON_doc.safety_size * 0.8)
    {
      String bufJSON = this->toJSONString();
      // attr.docSensor->clear();
      attr.docSensor = new DynamicJsonDocument(validateJSON_doc.max_size + 2048); // offset size
      Serial.println("# Update [Key] \"" + sensorKey + "\" JsonBuffer is full adjust to: " + String(coreMQTT->readBufferSensor(*attr.docSensor)));
      deserializeJson(*attr.docSensor, bufJSON);
      Serial.println("Updated [Key]: " + sensorKey);
      coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
    }
    else
    {
      Serial.println("Updated [Key]: " + sensorKey);
      coreMQTT->updateSensor(sensorKey, sensorValue, *attr.docSensor);
    }
  }
  else
  {
    Serial.println("Not found [Key]: " + sensorKey + " to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::Location::add(String LocationKey, double latitude, double longtitude)
{
  char b_lat[25];
  char b_lng[25];
  sprintf(b_lat, "%f", latitude);
  sprintf(b_lng, "%f", longtitude);
  String location = String(b_lat) + "," + String(b_lng);
  coreMQTT->addSensor(LocationKey, location, *attr.docSensor);
}

void MAGELLAN_SIM7600E_MQTT::Sensor::Location::add(String LocationKey, String latitude, String longtitude)
{
  if ((coreMQTT->CheckString_isDouble(latitude)) && (coreMQTT->CheckString_isDouble(longtitude)))
  {
    String location = latitude + "," + longtitude;
    coreMQTT->addSensor(LocationKey, location, *attr.docSensor);
  }
  else
  {
    Serial.println("# [" + LocationKey + "] Can't add Location latitude or longtitude is invalid (not number)");
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::Location::update(String LocationKey, double latitude, double longtitude)
{
  if (coreMQTT->findKey(LocationKey, *attr.docSensor))
  {
    char b_lat[25];
    char b_lng[25];
    sprintf(b_lat, "%f", latitude);
    sprintf(b_lng, "%f", longtitude);
    String location = String(b_lat) + "," + String(b_lng);
    Serial.println("Updated [Key]: " + LocationKey);
    coreMQTT->updateSensor(LocationKey, location, *attr.docSensor);
  }
  else
  {
    Serial.println("Not found [Key]: " + LocationKey + " to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::Location::update(String LocationKey, String latitude, String longtitude)
{
  if ((coreMQTT->CheckString_isDouble(latitude)) && (coreMQTT->CheckString_isDouble(longtitude)))
  {
    if (coreMQTT->findKey(LocationKey, *attr.docSensor))
    {
      String location = String(latitude) + "," + String(longtitude);
      Serial.println("Updated [Key]: " + LocationKey);
      coreMQTT->updateSensor(LocationKey, location, *attr.docSensor);
    }
    else
    {
      Serial.println("Not found [Key]: \"" + LocationKey + "\" to update");
    }
  }
  else
  {
    Serial.println("# [" + LocationKey + "] Can't update Location latitude or longtitude is invalid (not number)");
  }
}

void MAGELLAN_SIM7600E_MQTT::Sensor::clear()
{
  coreMQTT->clearSensorBuffer(*attr.docSensor);
}

void MAGELLAN_SIM7600E_MQTT::Sensor::setJSONBufferSize(size_t JsonBuffersize)
{
  Serial.print("# Set JSON buffer size: " + String(JsonBuffersize));
  coreMQTT->adjustBufferSensor(JsonBuffersize);
  Serial.println(" Status: " + String((readJSONBufferSize() == (int)JsonBuffersize) ? "Success" : "Fail"));
}
int MAGELLAN_SIM7600E_MQTT::Sensor::readJSONBufferSize()
{
  return coreMQTT->readBufferSensor(*attr.docSensor);
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::add(String clientConfigKey, String clientConfigValue)
{
  if (clientConfigValue == "null")
  {
    Serial.println("# add [Key] \"" + clientConfigKey + "\" failed, this function does not allow to set value \"null\"");
  }
  else
  {
    coreMQTT->addSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
  }
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::add(String clientConfigKey, const char *clientConfigValue)
{
  if (clientConfigValue == "null")
  {
    Serial.println("# add [Key] " + clientConfigKey + " failed, this function does not allow to set value \"null\"");
  }
  else
  {
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
  if (bufferPlayload.indexOf("null") == -1)
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
  if (coreMQTT->findKey(clientConfigKey, attr.docClientConf))
  {
    coreMQTT->remove(clientConfigKey, attr.docClientConf);
  }
  else
  {
    Serial.println("Not found [Key]: \"" + clientConfigKey + "\" to Remove");
  }
}

boolean MAGELLAN_SIM7600E_MQTT::ClientConfig::findKey(String clientConfigKey)
{
  return coreMQTT->findKey(clientConfigKey, attr.docClientConf);
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::update(String clientConfigKey, String clientConfigValue)
{
  if (coreMQTT->findKey(clientConfigKey, attr.docClientConf))
  {
    Serial.println("Updated [Key]: " + clientConfigKey);
    coreMQTT->updateSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
  }
  else
  {
    Serial.println("Not found [Key]: " + clientConfigKey + " to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::update(String clientConfigKey, const char *clientConfigValue)
{
  if (findKey(clientConfigKey))
  {
    Serial.println("Updated [Key]: " + clientConfigKey);
    coreMQTT->updateSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
  }
  else
  {
    Serial.println("Not found [Key]: " + clientConfigKey + " to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::update(String clientConfigKey, int clientConfigValue)
{
  if (findKey(clientConfigKey))
  {
    Serial.println("Updated [Key]: " + clientConfigKey);
    coreMQTT->updateSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
  }
  else
  {
    Serial.println("Not found [Key]: " + clientConfigKey + " to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::update(String clientConfigKey, float clientConfigValue)
{
  if (findKey(clientConfigKey))
  {
    Serial.println("Updated [Key]: " + clientConfigKey);
    coreMQTT->updateSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
  }
  else
  {
    Serial.println("Not found [Key]: " + clientConfigKey + " to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::update(String clientConfigKey, boolean clientConfigValue)
{
  if (findKey(clientConfigKey))
  {
    Serial.println("Updated [Key]: " + clientConfigKey);
    coreMQTT->updateSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
  }
  else
  {
    Serial.println("Not found [Key]: " + clientConfigKey + " to update");
  }
}

void MAGELLAN_SIM7600E_MQTT::ClientConfig::clear()
{
  coreMQTT->clearSensorBuffer(attr.docClientConf);
}

void MAGELLAN_SIM7600E_MQTT::subscribes(func_callback_registerList cb_subscribe_list)
{
  if (cb_subscribe_list != NULL)
  {
    duplicate_subs_list = cb_subscribe_list; // ver.1.1.2
  }
  coreMQTT->registerList(cb_subscribe_list);
}

void MAGELLAN_SIM7600E_MQTT::interval(unsigned long second, func_callback_ms cb_interval)
{
  if (!attr.inProcessOTA)
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

void MAGELLAN_SIM7600E_MQTT::getResponse(unsigned int eventResponse, resp_callback resp_cb)
{
  coreMQTT->getRESP(eventResponse, resp_cb);
}

// OTA Feature //////
void MAGELLAN_SIM7600E_MQTT::OnTheAir::begin()
{
  if (attr.calculate_chunkSize > 4096)
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

boolean MAGELLAN_SIM7600E_MQTT::OnTheAir::updateProgress(String FOTAstate, String description)
{
  return coreMQTT->updateProgressOTA(FOTAstate, description);
}

boolean flag_startOTA = false;
boolean MAGELLAN_SIM7600E_MQTT::OnTheAir::downloadFirmware(unsigned int fw_part, size_t part_size)
{
  boolean statusDL = false;

  if (coreMQTT->OTA_info.firmwareTotalSize <= 0 && !coreMQTT->OTA_info.isReadyOTA)
  {
    Serial.println(F("# [Warning] Can't downloadFirmware"));
    Serial.println(F("# Don't have firmware OTA information in save or the thing don't have firmware OTA"));
    Serial.println(F("# Make sure you get firmware Information first"));
    statusDL = false;
  }
  else
  {
    if (part_size <= 0)
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

  if (!exc_until_info_fwReady)
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

  while (true)
  {
    coreMQTT->loop();
    coreMQTT->handleOTA(true);
    if (millis() - exc_prvMillis > 5000 && !exc_until_info_fwReady) // get fw info every 5sec until fwReady
    {

      exc_prvMillis = millis();
      if (OTA_info.firmwareIsUpToDate == UNKNOWN)
      {
        countIfUnknownVersion++;
        attr.usingCheckUpdate = false;
        coreMQTT->requestFW_Info(); // getFirmwareInfo
        if (countIfUnknownVersion > MaxIfUnknownVersion)
        {
          Serial.println(F(""));
          Serial.println(F("# ====================================="));
          Serial.println(F("# No response from request firmware information"));
          Serial.println(F("# ====================================="));
          Serial.println(F(""));
          countIfUnknownVersion = 0;
          if (attr.isBypassAutoUpdate)
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

      else if (OTA_info.firmwareIsUpToDate == UP_TO_DATE)
      {
        Serial.println(F(""));
        Serial.println(F("# ====================================="));
        Serial.println(F("# Firmware is up to date execute cancel"));
        Serial.println(F("# ====================================="));
        Serial.println(F(""));
        exc_until_info_fwReady = true;
        if (attr.isBypassAutoUpdate)
        {
          attr.flagAutoOTA = false;
        }
        else
        {
          attr.flagAutoOTA = true;
        }
        break;
      }

      else if (OTA_info.firmwareIsUpToDate == OUT_OF_DATE)
      {
        exc_until_info_fwReady = attr.startReqDownloadOTA;
        if (!attr.inProcessOTA)
        {
          Serial.println(F(""));
          Serial.println(F("# ====================================="));
          Serial.println(F("# Execute start"));
          Serial.println(F("# ====================================="));
          Serial.println(F(""));
        }
      }
    }
    if (OTA_info.firmwareIsUpToDate == UP_TO_DATE && exc_until_info_fwReady)
    {
      exc_until_info_fwReady = true;
      Serial.println(F("# Debug Uptodate but infinity loop [UP_TO_DATE]"));
      break;
    }
    else if (OTA_info.firmwareIsUpToDate == UNKNOWN && exc_until_info_fwReady)
    {
      exc_until_info_fwReady = true;
      Serial.println(F("# Debug Uptodate but infinity loop [UNKNOWN]"));
      break;
    }
    if (!coreMQTT->isConnected())
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
  if (!attr.flagAutoOTA)
  {
    attr.isBypassAutoUpdate = true;
    coreMQTT->unregisterDownloadOTA();
  }
  else
  {
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
  if (attr.usingCheckUpdate)
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

  while (true)
  {
    coreMQTT->loop();
    coreMQTT->handleOTA(false);
    diff_timeMillis = millis() - check_prvMillis;
    if (diff_timeMillis > 3000 && !checkUntil_end) // get fw info every 5sec until fwReady
    {
      if (coreMQTT->OTA_info.firmwareIsUpToDate == UNKNOWN)
      {
        attr.usingCheckUpdate = true;
        coreMQTT->requestFW_Info();
        countCheckUpdate++;
        if (countCheckUpdate > maxCheckUpdate)
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
      else if (coreMQTT->OTA_info.firmwareIsUpToDate == UP_TO_DATE)
      {
        countCheckUpdate = 0;
        // checkUntil_end = true;
        // break;
      }
      else if (coreMQTT->OTA_info.firmwareIsUpToDate == OUT_OF_DATE)
      {
        countCheckUpdate = 0;
        // checkUntil_end = true;
        // break;
      }
      if (!attr.usingCheckUpdate)
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
    if (!attr.usingCheckUpdate && checkUntil_end)
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
  if (!flag_startOTA)
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

// v1.1.2
void adjust_BufferForMedia(size_t len_payload)
{
  if (len_payload <= (size_t)attr.max_payload_report)
  {
    size_t crr_clientBuffer = attr.mqtt_client->getBufferSize();
    if (crr_clientBuffer > (attr.calculate_chunkSize * 2))
    {
      attr.mqtt_client->setBufferSize(attr.calculate_chunkSize * 2);
    }

    if (attr.mqtt_client != NULL && attr.ext_Token.length() > 30)
    {
      if ((len_payload > crr_clientBuffer) && (crr_clientBuffer <= (size_t)attr.max_payload_report))
      {
        attr.mqtt_client->setBufferSize(len_payload + 2000); // offset mqtt client buffer
      }
    }
  }
  else
  {
    Serial.println("# Sensors payload is too large geater than: " + String(attr.max_payload_report));
    return;
  }
}

JsonDocUtils readSafetyCapacity_Json_doc(JsonDocument &ref_docs)
{
  JsonDocUtils JsonDocInfo;
  size_t mmr_usage = ref_docs.memoryUsage();
  size_t max_size = ref_docs.memoryPool().capacity();
  size_t safety_size = max_size * (0.97);
  JsonDocInfo.used = mmr_usage;
  JsonDocInfo.max_size = max_size;
  JsonDocInfo.safety_size = safety_size;
  return JsonDocInfo;
}

unsigned int MAGELLAN_SIM7600E_MQTT::OnTheAir::Downloads::getDelay()
{
  return attr.delayRequest_download;
}

void MAGELLAN_SIM7600E_MQTT::OnTheAir::Downloads::setDelay(unsigned int delayMillis)
{
  attr.delayRequest_download = delayMillis;
}

void checkLimitationPayload(size_t len_payload, size_t limitation)
{
  if (attr.clientNetInterface == useGSMClient)
  {
    if (len_payload > limitation) // GSM max 1500(topic(66) + payload)
    {
      Serial.println();
      Serial.println(F("# [ERROR] Report payload failure because GSMClient maximum package data(topic + payload) length is 1500"));
      Serial.println();
    }
  }
}
