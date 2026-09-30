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
Modified: 30 September 2026.
*/

/*
 * This file includes code from TinyGSM
 * Copyright (c) 2016-2024 Volodymyr Shymanskyy
 * Licensed under LGPL-3.0-or-later
 *
 * Modifications:
 *  - Adapted for AIS 4G Board
 */
#include <Arduino.h>
#include "MAGELLAN_MQTT_4G_BOARD.h"
const char *_apn = "aisboard.4g.ais";
HardwareSerial _SerialAT(1);
TinyGsm _modem(_SerialAT);
TinyGsmClient _gsmClient(_modem);

static bool _serialAtStarted = false;

static bool waitForModemATReady(uint8_t maxRetries, uint16_t delayMs)
{
  for (uint8_t retry = 0; retry < maxRetries; ++retry)
  {
    if (_modem.testAT(1000))
    {
      return true;
    }
    delay(delayMs);
  }
  return false;
}

TinyGsmClient &MAGELLAN_MQTT_4G_BOARD::getGSMClient()
{
  return _gsmClient;
}

TinyGsm &MAGELLAN_MQTT_4G_BOARD::getGSMModem()
{
  return _modem;
}

int mapRSSITodBm(int rssi)
{
  if (rssi == 99)
    return -113; // Not detectable

  return -113 + (rssi * 2); // Map RSSI to dBm
}

String _getSignalStrengthCategory(int dBm)
{
  if (dBm <= -113)
    return "Very Poor";
  else if (dBm > -113 && dBm <= -85)
    return "Poor";
  else if (dBm > -85 && dBm <= -70)
    return "Fair";
  else if (dBm > -70 && dBm <= -55)
    return "Good";
  else if (dBm > -55)
    return "Excellent";

  return "Unknown";
}

void _getRadio()
{
  MG_LOG_I("#========= Radio Quality information ==========");
  int rssiNomalized = _modem.getSignalQuality();
  int rssiDbm = mapRSSITodBm(rssiNomalized);
  MG_LOG_I_S("Signal Strength: " + String(rssiNomalized));
  MG_LOG_I_S("Signal Strength(dBm): " + String(rssiDbm));
  MG_LOG_I_S("Description: " + String(_getSignalStrengthCategory(rssiDbm)));
}

MAGELLAN_MQTT_4G_BOARD::MAGELLAN_MQTT_4G_BOARD() : MAGELLAN_MQTT_TEMP(_gsmClient)
{
  gps.parent = this;
  centric.parent = this;
  radioSignal.parent = this;
  GSMModem.parent = this;
  builtInSensor.parent = this;

  attr.cb_before_restart = []()
  {
    MG_LOG_I("# GSM shutdown before restart...");
    _modem.poweroff();
  };
}

MAGELLAN_MQTT_4G_BOARD::MAGELLAN_MQTT_4G_BOARD(Client &client) : MAGELLAN_MQTT_TEMP(client)
{
}

void MAGELLAN_MQTT_4G_BOARD::initSerialModem()
{
  if (!_serialAtStarted)
  {
    _SerialAT.setRxBufferSize(4096 * 2);
    _SerialAT.begin(115200, SERIAL_8N1, PIN_MODEM_RX, PIN_MODEM_TX);
    _serialAtStarted = true;
    delay(1000);
  }
  else
  {
    // Keep UART active on warm-reset flows; avoid resizing while running.
    delay(100);
  }
  _modem.init();
}

void MAGELLAN_MQTT_4G_BOARD::powerModem()
{
  pinMode(PIN_MODEM_PWR, OUTPUT);
  MG_LOG_I("Power cycling modem (Hard Cut VBAT)...");

  // สั่งตัดไฟโมดูล
  digitalWrite(PIN_MODEM_PWR, LOW); // หรือ HIGH แล้วแต่วงจร MOSFET

  // หน่วงเวลาอย่างน้อย 3 วินาที เพื่อให้ C-Filter คายประจุหมดเกลี้ยง
  delay(3000);

  // จ่ายไฟกลับเข้าโมดูล
  digitalWrite(PIN_MODEM_PWR, HIGH);

  // รอนานขึ้นหน่อยเพื่อให้โมดูลเริ่ม Boot หลังจากได้รับไฟใหม่
  MG_LOG_I("Waiting for modem bootup...");
  delay(5000);
}

bool MAGELLAN_MQTT_4G_BOARD::isDataPathReady()
{
  return _modem.isGprsConnected() && _modem.localIP() != IPAddress(0, 0, 0, 0);
}

