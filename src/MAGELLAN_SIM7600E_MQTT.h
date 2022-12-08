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
Modified: 1 december 2022.
Released for private usage.
*/
#ifndef MAGELLAN_SIM7600E_MQTT_h
#define MAGELLAN_SIM7600E_MQTT_h

#include <Arduino.h>
#include "utils/MAGELLAN_MQTT_device_core.h"
#include <Update.h>

struct Setting{
  String ThingIdentifier = "null"; //ICCID, ThingKey, ThingIdentifier
  String ThingSecret = "null"; //IMSI, ThingSecret
  String IMEI = "null";
  size_t clientBufferSize = defaultBuffer;
  boolean builtInSensor = true;
  String endpoint = _host_production;
};

class MAGELLAN_SIM7600E_MQTT: private MAGELLAN_MQTT_device_core
{
private:

public:
  MAGELLAN_SIM7600E_MQTT();
  MAGELLAN_SIM7600E_MQTT(Client& client);
  void begin(Setting _setting);
  void begin(uint16_t bufferSize = 1024, boolean builtInSensor = true);
  void begin(String _thingIden, String _thingSencret, String _imei, unsigned int Zone = Production, uint16_t bufferSize = 1024, boolean builtinSensor = true);
  void beginCustom(String _client_id, boolean buildinSensor = true, String _host = "magellan.ais.co.th", int _port = mgPort, uint16_t bufferSize = 1024);
  void beginCustom(String _thingIden, String _thingSencret, String _imei, String _host = "magellan.ais.co.th", int _port = mgPort, uint16_t bufferSize = 1024, boolean builtinSensor = true);
  void loop();
  void heartbeat(unsigned int second);
  void subscribes(func_callback_registerList cb_subscribe_list);
  void interval(unsigned long second, func_callback_ms cb_interval);
  boolean getServerTime();
  void getControl(String focusOnKey, ctrl_handleCallback ctrl_callback);
  void getControl(ctrl_PTAhandleCallback ctrl_pta_callback);
  void getControlJSON(ctrl_Json_handleCallback ctrl_json_callback);
  void getControlJSON(ctrl_JsonOBJ_handleCallback jsonOBJ_cb);

  void getServerConfig(String focusOnKey, conf_handleCallback _conf_callback);
  void getServerConfig(conf_PTAhandleCallback conf_pta_callback);
  void getServerConfigJSON(conf_Json_handleCallback conf_json_callback);
  void getServerConfigJSON(conf_JsonOBJ_handleCallback jsonOBJ_cb);
  void getResponse(unsigned int eventResponse, resp_callback resp_cb);
  String deserializeControl(String payload);
  boolean isConnected();
  struct Sensor
  {
    public:
      void add(String sensorKey, String sensorValue);
      void add(String sensorKey, const char* sensorValue);
      void add(String sensorKey, int sensorValue);
      void add(String sensorKey, float sensorValue);
      void add(String sensorKey, boolean sensorValue);
      void remove(String sensorKey);
      String toJSONString();
      void report();
      void update(String sensorKey, String sensorValue);
      void update(String sensorKey, const char* sensorValue);
      void update(String sensorKey, int sensorValue);
      void update(String sensorKey, float sensorValue);
      void update(String sensorKey, boolean sensorValue);
      boolean findKey(String sensorKey);
      void clear();
      void setJSONBufferSize(size_t JsonBuffersize);
      int readJSONBufferSize();
    struct Location{
      void add(String LocationKey, double latitude, double longtitude);
      void add(String LocationKey, String latitude, String longtitude);
      void update(String LocationKey, double latitude, double longtitude);
      void update(String LocationKey, String latitude, String longtitude);   
    }location;
  }sensor;
  struct ClientConfig
  {
    public:
      void add(String clientConfigKey, String clientConfigValue);
      void add(String clientConfigKey, const char* clientConfigValue);
      void add(String clientConfigKey, int clientConfigValue);
      void add(String clientConfigKey, float clientConfigValue);
      void add(String clientConfigKey, boolean clientConfigValue);
      void remove(String clientConfigKey);
      String toJSONString();
      void save();
      void save(String payload);
      void update(String clientConfigKey, String clientConfigValue);
      void update(String clientConfigKey, const char* clientConfigValue);
      void update(String clientConfigKey, int clientConfigValue); 
      void update(String clientConfigKey, float clientConfigValue);
      void update(String clientConfigKey, boolean clientConfigValue);      
      boolean findKey(String clientConfigKey);
      void clear();
  }clientConfig;
  struct ServerConfig
  {
    void request();
    void request(String serverConfigKey);
  }serverConfig;

