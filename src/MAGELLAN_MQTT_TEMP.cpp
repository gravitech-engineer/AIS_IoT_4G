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
support esp32, esp8266

Author:(POC Device Magellan team)
Create Date: 25 April 2022.
Modified: 22 dec 2025.
*/

/*
 * This file includes code from TinyGSM
 * Copyright (c) 2016-2024 Volodymyr Shymanskyy
 * Licensed under LGPL-3.0-or-later
 *
 * Modifications:
 *  - Adapted for AIS 4G Board
 */

#include "MAGELLAN_MQTT_TEMP.h"
cb_on_disconnect func_on_disc;

static void adjust_BufferForMedia(size_t len_payload);
struct JsonDocUtils
{
  size_t used;
  size_t max_size;
  size_t safety_size;
};
static JsonDocUtils readSafetyCapacity_Json_doc(JsonDocument &ref_docs);

MAGELLAN_MQTT_device_core *MAGELLAN_MQTT_TEMP::coreMQTT = NULL;
MagellanSetting setting;

MAGELLAN_MQTT_TEMP::MAGELLAN_MQTT_TEMP(Client &_Client)
{
  coreMQTT = new MAGELLAN_MQTT_device_core(_Client);
}

boolean MAGELLAN_MQTT_TEMP::isConnected()
{
  return coreMQTT->isConnected();
}

void MAGELLAN_MQTT_TEMP::reconnect()
{
  coreMQTT->reconnect();
}

void MAGELLAN_MQTT_TEMP::disconnect()
{
  coreMQTT->disconnect();
}

void MAGELLAN_MQTT_TEMP::begin(MagellanSetting _setting)
{
#ifdef BYPASS_REQTOKEN
  if (_setting.ThingToken != "null" && _setting.ThingToken.length() > 25)
  {
    this->setManualToken(_setting.ThingToken);
  }
  else
  {
    Serial.println(F("# Invalid setting ThingToken"));
    Serial.println(F("# Define \"BYPASS_REQTOKEN\" but not setting ThingToken manual back into auto renew ThingToken mode"));
  }
#endif

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

  size_t revertChunkToBufferSize = attr.calculate_chunkSize * 2;
  // ThingIdentifier(ICCID) and ThingSecret(IMSI) .
  _setting.ThingIdentifier.trim();
  _setting.ThingSecret.trim();
  _setting.IMEI.trim();

  // second validate after get information
  if (coreMQTT->CheckString_isDigit(_setting.ThingIdentifier) && coreMQTT->CheckString_isDigit(_setting.ThingSecret))
  {
    Serial.print(F("ThingIdentifier: "));
    Serial.println(_setting.ThingIdentifier);
    Serial.print(F("ThingSecret: "));
    Serial.println(_setting.ThingSecret);
    Serial.print(F("IMEI: "));
    Serial.println(_setting.IMEI);
    if (_setting.port > 0)
    {

      beginCustom(_setting.ThingIdentifier, _setting.ThingSecret, _setting.IMEI, _setting.endpoint, _setting.port, revertChunkToBufferSize);
    }
    else
    {
      beginCustom(_setting.ThingIdentifier, _setting.ThingSecret, _setting.IMEI, _setting.endpoint, mgPort, revertChunkToBufferSize);
    }
    setting = _setting;
  }
  else
  {
    Serial.println(F("# ThingIdentifier(ICCID) or ThingSecret(IMSI) invalid value please check again"));
    Serial.println("# ThingIdentifier =>" + _setting.ThingIdentifier);
    Serial.println("# ThingSecret =>" + _setting.ThingSecret);
    Serial.println(F("# ==========================="));
    Serial.println(F("# Restart board"));
    delay(5000);
    ESP.restart();
  }
}

void MAGELLAN_MQTT_TEMP::beginCustom(String _thingIden, String _thingSecret, String _imei, String _host, int _port, uint16_t bufferSize)
{
  _thingIden.trim();
  _thingSecret.trim();
  String t_iden = _thingIden;
  String t_sec = _thingSecret;
  _imei.trim();

  String genClientID = t_iden + "_" + String(random(1000, 9999));
  this->coreMQTT->setAuthMagellan(t_iden, t_sec, _imei);
  this->coreMQTT->beginCustom(genClientID, true, _host, _port, bufferSize);
  coreMQTT->activeOTA(attr.calculate_chunkSize, true);
}

void MAGELLAN_MQTT_TEMP::loop()
{
  this->coreMQTT->loop();
  if (attr.flagAutoOTA)
    this->coreMQTT->handleOTA(true);

  if ((func_on_disc != NULL) && (!isConnected()))
  {
    func_on_disc();
  }
}

void MAGELLAN_MQTT_TEMP::heartbeat(unsigned int second)
{
  this->coreMQTT->heartbeat(second * 1000);
}

void MAGELLAN_MQTT_TEMP::heartbeat()
{
  this->coreMQTT->heartbeat();
}

String MAGELLAN_MQTT_TEMP::deserializeControl(String payload)
{
  return this->coreMQTT->deserialControlJSON(payload);
}

bool MAGELLAN_MQTT_TEMP::Report::send(String payload)
{
  ResultReport result;
  int len = payload.length();
  adjust_BufferForMedia(len + 2000);
  return coreMQTT->report(payload);
}

bool MAGELLAN_MQTT_TEMP::Report::send(String payload, int msgId) // 1.1.0
{
  ResultReport result;
  int len = payload.length();
  adjust_BufferForMedia(len + 2000);
  result = this->sendWithMsgId(payload, msgId);
  return result.statusReport;
}