void MAGELLAN_MQTT_4G_BOARD::connectModem()
{
  if (this->isDataPathReady())
  {
    MG_LOG_I("GPRS already connected. Skip reconnect.");
    return;
  }

  const uint8_t maxNetworkWaitRetries = 6;
  uint8_t networkWaitRetry = 0;
  while (!_modem.isNetworkConnected() && networkWaitRetry < maxNetworkWaitRetries)
  {
    MG_LOG_I_S("Waiting for network registration... " + String(networkWaitRetry + 1) + "/" + String(maxNetworkWaitRetries));
    // Bounded wait so init path does not block forever when cell registration is unstable.
    _modem.waitForNetwork(10000L, true);
    delay(300);
    networkWaitRetry++;
  }

  if (!_modem.isNetworkConnected())
  {
    MG_LOG_E("Network registration timeout. Restarting ESP...");
    ESP.restart();
    return;
  }

  MG_LOG_I("Connecting to mobile network...");
  uint8_t retry = 0;
  const uint8_t maxGprsRetries = 10;
  while (!_modem.gprsConnect(_apn))
  {
    retry++;
    MG_LOG_E_S("Failed to connect! Retry " + String(retry) + "/" + String(maxGprsRetries));
    delay(800);

    if (retry >= maxGprsRetries)
    {
      MG_LOG_E("Max retries reached. Restarting ESP...");
      ESP.restart();
      return;
    }
  }
  MG_LOG_I("modem connected!");
  // PDP context can report success before the data path is actually routable; give it
  // a moment to settle before the caller immediately tries an MQTT/TCP connection.
  delay(1500); // Wait for data path to settle
}
static unsigned long _prev_checkModem_millis = 0;
void MAGELLAN_MQTT_4G_BOARD::checkModem()
{
  unsigned long now = millis();
  // Rate-limit reconnect attempts: wait 5000 ms between tries to let recovery stabilise
  if (now - _prev_checkModem_millis >= 5000)
  {
    _prev_checkModem_millis = now;
    if (!this->isDataPathReady())
    {
      if (!_modem.isNetworkConnected())
      {
        MG_LOG_E("Cellular Network is registering in background... skip this round.");
        return;
      }
      MG_LOG_I("Data path not ready. Performing recovery...");
      this->recoverCellular();
      return;
    }
  }
}

// old
//  void MAGELLAN_MQTT_4G_BOARD::checkModem()
//  {
//    if (!_modem.isGprsConnected())
//    {
//      unsigned long now = millis();
//      // Rate-limit reconnect attempts: wait 500 ms between tries to let PPP stabilise
//      if (now - _prev_checkModem_millis >= 500)
//      {
//        MG_LOG_I("Reconnecting PPP...");
//        _modem.gprsConnect(_apn);
//        _prev_checkModem_millis = now;
//      }
//    }
//  }

// void MAGELLAN_MQTT_4G_BOARD::HandleModemMagellanConnection()
// {
//   if (_modem.isGprsConnected() && !this->MAGELLAN_MQTT_TEMP::isConnected())
//   {
//     MG_LOG_I("Reconnecting MQTT...");
//     this->MAGELLAN_MQTT_TEMP::reconnect();
//   }
// }

// // Runtime network recovery state for throttled checks and edge-triggered logs.
static uint32_t lastNetCheckTime = 0;
static const uint32_t netCheckInterval = 5000; // Check every 5s to avoid blocking loops.
void MAGELLAN_MQTT_4G_BOARD::handleModemMagellan()
{
  if (this->MAGELLAN_MQTT_TEMP::isConnected())
  {
    return;
  }

  if (millis() - lastNetCheckTime >= netCheckInterval)
  {
    lastNetCheckTime = millis(); // อัปเดตเวลาล่าสุด
    if (!this->isDataPathReady())
    {
      if (!_modem.isNetworkConnected())
      {
        MG_LOG_E("Cellular Network is registering in background... skip this round.");
        return;
      }

      // ถ้าสัญญาณเสายังดี แต่ท่อเน็ตปิดอยู่ ให้เรียก recoverCellular สำหรับการกู้คืนแบบเต็มรูปแบบ
      MG_LOG_I("Network ready but data path not ready. Performing full recovery...");
      this->recoverCellular();
      return;
    }

    MG_LOG_I("GPRS is OK but MQTT is down. Reconnecting MQTT...");
    if (!this->MAGELLAN_MQTT_TEMP::isConnected())
    {
      this->MAGELLAN_MQTT_TEMP::reconnect();
    }
  }
}

bool MAGELLAN_MQTT_4G_BOARD::resolveDomain(const char *domain, IPAddress &ip)
{
  TinyGsm &modem = this->getGSMModem();

  MG_LOG_I("[DNS] Resolving %s", domain);

  modem.sendAT("+CDNSGIP=\"", domain, "\"");

  uint32_t start = millis();

  while (millis() - start < 15000)
  {
    if (modem.stream.available())
    {
      String line = modem.stream.readStringUntil('\n');
      line.trim();

      // MG_LOG_D("[DNS] %s", line.c_str());

      if (line.startsWith("+CDNSGIP:"))
      {

        int q1 = line.indexOf('"');
        int q2 = line.indexOf('"', q1 + 1);
        int q3 = line.indexOf('"', q2 + 1);
        int q4 = line.indexOf('"', q3 + 1);

        if (q3 >= 0 && q4 > q3)
        {
          String ipStr = line.substring(q3 + 1, q4);

          if (ip.fromString(ipStr))
          {
            // MG_LOG_I("[DNS] %s -> %s",
            //          domain,
            //          ip.toString().c_str());
            return true;
          }
        }
      }

      if (line == "ERROR")
        return false;
    }

    delay(10);
  }

  MG_LOG_E("[DNS] Timeout resolving %s", domain);
  return false;
}