  struct Control
  {
    void request();
    void request(String controlKey);
    boolean ACK(String controlKey, String controlValue);
    boolean ACK(String payload);
  }control;

  struct Information
  {
    String getHostName(); //
    void getBoardInfo(); //
    String getICCID();
    String getIMSI();
    String getIMEI();
    String getThingToken();
  }info;

  struct Report
  {
    boolean send(String payload);
    boolean send(String reportKey, String reportValue);
    boolean send(int UnixtsTimstamp, String payload);
  }report;

  struct Subscribe
  {
    boolean control(unsigned int format = JSON);
    boolean control(String controlKey);
    boolean serverConfig(unsigned int format = JSON);
    boolean serverConfig(String configKey);
    boolean getServerTime(unsigned int format = JSON);
    struct Report
    {
      boolean response(unsigned int format = JSON);
    }report;
    struct ReportWithTimestamp
    {
      boolean response();
    }reportWithTimestamp;
    struct Heartbeat
    {
      boolean response(unsigned int format = JSON);
    }heartbeat;

  }subscribe;

  struct Unsubscribe
  {
    boolean control(unsigned int format = JSON);
    boolean control(String controlKey);
    boolean serverConfig(unsigned int format = JSON);
    boolean serverConfig(String configKey);
    boolean getServerTime(unsigned int format = JSON);
    struct Report
    {
      boolean response(unsigned int format = JSON);
    }report;
    struct ReportWithTimestamp
    {
      boolean response();
    }reportWithTimestamp;
    struct Heartbeat
    {
      boolean response(unsigned int format = JSON);
    }heartbeat;

  }unsubscribe;
  struct OnTheAir
  {
    public:
      int checkUpdate();
      OTA_INFO utility();
      void autoUpdate(boolean flagSetAuto = true);
      boolean getAutoUpdate();
      void executeUpdate();
      String readDeviceInfo();
    private:
      boolean getFirmwareInfo();
      void begin();
      boolean start();   
      void handle(boolean OTA_after_getInfo = true);
      void setChecksum(String md5Checksum);
      boolean updateProgress(String OTA_state, String description);
      boolean downloadFirmware(unsigned int fw_part = 0, size_t part_size = 0);     
      struct Subscribe{
          boolean firmwareInfo();
          boolean firmwareDownload();
      }subscribe;
      struct Unsubscribe{
          boolean firmwareInfo();
          boolean firmwareDownload();
      }unsubscribe;

  }OTA; 

  struct GPSmodule
  {
    boolean available();
    float readLatitude();
    float readLongitude();
    float readAltitude();
    float readSpeed();
    float readCourse();
    String readLocation();
    unsigned long getUnixTime(); //*
  }gps;
  struct BuiltinSensor
  {
    float readTemperature();
    float readHumidity();
  }builtInSensor;
  struct Centric
  {
    public:
      void begin(uint16_t setBufferSize = 1024);
      void begin(Setting _setting);
  }centric;

  struct Utility{
    String toDateTimeString(unsigned long unixtTime, int timeZone);
    String toUniversalTime(unsigned long unixtTime, int timeZone);
    unsigned long toUnix(tm time_);
    tm convertUnix(unsigned long unix, int timeZone);   
  }utils;
protected:
  static MAGELLAN_MQTT_device_core *coreMQTT;
};
extern Setting setting;
#endif