bool MAGELLAN_MQTT_TEMP::Report::send(String key, String value)
{
  int len = value.length();
  adjust_BufferForMedia(len + 2000);
  return coreMQTT->report(key, value);
}

bool MAGELLAN_MQTT_TEMP::Report::send(String key, String value, int msgId) // 1.1.0
{
  ResultReport result;
  int len = value.length();
  adjust_BufferForMedia(len + 2000);
  result = sendWithMsgId(key, value, msgId);
  return result.statusReport;
}

bool MAGELLAN_MQTT_TEMP::Report::send(int UnixtsTimstamp, String payload)
{
  int len = payload.length();
  adjust_BufferForMedia(len + 2000);
  String u_timestamp = String(UnixtsTimstamp);
  return coreMQTT->reportTimestamp(u_timestamp, payload);
}

// ver.1.1.0
int MAGELLAN_MQTT_TEMP::Report::generateMsgId()
{
  return (int)random(9999, 9999999);
}

ResultReport MAGELLAN_MQTT_TEMP::Report::send(String payload, RetransmitSetting &retrans)
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
      result.statusReport = this->send(payload, retrans.msgId);
      result.msgId = retrans.msgId;
    }
    return result;
  }
  return result;
}

ResultReport MAGELLAN_MQTT_TEMP::Report::send(String reportKey, String reportValue, RetransmitSetting &retrans)
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