String MAGELLAN_MQTT_4G_BOARD::printServingCell()
{
  auto &modem = this->getGSMModem();

  // Serial.println("=== SERVING CELL ===");
  modem.sendAT("+CPSI?");
  String response;

  int8_t result = modem.waitResponse(
      3000L,
      response);

  // Serial.printf("CPSI status: %d\n", status);
  // Serial.println("Raw response:");
  // Serial.println(response);

  response.replace("OK", "");
  response.trim(); // Removes trailing \r and \n

  if (result != 1)
  {
    return "";
  }

  int start = response.indexOf("+CPSI:");
  if (start < 0)
  {
    return "";
  }

  int end = response.indexOf('\r', start);
  if (end < 0)
  {
    end = response.indexOf('\n', start);
  }

  return end >= 0 ? response.substring(start, end)
                  : response.substring(start);
  return response;
}

static const uint32_t CELLULAR_REGISTER_TIMEOUT_MS = 15000L;
static const uint32_t PDP_SETTLE_DELAY_MS = 1500;
static const uint32_t PRE_REINIT_DELAY_MS = 500;

static bool waitForCellularRegistration(TinyGsm &modem)
{
  MG_LOG_I("[reconnect] Waiting for cellular network...");
  if (!modem.waitForNetwork(CELLULAR_REGISTER_TIMEOUT_MS))
  {
    MG_LOG_E("[reconnect] Cellular network registration failed");
    return false;
  }
  MG_LOG_I("[reconnect] Cellular network registered");
  return true;
}

void MAGELLAN_MQTT_4G_BOARD::recoverCellular()
{
  MG_LOG_I("[recoverCellular] Handling reconnect...");
  TinyGsm &modem = this->getGSMModem();
  const IPAddress noIP(0, 0, 0, 0);

  IPAddress localIP = modem.localIP();
  MG_LOG_I("[Info] Local IP: %s", localIP.toString().c_str());

  LTE_Signal_INFO lteSignalInfo = this->radioSignal.getDetailedSignal();
  MG_LOG_I("[Info] LTE Signal Strength (RSSI): %d", lteSignalInfo.rssi);
  MG_LOG_I("[Info] LTE Signal Quality (RSRQ): %d", lteSignalInfo.rsrq);
  MG_LOG_I("[Info] LTE Signal RSRP: %d", lteSignalInfo.rsrp);
  MG_LOG_I("[Info] LTE Signal SINR: %d", lteSignalInfo.sinr);

  if (localIP != noIP)
  {
    MG_LOG_I("[recoverCellular Check] Modem has valid IP.");
    return;
  }

  if (!waitForCellularRegistration(modem))
  {
    return;
  }

  // gprsConnect() closes any stale PDP context first, so it also fixes "connected but 0.0.0.0".
  MG_LOG_I("[reconnect] Connecting PDP...");
  if (!modem.gprsConnect(_apn))
  {
    MG_LOG_E("[reconnect] PDP connection failed");
    return;
  }
  delay(PDP_SETTLE_DELAY_MS);

  localIP = modem.localIP();
  MG_LOG_I("[reconnect] IP: %s", localIP.toString().c_str());
  if (localIP == noIP)
  {
    MG_LOG_E("[reconnect] SIM7600E Recovery Second Check failed, IP is 0.0.0.0");
    return;
  }

  MG_LOG_I("[reconnect] GPRS connected");
  delay(PRE_REINIT_DELAY_MS);
  this->reinitializeGSM();
  if (!modem.isNetworkConnected())
  {
    waitForCellularRegistration(modem);
  }
}

//# Backup
// void MAGELLAN_MQTT_4G_BOARD::recoverCellular()
// {
//   MG_LOG_I("[recoverCellular] Handling reconnect...");
//   TinyGsm &_modem = this->getGSMModem();
//   IPAddress localIP = _modem.localIP();
//   MG_LOG_I("[Info] Local IP: %s", localIP.toString().c_str());

//   LTE_Signal_INFO lteSignalInfo = this->radioSignal.getDetailedSignal();
//   MG_LOG_I("[Info] LTE Signal Strength (RSSI): %d", lteSignalInfo.rssi);
//   MG_LOG_I("[Info] LTE Signal Quality (RSRQ): %d", lteSignalInfo.rsrq);
//   MG_LOG_I("[Info] LTE Signal RSRP: %d", lteSignalInfo.rsrp);
//   MG_LOG_I("[Info] LTE Signal SINR: %d", lteSignalInfo.sinr);

//   if (localIP == IPAddress(0, 0, 0, 0))
//   {
//     // MG_LOG_D("=== SIM7600E Recovery Check ===");
//     // MG_LOG_D("Network: %s", _modem.isNetworkConnected() ? "YES" : "NO");
//     // MG_LOG_D("GPRS: %s", _modem.isGprsConnected() ? "YES" : "NO");
//     // MG_LOG_D("IP: %s", localIP.toString().c_str());
//     // MG_LOG_D("CSQ: %d", _modem.getSignalQuality());

//     MG_LOG_I("[reconnect] Waiting for cellular network...");

//     if (!_modem.waitForNetwork(30000L))
//     {
//       MG_LOG_E("[reconnect] Cellular network registration failed");
//       return;
//     }

