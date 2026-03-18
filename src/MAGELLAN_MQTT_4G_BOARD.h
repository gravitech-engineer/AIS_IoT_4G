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

#ifndef MAGELLAN_MQTT_4G_H
#define MAGELLAN_MQTT_4G_H
#include <Arduino.h>
#include "utils/MAGELLAN_MQTT_device_core.h"
#include "MAGELLAN_MQTT_TEMP.h"

#ifdef ESP32
#include <Update.h>
#include "utils/TinyGSM/src/TinyGsmClient.h"
#include "utils/BuiltInSensorsModule.h"
#define PIN_MODEM_TX 13
#define PIN_MODEM_RX 14
#define PIN_MODEM_PWR 12
#elif defined ESP8266
#error "AIS 4G Board is only supported on ESP32 platform."
#endif

extern MagellanSetting setting;
class MAGELLAN_MQTT_4G_BOARD : public MAGELLAN_MQTT_TEMP
{
public:
  MAGELLAN_MQTT_4G_BOARD();
  void powerModem();
  void initSerialModem();
  void connectModem();

  void checkModem();
  void HandleModem(); // handle modem connection and reconnect mqtt when ppp connected
  void InitGSM();     // initialize GSM modem is using function above running by correctly sequence.
  TinyGsmClient &getGSMClient();
  TinyGsm &getGSMModem();

  void begin(MagellanSetting _setting = setting);
  void disconnect();
  void reconnect() override;
  void loop() override;

  int16_t getSignalStrength(); // in dBm
  String getRSSIQuality();

  struct Centric
  {
  public:
    void begin(MagellanSetting _setting = setting);
    MAGELLAN_MQTT_4G_BOARD *parent;

  private:
  } centric;

  struct GPS_utils
  {
  public:
    MAGELLAN_MQTT_4G_BOARD *parent;

    void begin();
    void disable();
    boolean available();
    float readLatitude();
    float readLongitude();
    float readAltitude();
    float readSpeed();
    float readCourse();
    String readLocation();
    unsigned long getUnixTime();
    GPS_Data getCurrentGPSData();

  private:
    GPS_SIM7600E gps_internal;
    GPS_Data _gpsData;
  } gps;

  struct BuiltinSensor
  {
  public:
    MAGELLAN_MQTT_4G_BOARD *parent;
    void begin();
    float readTemperature();
    float readHumidity();
  } builtInSensor;

private:
protected:
};
#endif