// ver.1.1.0 private
ResultReport MAGELLAN_MQTT_TEMP::Report::sendWithMsgId(String payload, int msgId)
{
  int len = payload.length();
  String _topic = "api/v2/thing/" + attr.ext_Token + "/report/persist/?id=" + String(msgId);
  ResultReport internalResult;
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

ResultReport MAGELLAN_MQTT_TEMP::Report::sendWithMsgId(String reportKey, String reportValue, int msgId)
{
  int len = reportValue.length();

  String _topic = "api/v2/thing/" + attr.ext_Token + "/report/persist/pta/?sensor=" + reportKey + "&id=" + String(msgId);
  ResultReport internalResult;
  adjust_BufferForMedia(len + 2000);
  bool result = attr.mqtt_client->publish(_topic.c_str(), reportValue.c_str());
  internalResult.statusReport = result;
  internalResult.msgId = msgId;
  String _debug = (result == true) ? "Success" : "Failure";
  Serial.println(F("-------------------------------"));
  Serial.println("# Report Plaintext with MsgId: " + String(msgId) + " is " + _debug);
  Serial.println("# [Sensors]: " + reportValue);
  return internalResult;
}

ResultReport MAGELLAN_MQTT_TEMP::Report::sendWithMsgId(String payload)
{
  int randomID = generateMsgId();
  return this->sendWithMsgId(payload, randomID);
}

ResultReport MAGELLAN_MQTT_TEMP::Report::sendWithMsgId(String reportKey, String reportValue)
{
  int randomID = this->generateMsgId();
  return this->sendWithMsgId(reportKey, reportValue, randomID);
}

unsigned long _prev_millis_retrans = 0;
unsigned long _prev_millis_timeout = 0;

ResultReport MAGELLAN_MQTT_TEMP::Report::sendRetransmit(String payload, RetransmitSetting retrans)
{
  attr.reqRetransmit = true;
  ResultReport result;
  int countRetransmit = 0;
  attr.matchMsgId_send = retrans.msgId;
  int Timeout = ((retrans.repeat * retrans.duration) + 2) * 1000;
  _prev_millis_timeout = millis();
  while (true)
  {
    coreMQTT->loop();
    coreMQTT->registerList(duplicate_subs_list);
    if (millis() - _prev_millis_retrans > (retrans.duration * 1000) && countRetransmit < (retrans.repeat + 1))
    {
      if (countRetransmit > 0)
      {
        Serial.print(F("\n#Retransmit count: "));
        Serial.print(countRetransmit);
        Serial.print(F(" on MsgId: "));
        Serial.println(retrans.msgId);
      }
      result = this->sendWithMsgId(payload, retrans.msgId);
      countRetransmit++;
      if (countRetransmit > retrans.repeat)
      {
        Serial.print(F("\n# Report retransmit fail timeout on MsgId: "));
        Serial.println(attr.matchMsgId_send);

        attr.reqRetransmit = false;
        break;
      }
      _prev_millis_retrans = millis();
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
    if (!coreMQTT->isConnected())
    {
      if (func_on_disc != NULL)
      {
        Serial.println(F("# connection lost from server"));
        Serial.println(F("# Report retransmit trigger event on disconnect"));
        func_on_disc();
      }
      else
      {
        Serial.print(F("\n# Report retransmit fail connection lost on MsgId: "));
        Serial.println(attr.matchMsgId_send);
        break;
      }
    }
    if (millis() - _prev_millis_timeout > Timeout)
    {
      result.msgId = attr.matchMsgId_send; // assign msgId
      result.statusReport = false;
      attr.reqRetransmit = false;
      _prev_millis_timeout = millis();
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

ResultReport MAGELLAN_MQTT_TEMP::Report::sendRetransmit(String reportKey, String reportValue, RetransmitSetting retrans)
{
  attr.reqRetransmit = true;
  ResultReport result;
  int countRetransmit = 0;
  attr.matchMsgId_send = retrans.msgId;
  int Timeout = ((retrans.repeat * retrans.duration) + 2) * 1000;
  _prev_millis_timeout = millis();
  while (true)
  {
    coreMQTT->loop();
    coreMQTT->registerList(duplicate_subs_list);
    if (millis() - _prev_millis_retrans > (retrans.duration * 1000) && countRetransmit < (retrans.repeat + 1))
    {
      result = this->sendWithMsgId(reportKey, reportValue, retrans.msgId);
      if (countRetransmit > 0)
      {
        Serial.print(F("\n# Retransmit count: "));
        Serial.print(countRetransmit);
        Serial.print(F(" on MsgId: "));
        Serial.println(retrans.msgId);
      }

      // result.msgId = retrans.msgId;

      countRetransmit++;
      if (countRetransmit > retrans.repeat)
      {
        Serial.print(F("\n# Report retransmit fail timeout on MsgId: "));
        Serial.println(attr.matchMsgId_send);
        attr.reqRetransmit = false;
        break;
      }
      _prev_millis_retrans = millis();
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
      if (func_on_disc != NULL)
      {
        Serial.println(F("# connection lost from server"));
        Serial.println(F("# Report retransmit trigger event on disconnect"));
        func_on_disc();
      }
      else
      {
        Serial.print(F("\n# Report retransmit fail connection lost on MsgId: "));
        Serial.println(attr.matchMsgId_send);
        break;
      }
    }
    if (millis() - _prev_millis_timeout > Timeout)
    {
      _prev_millis_timeout = millis();
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

ResultReport MAGELLAN_MQTT_TEMP::Report::sendRetransmit(String payload)
{
  int randomID = this->generateMsgId();
  RetransmitSetting retrans;
  retrans.msgId = randomID;
  ResultReport result = sendRetransmit(payload, retrans);
  return result;
}

ResultReport MAGELLAN_MQTT_TEMP::Report::sendRetransmit(String reportKey, String reportValue)
{
  int randomID = this->generateMsgId();
  RetransmitSetting retrans;
  retrans.msgId = randomID;
  ResultReport result = sendRetransmit(reportKey, reportValue, retrans);
  return result;
}

/// @details v1.1.0

boolean MAGELLAN_MQTT_TEMP::Subscribe::Report::response(unsigned int format)
{
  return coreMQTT->registerResponseReport(format);
}

boolean MAGELLAN_MQTT_TEMP::Subscribe::ReportWithTimestamp::response()
{
  return coreMQTT->registerResponseReportTimestamp();
}

boolean MAGELLAN_MQTT_TEMP::Subscribe::Heartbeat::response(unsigned int format)
{
  return coreMQTT->registerResponseHeartbeat(format);
}

boolean MAGELLAN_MQTT_TEMP::Subscribe::control(unsigned int format)
{
  return coreMQTT->registerControl(format);
}

boolean MAGELLAN_MQTT_TEMP::Subscribe::control(String controlKey)
{
  return coreMQTT->registerControl(controlKey);
}

boolean MAGELLAN_MQTT_TEMP::Subscribe::serverConfig(unsigned int format)
{
  return coreMQTT->registerConfig(format);
}

boolean MAGELLAN_MQTT_TEMP::Subscribe::serverConfig(String controlKey)
{
  return coreMQTT->registerConfig(controlKey);
}

boolean MAGELLAN_MQTT_TEMP::Subscribe::getServerTime(unsigned int format)
{
  return coreMQTT->registerTimestamp(format);
}

////////////////////////////////////////////////////////////////////////////////
boolean MAGELLAN_MQTT_TEMP::Unsubscribe::Report::response(unsigned int format)
{
  return coreMQTT->unregisterResponseReport(format);
}

boolean MAGELLAN_MQTT_TEMP::Unsubscribe::ReportWithTimestamp::response()
{
  return coreMQTT->unregisterResponseReportTimestamp();
}

boolean MAGELLAN_MQTT_TEMP::Unsubscribe::Heartbeat::response(unsigned int format)
{
  return coreMQTT->unregisterResponseHeartbeat(format);
}

boolean MAGELLAN_MQTT_TEMP::Unsubscribe::control(unsigned int format)
{
  return coreMQTT->unregisterControl(format);
}

boolean MAGELLAN_MQTT_TEMP::Unsubscribe::control(String controlKey)
{
  return coreMQTT->unregisterControl(controlKey);
}

boolean MAGELLAN_MQTT_TEMP::Unsubscribe::serverConfig(unsigned int format)
{
  return coreMQTT->unregisterConfig(format);
}

boolean MAGELLAN_MQTT_TEMP::Unsubscribe::serverConfig(String controlKey)
{
  return coreMQTT->unregisterConfig(controlKey);
}

boolean MAGELLAN_MQTT_TEMP::Unsubscribe::getServerTime(unsigned int format)
{
  return coreMQTT->unregisterTimestamp(format);
}
////////////////////////////////////////////////////////////////////////////////

String MAGELLAN_MQTT_TEMP::Information::getHostName()
{
  return coreMQTT->getHostName();
}

String MAGELLAN_MQTT_TEMP::Information::getThingToken()
{
  return coreMQTT->readToken();
}

void MAGELLAN_MQTT_TEMP::Information::getBoardInfo()
{
  Serial.println(F("#====== Board information ========="));
  Serial.println("ThingIdentifier: " + String(setting.ThingIdentifier));
  Serial.println("ThingSecret: " + String(setting.ThingSecret));
  if (setting.IMEI != "null" && setting.IMEI.length() > 8)
    Serial.println("IMEI: " + String(setting.IMEI));
  Serial.println(F("#================================="));
}

String MAGELLAN_MQTT_TEMP::Information::getThingIdentifier()
{
  return setting.ThingIdentifier;
}

String MAGELLAN_MQTT_TEMP::Information::getThingSecret()
{
  return setting.ThingSecret;
}

boolean MAGELLAN_MQTT_TEMP::Control::ACK(String controlKey, String controlValue)
{
  return coreMQTT->ACKControl(controlKey, controlValue);
}

boolean MAGELLAN_MQTT_TEMP::Control::ACK(String payload)
{
  return coreMQTT->ACKControl(payload);
}

void MAGELLAN_MQTT_TEMP::Control::request()
{
  coreMQTT->reqControlJSON();
}

void MAGELLAN_MQTT_TEMP::Control::request(String controlKey)
{
  coreMQTT->reqControl(controlKey);
}

void MAGELLAN_MQTT_TEMP::ServerConfig::request()
{
  coreMQTT->reqConfigJSON();
}

void MAGELLAN_MQTT_TEMP::ServerConfig::request(String serverConfigKey)
{
  coreMQTT->reqConfig(serverConfigKey);
}

void MAGELLAN_MQTT_TEMP::Sensor::add(String sensorKey, String sensorValue)
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

void MAGELLAN_MQTT_TEMP::Sensor::add(String sensorKey, const char *sensorValue)
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

void MAGELLAN_MQTT_TEMP::Sensor::add(String sensorKey, int sensorValue)
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

void MAGELLAN_MQTT_TEMP::Sensor::add(String sensorKey, float sensorValue)
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

void MAGELLAN_MQTT_TEMP::Sensor::add(String sensorKey, boolean sensorValue)
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

String MAGELLAN_MQTT_TEMP::Sensor::toJSONString()
{
  return coreMQTT->buildSensorJSON(*attr.docSensor);
}

void MAGELLAN_MQTT_TEMP::Sensor::report()
{
  String bufferPlayload = coreMQTT->buildSensorJSON(*attr.docSensor);
  int len = bufferPlayload.length();

  if (bufferPlayload.indexOf("null") == -1)
  {
    if (bufferPlayload.indexOf("null") == -1 && len < attr.max_payload_report)
    {
      adjust_BufferForMedia(len);
      coreMQTT->report(bufferPlayload);
      coreMQTT->clearSensorBuffer(*attr.docSensor);
    }
    else if (bufferPlayload.indexOf("null") == -1 && len > attr.max_payload_report)
    {
      Serial.println("# [ERROR] Current payload length : " + String(len));
      Serial.println("# [ERROR] Sensor.report() Failed payload is geather than: " + String(attr.max_payload_report));
      coreMQTT->clearSensorBuffer(*attr.docSensor);
      return;
    }
  }
  else
  {
    Serial.println(F("# Can't sensor.report Because Not set function \" sensor.add(key,value)\" before sensor.report or Overload Memory toJSONString"));
  }
}

ResultReport MAGELLAN_MQTT_TEMP::Sensor::sendRetransmit(String payload, RetransmitSetting retrans)
{
  attr.reqRetransmit = true;
  ResultReport result;
  Report report;
  int countRetransmit = 0;
  attr.matchMsgId_send = retrans.msgId;
  int Timeout = ((retrans.repeat * retrans.duration) + 2) * 1000;
  _prev_millis_timeout = millis();
  while (true)
  {
    coreMQTT->loop();
    coreMQTT->registerList(duplicate_subs_list);
    if (millis() - _prev_millis_retrans > (retrans.duration * 1000) && countRetransmit < (retrans.repeat + 1))
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
      _prev_millis_retrans = millis();
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
    if (!coreMQTT->isConnected())
    {
      if (func_on_disc != NULL)
      {
        Serial.println(F("# connection lost from server"));
        Serial.println(F("# Report retransmit trigger event on disconnect"));
        func_on_disc();
      }
      else
      {
        Serial.print(F("\n# Report retransmit fail connection lost on MsgId: "));
        Serial.println(attr.matchMsgId_send);
        break;
      }
    }
    if (millis() - _prev_millis_timeout > Timeout)
    {
      result.msgId = attr.matchMsgId_send; // assign msgId
      result.statusReport = false;
      attr.reqRetransmit = false;
      _prev_millis_timeout = millis();
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

ResultReport MAGELLAN_MQTT_TEMP::Sensor::report(RetransmitSetting &retrans)
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
          result = sendRetransmit(bufferPlayload, retrans);
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
      result = sendRetransmit(bufferPlayload, retrans);
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

void MAGELLAN_MQTT_TEMP::Sensor::remove(String sensorKey)
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

boolean MAGELLAN_MQTT_TEMP::Sensor::findKey(String sensorKey)
{
  return coreMQTT->findKey(sensorKey, *attr.docSensor);
}

void MAGELLAN_MQTT_TEMP::Sensor::update(String sensorKey, String sensorValue)
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

void MAGELLAN_MQTT_TEMP::Sensor::update(String sensorKey, const char *sensorValue)
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

void MAGELLAN_MQTT_TEMP::Sensor::update(String sensorKey, int sensorValue)
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

void MAGELLAN_MQTT_TEMP::Sensor::update(String sensorKey, float sensorValue)
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

void MAGELLAN_MQTT_TEMP::Sensor::update(String sensorKey, boolean sensorValue)
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

void MAGELLAN_MQTT_TEMP::Sensor::Location::add(String LocationKey, double latitude, double longtitude)
{
  char b_lat[25];
  char b_lng[25];
  sprintf(b_lat, "%f", latitude);
  sprintf(b_lng, "%f", longtitude);
  String location = String(b_lat) + "," + String(b_lng);
  coreMQTT->addSensor(LocationKey, location, *attr.docSensor);
}

void MAGELLAN_MQTT_TEMP::Sensor::Location::add(String LocationKey, String latitude, String longtitude)
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

void MAGELLAN_MQTT_TEMP::Sensor::Location::update(String LocationKey, double latitude, double longtitude)
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

void MAGELLAN_MQTT_TEMP::Sensor::Location::update(String LocationKey, String latitude, String longtitude)
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

void MAGELLAN_MQTT_TEMP::Sensor::clear()
{
  coreMQTT->clearSensorBuffer(*attr.docSensor);
}

void MAGELLAN_MQTT_TEMP::Sensor::setJSONBufferSize(size_t JsonBuffersize)
{
  Serial.print("# Set JSON buffer size: " + String(JsonBuffersize));
  coreMQTT->adjustBufferSensor(JsonBuffersize);
  Serial.println(" Status: " + String((readJSONBufferSize() == (int)JsonBuffersize) ? "Success" : "Fail"));
}
int MAGELLAN_MQTT_TEMP::Sensor::readJSONBufferSize()
{
  return coreMQTT->readBufferSensor(*attr.docSensor);
}

void MAGELLAN_MQTT_TEMP::ClientConfig::add(String clientConfigKey, String clientConfigValue)
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

void MAGELLAN_MQTT_TEMP::ClientConfig::add(String clientConfigKey, const char *clientConfigValue)
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

void MAGELLAN_MQTT_TEMP::ClientConfig::add(String clientConfigKey, int clientConfigValue)
{
  coreMQTT->addSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
}

void MAGELLAN_MQTT_TEMP::ClientConfig::add(String clientConfigKey, float clientConfigValue)
{
  coreMQTT->addSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
}

void MAGELLAN_MQTT_TEMP::ClientConfig::add(String clientConfigKey, boolean clientConfigValue)
{
  coreMQTT->addSensor(clientConfigKey, clientConfigValue, attr.docClientConf);
}

String MAGELLAN_MQTT_TEMP::ClientConfig::toJSONString()
{
  return coreMQTT->buildSensorJSON(attr.docClientConf);
}

void MAGELLAN_MQTT_TEMP::ClientConfig::save()
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

void MAGELLAN_MQTT_TEMP::ClientConfig::save(String payload)
{
  coreMQTT->reportClientConfig(payload);
}

void MAGELLAN_MQTT_TEMP::ClientConfig::remove(String clientConfigKey)
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

boolean MAGELLAN_MQTT_TEMP::ClientConfig::findKey(String clientConfigKey)
{
  return coreMQTT->findKey(clientConfigKey, attr.docClientConf);
}

void MAGELLAN_MQTT_TEMP::ClientConfig::update(String clientConfigKey, String clientConfigValue)
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

void MAGELLAN_MQTT_TEMP::ClientConfig::update(String clientConfigKey, const char *clientConfigValue)
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

void MAGELLAN_MQTT_TEMP::ClientConfig::update(String clientConfigKey, int clientConfigValue)
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

void MAGELLAN_MQTT_TEMP::ClientConfig::update(String clientConfigKey, float clientConfigValue)
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

void MAGELLAN_MQTT_TEMP::ClientConfig::update(String clientConfigKey, boolean clientConfigValue)
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

void MAGELLAN_MQTT_TEMP::ClientConfig::clear()
{
  coreMQTT->clearSensorBuffer(attr.docClientConf);
}

OTA_state isUpToDate = OTA_state::UNKNOWN_STATE;
void MAGELLAN_MQTT_TEMP::checkUpdate_inside()
{
  this->registerInfoOTA();
  if (!attr.inProcessOTA && attr.flagAutoOTA)
  {
    Serial.println(F("#[Allow auto OTA]checkUpdate after open new connection or reconnect"));
    isUpToDate = this->OTA.checkUpdate();
    switch (isUpToDate)
    {
    case OTA_state::UNKNOWN_STATE:
      Serial.println(F("#[Allow auto OTA] firmware profile is EMPTY or not set Publish"));
      break;
    case OTA_state::OUT_OF_DATE:
      Serial.println(F("#[Allow auto OTA] firmware profile is OUT_OF_DATE"));
      Serial.println(F("#[Allow auto OTA] OTA Execute start!"));
      this->OTA.executeUpdate();
      break;
    case OTA_state::UP_TO_DATE:
      Serial.println(F("#[Allow auto OTA] firmware profile is already UP_TO_DATE"));
      break;
    default:
      Serial.println(F("#[Allow auto OTA] out of scope on checkUpdate"));
      break;
    }
    attr.checkUpdate_inside = true;
  }
}

unsigned long Prv_checkupdate = 0;
void MAGELLAN_MQTT_TEMP::subscribes(func_callback_registerList cb_subscribe_list)
{
  if (cb_subscribe_list != NULL)
  {
    duplicate_subs_list = cb_subscribe_list; // ver.1.1.2
  }
  coreMQTT->registerList(cb_subscribe_list);
  // if (attr.flagAutoOTA && !attr.checkUpdate_inside)
  // {
  //   if (millis() - Prv_checkupdate >= attr.delayCheckUpdate_inside)
  //   {
  //     Prv_checkupdate = millis();
  //     checkUpdate_inside();
  //   }
  // }
}

void MAGELLAN_MQTT_TEMP::interval(unsigned long second, func_callback_ms cb_interval)
{
  if (!attr.inProcessOTA)
  {
    coreMQTT->interval_ms(second * 1000, cb_interval);
  }
}

boolean MAGELLAN_MQTT_TEMP::getServerTime()
{
  return coreMQTT->getTimestamp();
}

void MAGELLAN_MQTT_TEMP::getControl(String focusOnKey, ctrl_handleCallback ctrl_callback)
{
  attr.sub_check_list.SetSubscription(SubControlPlaintext, true);
  coreMQTT->getControl(focusOnKey, ctrl_callback);
}

void MAGELLAN_MQTT_TEMP::getControl(ctrl_PTAhandleCallback ctrl_pta_callback)
{
  attr.sub_check_list.SetSubscription(SubControlPlaintext, true);
  coreMQTT->getControl(ctrl_pta_callback);
}

void MAGELLAN_MQTT_TEMP::getControlJSON(ctrl_Json_handleCallback ctrl_json_callback)
{
  attr.sub_check_list.SetSubscription(SubControlJSON, true);
  coreMQTT->getControlJSON(ctrl_json_callback);
}

void MAGELLAN_MQTT_TEMP::getControlJSON(ctrl_JsonOBJ_handleCallback jsonOBJ_cb)
{
  attr.sub_check_list.SetSubscription(SubControlJSON, true);
  coreMQTT->getControlJSON(jsonOBJ_cb);
}

void MAGELLAN_MQTT_TEMP::getServerConfig(String focusOnKey, conf_handleCallback _conf_callback)
{
  attr.sub_check_list.SetSubscription(SubServerConfigPlaintext, true);
  coreMQTT->getConfig(focusOnKey, _conf_callback);
}

void MAGELLAN_MQTT_TEMP::getServerConfig(conf_PTAhandleCallback conf_pta_callback)
{
  attr.sub_check_list.SetSubscription(SubServerConfigPlaintext, true);
  coreMQTT->getConfig(conf_pta_callback);
}

void MAGELLAN_MQTT_TEMP::getServerConfigJSON(conf_Json_handleCallback conf_json_callback)
{
  attr.sub_check_list.SetSubscription(SubServerConfigJSON, true);
  coreMQTT->getConfigJSON(conf_json_callback);
}

void MAGELLAN_MQTT_TEMP::getServerConfigJSON(conf_JsonOBJ_handleCallback jsonOBJ_cb)
{
  attr.sub_check_list.SetSubscription(SubServerConfigJSON, true);
  coreMQTT->getConfigJSON(jsonOBJ_cb);
}

void MAGELLAN_MQTT_TEMP::getResponse(unsigned int eventResponse, resp_callback resp_cb)
{
  switch (eventResponse)
  {
  case UNIXTIME:
    attr.sub_check_list.SetSubscription(SubServerTimePlaintext, true);
    break;
  case RESP_REPORT_JSON:
    attr.sub_check_list.SetSubscription(SubReportResponseJSON, true);
    break;
  case RESP_REPORT_PLAINTEXT:
    attr.sub_check_list.SetSubscription(SubReportResponsePlaintext, true);
    break;
  case RESP_HEARTBEAT_JSON:
    attr.sub_check_list.SetSubscription(SubHeartbeatJSON, true);
    break;
  case RESP_HEARTBEAT_PLAINTEXT:
    attr.sub_check_list.SetSubscription(SubHeartbeatPlaintext, true);
    break;
  default:
    break;
  }
  coreMQTT->getRESP(eventResponse, resp_cb);
}

// @param sendingMsgId from user generate compare with recieve msgId inside callback
// @param incomingMsgId from callback push data in EVENTS key MsgId
boolean MAGELLAN_MQTT_TEMP::matchingMsgId(int sendingMsgId, int incomingMsgId)
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

void MAGELLAN_MQTT_TEMP::onDisconnect(cb_on_disconnect cb_disc)
{
  if (cb_disc != NULL)
  {
    func_on_disc = cb_disc;
  }
}

// OTA Feature //////
void MAGELLAN_MQTT_TEMP::OnTheAir::begin()
{
  if (attr.calculate_chunkSize > 4096)
  {
    Serial.print(F("#[Warning] activeOTA can't set chunk size: "));
    Serial.print(attr.calculate_chunkSize);
    Serial.print(F(" Chunk size Maximum is 4096 adjust chunk size to: "));
    attr.calculate_chunkSize = 4096;
    Serial.println(attr.calculate_chunkSize);
    coreMQTT->activeOTA(attr.calculate_chunkSize, true);
  }
  else
  {
    coreMQTT->activeOTA(attr.calculate_chunkSize, true);
  }
}

void MAGELLAN_MQTT_TEMP::OnTheAir::handle(boolean OTA_after_getInfo)
{
  coreMQTT->handleOTA(OTA_after_getInfo);
}

void MAGELLAN_MQTT_TEMP::OnTheAir::setChecksum(String md5Checksum)
{
  coreMQTT->setChecksum(md5Checksum);
}

boolean MAGELLAN_MQTT_TEMP::OnTheAir::Subscribe::firmwareInfo()
{
  return coreMQTT->registerInfoOTA();
}

boolean MAGELLAN_MQTT_TEMP::OnTheAir::Subscribe::firmwareDownload()
{
  return coreMQTT->registerDownloadOTA();
}
boolean MAGELLAN_MQTT_TEMP::OnTheAir::Unsubscribe::firmwareInfo()
{
  return coreMQTT->unregisterInfoOTA();
}

boolean MAGELLAN_MQTT_TEMP::OnTheAir::Unsubscribe::firmwareDownload()
{
  return coreMQTT->unregisterDownloadOTA();
}

boolean MAGELLAN_MQTT_TEMP::OnTheAir::getFirmwareInfo()
{
  return coreMQTT->requestFW_Info();
}

boolean MAGELLAN_MQTT_TEMP::OnTheAir::updateProgress(String FOTAstate, String description)
{
  return coreMQTT->updateProgressOTA(FOTAstate, description);
}

static boolean flag_startOTA = false;
boolean MAGELLAN_MQTT_TEMP::OnTheAir::downloadFirmware(unsigned int fw_chunkPart, size_t chunk_size)
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
    if (chunk_size <= 0)
    {
      statusDL = coreMQTT->requestFW_Download(fw_chunkPart, attr.calculate_chunkSize);
    }
    else
    {
      statusDL = coreMQTT->requestFW_Download(fw_chunkPart, chunk_size);
    }
    flag_startOTA = true;
  }
  return statusDL;
}

// 1.1.2
void MAGELLAN_MQTT_TEMP::subscribesHandler(func_callback_registerList cb_onConnected)
{
  duplicate_subs_list = [&]
  {
    attr.sub_check_list.GetSubList();
    if (attr.ext_Token != "" && attr.ext_Token.length() > 30)
    {
      if (!attr.inProcessOTA)
      {
        // if (attr.sub_check_list.SubControlPlaintext)
        if (attr.sub_check_list.GetSubscriptionStatus("SubControlPlaintext"))
        {
          coreMQTT->registerControl(PLAINTEXT);
        }
        if (attr.sub_check_list.GetSubscriptionStatus("SubControlJSON"))
        {
          coreMQTT->registerControl(JSON);
        }
        if (attr.sub_check_list.GetSubscriptionStatus("SubServerConfigPlaintext"))
        {
          coreMQTT->registerConfig(PLAINTEXT);
        }
        if (attr.sub_check_list.GetSubscriptionStatus("SubServerConfigJSON"))
        {
          coreMQTT->registerConfig(JSON);
        }
        if (attr.sub_check_list.GetSubscriptionStatus("SubServerTimePlaintext"))
        {
          coreMQTT->registerTimestamp(PLAINTEXT);
        }
        if (attr.sub_check_list.GetSubscriptionStatus("SubServerTimeJSON"))
        {
          coreMQTT->registerTimestamp(JSON);
        }
        if (attr.sub_check_list.GetSubscriptionStatus("SubReportResponseJSON"))
        {
          coreMQTT->registerResponseReport(JSON);
        }
        if (attr.sub_check_list.GetSubscriptionStatus("SubReportResponsePlaintext"))
        {
          coreMQTT->registerResponseReport(PLAINTEXT);
        }
        if (attr.sub_check_list.GetSubscriptionStatus("SubHeartbeatJSON"))
        {
          coreMQTT->registerResponseHeartbeat(JSON);
        }
        if (attr.sub_check_list.GetSubscriptionStatus("SubHeartbeatPlaintext"))
        {
          coreMQTT->registerResponseHeartbeat(PLAINTEXT);
        }
        if (attr.sub_check_list.GetSubscriptionStatus("SubReportWithTimestamp"))
        {
          coreMQTT->registerResponseReportTimestamp();
        }
      }

      if (cb_onConnected != NULL)
      {
        cb_onConnected();
      }
      // else
      // {
      //   attr.triggerRemainSub = true;
      //   Serial.println("@ Found OTA Inprocessing Unsubscribes unnecessary");
      //   if (attr.sub_check_list.SubControlPlaintext)
      //     coreMQTT->unregisterControl(PLAINTEXT);
      //   if (attr.sub_check_list.SubControlJSON)
      //     coreMQTT->unregisterControl(JSON);
      //   if (attr.sub_check_list.SubServerConfigPlaintext)
      //     coreMQTT->unregisterConfig(PLAINTEXT);
      //   if (attr.sub_check_list.SubServerConfigJSON)
      //     coreMQTT->unregisterConfig(JSON);
      //   if (attr.sub_check_list.SubServerTimePlaintext)
      //     coreMQTT->unregisterTimestamp(PLAINTEXT);
      //   if (attr.sub_check_list.SubServerTimeJSON)
      //     coreMQTT->unregisterTimestamp(JSON);
      //   if (attr.sub_check_list.SubReportResponseJSON)
      //     coreMQTT->unregisterResponseReport(JSON);
      //   if (attr.sub_check_list.SubReportResponsePlaintext)
      //     coreMQTT->unregisterResponseReport(PLAINTEXT);
      //   if (attr.sub_check_list.SubHeartbeatJSON)
      //     coreMQTT->unregisterResponseHeartbeat(JSON);
      //   if (attr.sub_check_list.SubHeartbeatPlaintext)
      //     coreMQTT->unregisterResponseHeartbeat(PLAINTEXT);
      //   if (attr.sub_check_list.SubReportWithTimestamp)
      //     coreMQTT->unregisterResponseReportTimestamp();
      // }
    }
  };
  coreMQTT->registerList(duplicate_subs_list);
}

OTA_INFO MAGELLAN_MQTT_TEMP::OnTheAir::utility()
{
  return coreMQTT->OTA_info;
}

static int maxCheckUpdate = 5;
static int countCheckUpdate = 0;
static boolean checkUntil_end = false;
static unsigned long check_prvMillis = 0;
static unsigned long diff_timeMillis = 0;
OTA_state MAGELLAN_MQTT_TEMP::OnTheAir::checkUpdate()
{
  if (attr.usingCheckUpdate)
  {
    Serial.println(F("# Debug protect debounce using checkUpdate"));
    return coreMQTT->OTA_info.firmwareIsUpToDate;
  }
  Serial.println(F("# Check Update"));
  Serial.println(F("# Waiting for response"));
  coreMQTT->OTA_info.firmwareIsUpToDate = OTA_state::UNKNOWN_STATE;
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
      if (coreMQTT->OTA_info.firmwareIsUpToDate == OTA_state::UNKNOWN_STATE)
      {
        attr.usingCheckUpdate = true;
        coreMQTT->requestFW_Info();
        countCheckUpdate++;
        if (countCheckUpdate > maxCheckUpdate)
        {
          checkUntil_end = true;
          Serial.println(F(""));
          Serial.println(F("# ====================================="));
          Serial.println(F("# No  response from request firmware information"));
          Serial.println(F("# ====================================="));
          Serial.println(F(""));
          countCheckUpdate = 0;
          break;
        }
      }
      else if (coreMQTT->OTA_info.firmwareIsUpToDate == OTA_state::UP_TO_DATE)
      {
        countCheckUpdate = 0;
        // checkUntil_end = true;
        // break;
      }
      else if (coreMQTT->OTA_info.firmwareIsUpToDate == OTA_state::OUT_OF_DATE)
      {
        countCheckUpdate = 0;
        // checkUntil_end = true;
        // break;
      }
      else if (coreMQTT->OTA_info.firmwareIsUpToDate == OTA_state::NOT_AVAILABLE_STATE)
      {
        countCheckUpdate = 0;
        // checkUntil_end = true;
        Serial.println(F("# ====================================="));
        Serial.println(F("# Debug Device not found or don't have FOTA Profile"));
        Serial.println(F("# ====================================="));
      }
      if (!attr.usingCheckUpdate)
      {
        checkUntil_end = true;
        Serial.println(F("# ====================================="));
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

static int MaxIfUnknownVersion = 5;
static int countIfUnknownVersion = 0;
static boolean exc_until_info_fwReady = true;
static unsigned long exc_prvMillis = 0;
void MAGELLAN_MQTT_TEMP::OnTheAir::executeUpdate()
{
  coreMQTT->OTA_info.firmwareIsUpToDate = OTA_state::UNKNOWN_STATE; // back to Unknown for recieve new firmware status

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

      if (OTA_info.firmwareIsUpToDate == OTA_state::UNKNOWN_STATE)
      {
        countIfUnknownVersion++;
        attr.usingCheckUpdate = false;
        coreMQTT->requestFW_Info(); // getFirmwareInfo
        if (countIfUnknownVersion > MaxIfUnknownVersion)
        {
          Serial.println(F(""));
          Serial.println(F("# ====================================="));
          Serial.println(F("# No response from request firmware information"));
          Serial.println(F("# Execute cancel"));
          Serial.println(F("# ====================================="));
          Serial.println(F(""));
          countIfUnknownVersion = 0;
          if (attr.isBypassAutoUpdate) // back to existing
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
      else if (OTA_info.firmwareIsUpToDate == OTA_state::UP_TO_DATE)
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
      else if (OTA_info.firmwareIsUpToDate == OTA_state::OUT_OF_DATE)
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
    if (OTA_info.firmwareIsUpToDate == OTA_state::OUT_OF_DATE && exc_until_info_fwReady)
    {
      exc_until_info_fwReady = true;
      Serial.println(F("# Debug Uptodate but infinity loop [UP_TO_DATE]"));
      break;
    }
    else if (OTA_info.firmwareIsUpToDate == OTA_state::UNKNOWN_STATE && exc_until_info_fwReady)
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

void MAGELLAN_MQTT_TEMP::OnTheAir::autoUpdate(boolean flagSetAuto)
{
  attr.flagAutoOTA = flagSetAuto;
  if (!flagSetAuto)
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

boolean MAGELLAN_MQTT_TEMP::OnTheAir::getAutoUpdate()
{
  // boolean mode = attr.flagAutoOTA;
  // Serial.print(F("# Auto update : "));
  // Serial.println((mode)? "ENABLE":"DISABLE");
  return attr.flagAutoOTA;
}

String MAGELLAN_MQTT_TEMP::OnTheAir::readDeviceInfo()
{
  return configOTAFile.readLastedOTA();
}

boolean MAGELLAN_MQTT_TEMP::OnTheAir::start()
{
  if (!flag_startOTA)
  {
    Serial.println(F("# Start OTA!"));
    return downloadFirmware(0, attr.calculate_chunkSize);
  }
  return false;
}
/////////////////////

String MAGELLAN_MQTT_TEMP::Utility::toDateTimeString(unsigned long unixtTime, int timeZone)
{
  return utls.toDateTimeString(unixtTime, timeZone);
}

String MAGELLAN_MQTT_TEMP::Utility::toUniversalTime(unsigned long unixtTime, int timeZone)
{
  return utls.toUniversalTime(unixtTime, timeZone);
}

unsigned long MAGELLAN_MQTT_TEMP::Utility::toUnix(tm time_)
{
  return utls.toUnix(time_);
}

tm MAGELLAN_MQTT_TEMP::Utility::convertUnix(unsigned long unix, int timeZone)
{
  return utls.convertUnix(unix, timeZone);
}

// v1.1.0
static void adjust_BufferForMedia(size_t len_payload)
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

static JsonDocUtils readSafetyCapacity_Json_doc(JsonDocument &ref_docs)
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

unsigned int MAGELLAN_MQTT_TEMP::OnTheAir::Downloads::getDelay()
{
  return attr.delayRequest_download;
}

void MAGELLAN_MQTT_TEMP::OnTheAir::Downloads::setDelay(unsigned int delayMillis)
{
  attr.delayRequest_download = delayMillis;
}

#ifdef BYPASS_REQTOKEN
void MAGELLAN_MQTT_TEMP::setManualToken(String token_)
{
  this->coreMQTT->setManualToken(token_);
}
#endif