//     MG_LOG_I("[reconnect] Cellular network registered");

//     if (!_modem.isGprsConnected())
//     {
//       MG_LOG_I("[reconnect] Connecting PDP...");

//       if (!_modem.gprsConnect(_apn))
//       {
//         MG_LOG_E("[reconnect] PDP connection failed");
//         return;
//       }
//       // Let the PDP context settle before any DNS/TCP attempt reuses it,
//       // same as the library's own post-gprsConnect delay.
//       delay(1500);
//     }

//     MG_LOG_I("[reconnect] GPRS connected");
//     IPAddress _localIP = _modem.localIP();
//     MG_LOG_I("[reconnect] IP: %s", _localIP.toString().c_str());

//     if (_localIP != IPAddress(0, 0, 0, 0))
//     {
//       // MG_LOG_D("=== SIM7600E Recovery Second Check ===");
//       // MG_LOG_D("Network: %s", _modem.isNetworkConnected() ? "YES" : "NO");
//       // MG_LOG_D("GPRS: %s", _modem.isGprsConnected() ? "YES" : "NO");
//       // MG_LOG_D("IP: %s", _modem.localIP().toString().c_str());
//       // MG_LOG_D("CSQ: %d", _modem.getSignalQuality());

//       delay(500);
//       this->reinitializeGSM();

//       if (!_modem.isNetworkConnected())
//       {
//         MG_LOG_I("[reconnect] Waiting for cellular network...");

//         if (!_modem.waitForNetwork(30000L))
//         {
//           MG_LOG_E("[reconnect] Cellular network registration failed");
//           return;
//         }

//         MG_LOG_I("[reconnect] Cellular network registered");
//       }
//     }
//     else
//     {
//       MG_LOG_E("[reconnect] SIM7600E Recovery Second Check failed, IP is 0.0.0.0");
//     }
//   }
//   // MG_LOG_D("[Info] Serving Cell: %s", this->printServingCell().c_str());
//   IPAddress _localIP2 = _modem.localIP();
//   if (_localIP2 != IPAddress(0, 0, 0, 0))
//   {
//     // MG_LOG_D("=== SIM7600E Recovery Second Check 2 ===");
//     // MG_LOG_D("Network: %s", _modem.isNetworkConnected() ? "YES" : "NO");
//     // MG_LOG_D("GPRS: %s", _modem.isGprsConnected() ? "YES" : "NO");
//     // MG_LOG_D("IP: %s", _localIP2.toString().c_str());
//     // MG_LOG_D("CSQ: %d", _modem.getSignalQuality());

//     delay(500);
//     this->reinitializeGSM();

//     if (!_modem.isNetworkConnected())
//     {
//       MG_LOG_I("[handleReconnect 2] Waiting for cellular network...");

//       if (!_modem.waitForNetwork(30000L))
//       {
//         MG_LOG_E("[handleReconnect 2] Cellular network registration failed");
//         return;
//       }

//       MG_LOG_I("[handleReconnect 2] Cellular network registered");
//     }
//   }
// }

void MAGELLAN_MQTT_4G_BOARD::initGSM()
{
  MG_LOG_I("# ==== USE AIS 4G BOARD MODE INIT GSM ====");
  this->initSerialModem();
  // RESET FIRST
  if (!waitForModemATReady(10, 500))
  {
    MG_LOG_I("[RESET]Modem not ready, waiting...");
  }
  else
  {
    MG_LOG_I("[RESET]Modem communication ready");
  }

#ifndef SKIP_RESET_AT_BOOT
  MG_LOG_I_S(this->GSMModem.reset() ? "[RESET]Modem reset successfully" : "[RESET]Modem reset failed");
  unsigned long prvMillis = millis();
  while (millis() - prvMillis < 10000)
  {
    Serial.print(F("."));
    delay(100);
  }
  MG_LOG_I("[RESET]Modem restarted ready");
  _serialAtStarted = false;
#else
  MG_LOG_I("[RESET]Skipping modem reset at boot");
#endif // !SKIP_RESET_MODULE_AT_BOOT

  this->initSerialModem();
  // On ESP32 warm reset, SIM7600 can still be alive; avoid unnecessary power toggles.
  // On a full board power-cycle (unplug/replug), the modem cold-boots alongside the ESP32
  // and can take several seconds before it accepts AT - give it that time first, otherwise
  // this hard-cuts VBAT mid-bootup on nearly every cold start and makes things slower/flakier.
  if (!waitForModemATReady(10, 500))
  {
    MG_LOG_I("Modem not responding, toggling power key...");
    this->powerModem();
    // SIM7600 may need several seconds to boot and accept AT after PWRKEY pulse.
    delay(2000);
    this->initSerialModem();
  }

  if (!waitForModemATReady(12, 500))
  {
    MG_LOG_E("Modem AT not responding after power cycle. Restarting ESP...");
    ESP.restart();
    return;
  }

  this->connectModem();
  _getRadio();

  TinyGsm &_modem = this->GSMModem.getModem();
  MG_LOG_I("[NETWORK] Local IP: %s", _modem.localIP().toString().c_str());
#ifndef SKIP_RESOLVE_DOMAIN_AT_BOOT
  IPAddress IPbuff;
  bool resolveDonmain = this->resolveDomain(_host_production, IPbuff);
  MG_LOG_I_S(resolveDonmain ? "[NETWORK][DNS] Domain resolved successfully" : "[NETWORK][DNS] Domain resolved failed");
  MG_LOG_I("[NETWORK][DNS] Resolved IP: %s", IPbuff.toString().c_str());
#endif

  NetworkModuleMode mode = this->GSMModem.getNetworkMode();
  this->currentPreferedNetworkMode = mode;
  Serial.print(F("NetworkBand Mode: "));
  int networkMode = static_cast<int>(mode);
  Serial.println(this->GSMModem.networkModeToString(mode).c_str());
  MG_LOG_I("#==============================================");
}
void MAGELLAN_MQTT_4G_BOARD::begin(MagellanSetting _setting)
{
  this->initGSM();
  this->coreMQTT->prefixClient = "4G_TINY_B_";
  this->onReconnect([]() {});
  this->registerReconnectContinueHook();
#ifdef BYPASS_REQTOKEN
  if (_setting.ThingToken != "null" && _setting.ThingToken.length() > 25)
  {
    this->coreMQTT->setManualToken(_setting.ThingToken);
  }
  else
  {
    MG_LOG_E("# Invalid setting ThingToken");
    MG_LOG_I("# Define \"BYPASS_REQTOKEN\" but not setting ThingToken manual back into auto renew ThingToken mode");
  }
#endif

  if (_setting.clientBufferSize > _default_OverBufferSize)
  {
    MG_LOG_I_S("# You have set a buffer size greater than 8192, adjusts to: " + String(_default_OverBufferSize));
    this->coreMQTT->setMQTTBufferSize(_default_OverBufferSize);
    attr.calculate_chunkSize = _default_OverBufferSize / 2;
  }
  else
  {
    this->coreMQTT->setMQTTBufferSize(_setting.clientBufferSize);
    attr.calculate_chunkSize = _setting.clientBufferSize / 2;
  }

  size_t revertChunkToBufferSize = attr.calculate_chunkSize * 2;
  // ThingIdentifier(ICCID) and ThingSecret(IMSI) .
  _setting.ThingIdentifier.trim();
  _setting.ThingSecret.trim();
  _setting.IMEI.trim();

  if (_setting.ThingIdentifier == "null" || _setting.ThingSecret == "null")
  {
    _setting.ThingIdentifier = _modem.getSimCCID();
    delay(50);
    _setting.ThingSecret = _modem.getIMSI();
    delay(50);
    _setting.IMEI = _modem.getIMEI();
    delay(50);
    MG_LOG_D("============ Board Information ============");
    MG_LOG_D_S("ICCID: " + _setting.ThingIdentifier);
    MG_LOG_D_S("IMSI : " + _setting.ThingSecret);
    MG_LOG_I_S("IMEI : " + _setting.IMEI);
    setting = _setting;
  }
  // second validate after get information
  if (coreMQTT->CheckString_isDigit(_setting.ThingIdentifier) && coreMQTT->CheckString_isDigit(_setting.ThingSecret))
  {
    // Serial.println(F("=========== Prepare Credentials ============"));
    // Serial.print(F("ThingIdentifier: "));
    // Serial.println(_setting.ThingIdentifier);
    // Serial.print(F("ThingSecret: "));
    // Serial.println(_setting.ThingSecret);
    // Serial.print(F("IMEI: "));
    // Serial.println(_setting.IMEI);
    // Serial.println(F("============================================"));
    this->MAGELLAN_MQTT_TEMP::begin(_setting);
    setting = _setting;
  }
  else
  {
    MG_LOG_E("# ThingIdentifier(ICCID) or ThingSecret(IMSI) invalid value please check again");
    MG_LOG_D_S("# ThingIdentifier =>" + _setting.ThingIdentifier);
    MG_LOG_D_S("# ThingSecret =>" + _setting.ThingSecret);
    MG_LOG_E("# Restart board");
    delay(5000);
    ESP.restart();
  }

  if (setting.builtInSensor)
  {
    this->builtInSensor.begin();
  }

  this->pubstate();
}

void MAGELLAN_MQTT_4G_BOARD::pubstate()
{
  this->clientConfig.add("libVersion", String(lib_model_device) + "-" + String(lib_ver));
  this->clientConfig.add("preferredMode", this->GSMModem.networkModeToString(this->currentPreferedNetworkMode));
  this->clientConfig.save();
}

void MAGELLAN_MQTT_4G_BOARD::disconnect()
{
  this->MAGELLAN_MQTT_TEMP::disconnect();
}

void MAGELLAN_MQTT_4G_BOARD::reconnect()
{
  MG_LOG_I("# ==== USE AIS 4G BOARD MODE RECONNECT MQTT ====");
  this->MAGELLAN_MQTT_TEMP::reconnect();
}

void MAGELLAN_MQTT_4G_BOARD::loop()
{
  this->handleModemMagellan();
  this->MAGELLAN_MQTT_TEMP::loop();
}

void MAGELLAN_MQTT_4G_BOARD::Centric::begin(MagellanSetting _setting)
{
  this->parent->initGSM();
  this->parent->coreMQTT->prefixClient = "4G_TINY_B_";
  this->parent->connectModem();
  if (_setting.ThingIdentifier == "null" || _setting.ThingSecret == "null")
  {
    _setting.ThingIdentifier = _modem.getSimCCID();
    delay(50);
    _setting.ThingSecret = _modem.getIMSI();
    delay(50);
    _setting.IMEI = _modem.getIMEI();
    delay(50);
    MG_LOG_D_S("ICCID: " + _setting.ThingIdentifier);
    MG_LOG_D_S("IMSI : " + _setting.ThingSecret);
    MG_LOG_I_S("IMEI : " + _setting.IMEI);
    setting = _setting;
  }
  this->parent->onReconnect([this]()
                            { this->parent->recoverCellular(); });
  // Validate credentials
  if (coreMQTT->CheckString_isDigit(setting.ThingIdentifier) && coreMQTT->CheckString_isDigit(setting.ThingSecret))
  {
    MG_LOG_D_S("Centric ThingIdentifier: " + String(setting.ThingIdentifier));
    MG_LOG_D_S("Centric ThingSecret: " + String(setting.ThingSecret));

    parent->coreMQTT->setAuthMagellan(setting.ThingIdentifier, setting.ThingSecret, setting.IMEI);
    parent->coreMQTT->magellanCentric(this->_host.c_str(), this->_port);
    // Connect to MQTT broker with credentials
    MG_LOG_I("Connecting to Centric MQTT...");
  }
  else
  {
    MG_LOG_E("# Centric credentials invalid!");
    MG_LOG_D_S("# ThingIdentifier =>" + setting.ThingIdentifier);
    MG_LOG_D_S("# ThingSecret =>" + setting.ThingSecret);
    MG_LOG_E("# Restart board");
    delay(5000);
    ESP.restart();
  }

  if (setting.builtInSensor)
  {
    this->parent->builtInSensor.begin();
  }

  this->parent->pubstate();
}

int16_t MAGELLAN_MQTT_4G_BOARD::getSignalStrength()
{
  int rssiNomalized = _modem.getSignalQuality();
  int rssiDbm = mapRSSITodBm(rssiNomalized);
  return rssiDbm;
}

String MAGELLAN_MQTT_4G_BOARD::getRSSIQuality()
{
  int16_t dBm = _modem.getSignalQuality();
  return _getSignalStrengthCategory(dBm);
}

// GPS

GPS_Data MAGELLAN_MQTT_4G_BOARD::GPS_utils::getCurrentGPSData()
{
  TinyGsm &modem = this->parent->getGSMModem();
  GPS_Data data;
  if (this->gps_internal.gpsIsOn(modem))
  {
    this->gps_internal.gpsRead(modem, data);
  }
  this->_gpsData = data;
  return data;
}

boolean MAGELLAN_MQTT_4G_BOARD::GPS_utils::available()
{
  if (!this->isGPSinitialized)
  {
    this->begin();
  }
  TinyGsm &modem = this->parent->getGSMModem();
  return this->gps_internal.available(modem);
}
float MAGELLAN_MQTT_4G_BOARD::GPS_utils::readLatitude()
{
  if (!this->isGPSinitialized)
  {
    this->begin();
  }
  float _lat = 0.000000f;
  _lat = this->getCurrentGPSData().lat;
  return _lat;
}
float MAGELLAN_MQTT_4G_BOARD::GPS_utils::readLongitude()
{
  if (!this->isGPSinitialized)
  {
    this->begin();
  }
  float _lng = 0.000000f;
  _lng = this->getCurrentGPSData().lng;
  return _lng;
}
float MAGELLAN_MQTT_4G_BOARD::GPS_utils::readAltitude()
{
  if (!this->isGPSinitialized)
  {
    this->begin();
  }

  float _alt = 0.000000f;
  _alt = this->getCurrentGPSData().alt;
  return _alt;
}
float MAGELLAN_MQTT_4G_BOARD::GPS_utils::readSpeed()
{
  if (!this->isGPSinitialized)
  {
    this->begin();
  }
  float _spd = 0.000000f;
  _spd = this->getCurrentGPSData().speed;
  return _spd;
}
float MAGELLAN_MQTT_4G_BOARD::GPS_utils::readCourse()
{
  if (!this->isGPSinitialized)
  {
    this->begin();
  }
  float _course = 0.000000f;
  _course = this->getCurrentGPSData().course;
  return _course;
}
String MAGELLAN_MQTT_4G_BOARD::GPS_utils::readLocation()
{
  if (!this->isGPSinitialized)
  {
    this->begin();
  }
  String _location = "0.000000,0.000000";
  _location = String(this->readLatitude(), 6) + "," + String(this->readLongitude(), 6);
  return _location;
}
unsigned long MAGELLAN_MQTT_4G_BOARD::GPS_utils::getUnixTime()
{
  if (!this->isGPSinitialized)
  {
    this->begin();
  }
  unsigned long _unix = 0;
  _unix = this->getCurrentGPSData().utc;
  return _unix;
}

void MAGELLAN_MQTT_4G_BOARD::GPS_utils::disable()
{
  TinyGsm &modem = this->parent->getGSMModem();
  this->isGPSinitialized = false;
  this->gps_internal.gpsEnd(modem);
}
void MAGELLAN_MQTT_4G_BOARD::GPS_utils::begin()
{
  TinyGsm &modem = this->parent->getGSMModem();
  modem.enableGPS();
  delay(500);
  this->isGPSinitialized = true;
  this->gps_internal.gpsInit(modem);
}
void MAGELLAN_MQTT_4G_BOARD::GPS_utils::beginAGPS()
{
  TinyGsm &modem = this->parent->getGSMModem();
  modem.enableGPS();
  delay(500);
  this->isGPSinitialized = true;
  int retry = 0;
  while (this->gps_internal.gpsBeginAGPS(modem))
  {
    MG_LOG_I_S("AGPS initialization retry " + String(++retry) + "/10");
    if (retry >= 10)
    {
      MG_LOG_E("Max retries reached. Init AGPS Failed...");
      break;
    }
    delay(500);
  }
  if (retry < 10)
  {
    MG_LOG_I("AGPS initialized successfully.");
  }
}

// Built-in Sensor
void MAGELLAN_MQTT_4G_BOARD::BuiltinSensor::begin()
{
  Wire.begin();
  SHT40.begin();
}

float MAGELLAN_MQTT_4G_BOARD::BuiltinSensor::readTemperature()
{
  return SHT40.readTemperature();
}

float MAGELLAN_MQTT_4G_BOARD::BuiltinSensor::readHumidity()
{
  return SHT40.readHumidity();
}

LTE_Signal_INFO MAGELLAN_MQTT_4G_BOARD::SignalAnalysis::getDetailedSignal()
{
  LTE_Signal_INFO sig;

  // 1. ส่งคำสั่ง AT ผ่านท่อของ TinyGSM
  TinyGsm &modem = this->parent->getGSMModem();
  modem.sendAT("+CPSI?");

  String response = "";
  // รอการตอบกลับจากโมเด็มภายใน 2000 มิลลิวินาที
  if (modem.waitResponse(2000, response) == 1)
  {
    // นำข้อมูลมาตัดเอาเฉพาะบรรทัดที่มี +CPSI:
    int index = response.indexOf("+CPSI:");
    if (index >= 0)
    {
      String data = response.substring(index);
      data.replace("\r", "");
      data.replace("\n", "");

      // ตัวอย่างข้อมูล: +CPSI: LTE,Online,520-03,0x33A1,135372551,385,EUTRAN-band3,1850,5,5,-12,-82,-53,18
      // เราจะใช้การตัดคำด้วย Comma (,) เพื่อดึงตัวเลขท้ายประโยคมาใช้งาน
      int count = 0;
      int lastComma = 0;
      int nextComma = 0;

      String tokens[14]; // เก็บค่าแยกตามคอมมา

      while ((nextComma = data.indexOf(',', lastComma)) != -1 && count < 14)
      {
        tokens[count++] = data.substring(lastComma, nextComma);
        lastComma = nextComma + 1;
      }
      tokens[count] = data.substring(lastComma); // ตัวสุดท้าย (SINR)

      // ตรวจสอบว่าเป็นโหมด LTE ไหม และพาร์สข้อมูลตามตำแหน่งเลเยอร์
      if (tokens[0].indexOf("LTE") >= 0 && count >= 13)
      {
        sig.mode = "LTE";
        sig.band = tokens[6];               // EUTRAN-band
        sig.rsrq = tokens[10].toInt() / 10; // RSRQ
        sig.rsrp = tokens[11].toInt() / 10; // RSRP
        sig.rssi = tokens[12].toInt() / 10; // RSSI
        sig.sinr = tokens[13].toInt() / 10; // SINR

        // 0x033A
        sig.tac = strtoul(tokens[3].c_str(), nullptr, 0);

        // PCI
        sig.pci = tokens[5].toInt();
        // Cell ID
        sig.cellId = strtoul(tokens[4].c_str(), nullptr, 10);
        // 520-03
        int dash = tokens[2].indexOf('-');
        if (dash > 0)
        {
          sig.mcc = tokens[2].substring(0, dash);
          sig.mnc = tokens[2].substring(dash + 1);
        }
      }
    }
  }
  return sig;
}

NetworkModuleMode MAGELLAN_MQTT_4G_BOARD::ConnectivityModem::getNetworkMode()
{
  TinyGsm &modem = this->parent->getGSMModem();
  const int16_t mode = modem.getNetworkMode();

  switch (mode)
  {
  case static_cast<int>(NetworkModuleMode::GSM_2G_Only):
    return NetworkModuleMode::GSM_2G_Only;
  case static_cast<int>(NetworkModuleMode::WCDMA_3G_Only):
    return NetworkModuleMode::WCDMA_3G_Only;
  case static_cast<int>(NetworkModuleMode::LTE_4G_Only):
    return NetworkModuleMode::LTE_4G_Only;
  case static_cast<int>(NetworkModuleMode::Automatic):
    return NetworkModuleMode::Automatic;
  default:
    MG_LOG_E_S("Unsupported or unreadable +CNMP mode: " + String(mode));
    return NetworkModuleMode::Automatic;
  }
}

void MAGELLAN_MQTT_4G_BOARD::ConnectivityModem::setNetworkMode(NetworkModuleMode mode)
{
  TinyGsm &modem = this->parent->getGSMModem();
  const int networkMode = static_cast<int>(mode);

  if (networkMode != static_cast<int>(NetworkModuleMode::Automatic) &&
      networkMode != static_cast<int>(NetworkModuleMode::GSM_2G_Only) &&
      networkMode != static_cast<int>(NetworkModuleMode::WCDMA_3G_Only) &&
      networkMode != static_cast<int>(NetworkModuleMode::LTE_4G_Only))
  {
    MG_LOG_E_S("Unsupported +CNMP mode requested: " + String(networkMode));
    return;
  }

  if (!modem.setNetworkMode(static_cast<uint8_t>(networkMode)))
  {
    MG_LOG_E_S("Failed to set +CNMP mode: " + String(networkMode));
    return;
  }

  MG_LOG_I_S("Network mode set to +CNMP=" + String(networkMode));
}

String MAGELLAN_MQTT_4G_BOARD::ConnectivityModem::networkModeToString(NetworkModuleMode mode)
{
  switch (mode)
  {
  case NetworkModuleMode::GSM_2G_Only:
    return "[ONLY GSM 2G]";
  case NetworkModuleMode::WCDMA_3G_Only:
    return "[ONLY WCDMA 3G]";
  case NetworkModuleMode::LTE_4G_Only:
    return "[ONLY LTE 4G]";
  case NetworkModuleMode::Automatic:
    return "[Automatic]";
  default:
    return "UNKNOWN (" + String(static_cast<int>(mode)) + ")";
  }
}

bool MAGELLAN_MQTT_4G_BOARD::ConnectivityModem::reset()
{
  TinyGsm &modem = this->parent->getGSMModem();
  modem.sendAT("+CRESET");
  String response;
  int8_t status = modem.waitResponse(5000L, response);
  // response.trim();
  MG_LOG_I("[ConnectivityModem]Modem RESET:");
  MG_LOG_I_S(response);
  if (response.indexOf("OK") != -1)
    return true;
  return false;
}

void MAGELLAN_MQTT_4G_BOARD::ConnectivityModem::begin()
{
  this->parent->initGSM();
}
void MAGELLAN_MQTT_4G_BOARD::ConnectivityModem::handle()
{
  // MG_LOG_I("[ConnectivityModem]Handling modem...");
  this->parent->checkModem();
}
TinyGsmClient &MAGELLAN_MQTT_4G_BOARD::ConnectivityModem::getClient()
{
  return this->parent->getGSMClient();
}
TinyGsm &MAGELLAN_MQTT_4G_BOARD::ConnectivityModem::getModem()
{
  return this->parent->getGSMModem();
}

#ifdef BYPASS_REQTOKEN
void MAGELLAN_MQTT_4G_BOARD::setManualToken(String token_)
{
  this->coreMQTT->setManualToken(token_);
}
#endif

void MAGELLAN_MQTT_4G_BOARD::reinitializeGSM()
{
  TinyGsm &_modem = this->getGSMModem();
  for (int i = 0; i < 15; i++) //
  {
    if (_modem.testAT(1000))
    {
      MG_LOG_I("[reinitializeGSM]Modem responded by command. [%d]", i + 1);
      break;
    }
    delay(500);
  }
  MG_LOG_I("[reinitializeGSM]Reinitializing GSM...");
  this->initGSM();
}
// Registers the coreMQTT hook exactly once. Safe to call multiple times
// (from begin() and from onReconnect()) since the guard prevents re-wrapping
int _countReconnect = 0;
void MAGELLAN_MQTT_4G_BOARD::onReconnect(cb_on_reconnect cb_recon)
{
  // Store the user's callback regardless of call order relative to begin();
  // the coreMQTT hook itself is registered separately (see registerReconnectHook()).
  _userReconnectCb = cb_recon;
  cb_on_reconnect middle_cb_recon = cb_on_reconnect([this]()
                                                    {
    this->recoverCellular();
    _countReconnect++;
    if (_countReconnect > MAX_RECONNECT_RST_MODEM){
      _countReconnect = 0;
      GSMModem.reset();
      delay(5000);
    }
    if (this->_userReconnectCb)
    {
      this->_userReconnectCb();
    } });
  this->coreMQTT->onReconn(middle_cb_recon);
}

// Registers the coreMQTT continue-hook exactly once (see registerReconnectHook()).
void MAGELLAN_MQTT_4G_BOARD::registerReconnectContinueHook()
{
  if (_reconnectContinueHookRegistered)
  {
    return;
  }
  cb_on_reconnect middle_cb_recon = cb_on_reconnect([this]()
                                                    {
    if (this->_userReconnectContinueCb)
    {
      this->_userReconnectContinueCb();
    } });
  this->coreMQTT->onReconnContinue(middle_cb_recon);
  _reconnectContinueHookRegistered = true;
}

void MAGELLAN_MQTT_4G_BOARD::onReconnectingLoop(cb_on_reconnect cb_recon_continue)
{
  _userReconnectContinueCb = cb_recon_continue;
  this->registerReconnectContinueHook();
}
