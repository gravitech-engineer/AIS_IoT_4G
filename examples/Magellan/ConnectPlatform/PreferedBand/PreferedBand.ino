#include <Arduino.h>

#include <MAGELLAN_SIM7600E_MQTT.h>
MAGELLAN_SIM7600E_MQTT board;
MAGELLAN_SIM7600E_MQTT::ConnectivityModem &gsmBoard = board.GSMModem;

void setup()
{
  Serial.begin(115200);
  gsmBoard.begin();
  NetworkModuleMode mode = board.GSMModem.getNetworkMode();

  Serial.println(F("==== Network Module Mode Report ==="));
  Serial.println(F("Network Module Mode: "));
  int networkMode = static_cast<int>(mode);
  String modeStr = "UNKNOWN";

  //Force LTE 4G Only mode if the current mode is Automatic (uncomment)
  // if (mode == NetworkModuleMode::Automatic)
  // {
  //   Serial.println(F("Forcing LTE 4G Only mode..."));
  //   board.GSMModem.setNetworkMode(NetworkModuleMode::LTE_4G_Only);
  //   ESP.restart();
  // }

  switch (mode)
  {
  case NetworkModuleMode::GSM_2G_Only:
    Serial.println(F("GSM 2G Only"));
    modeStr = "2G_ONLY";
    break;
  case NetworkModuleMode::WCDMA_3G_Only:
    Serial.println(F("WCDMA 3G Only"));
    modeStr = "3G_ONLY";
    break;
  case NetworkModuleMode::LTE_4G_Only:
    Serial.println(F("LTE 4G Only"));
    modeStr = "4G_ONLY";
    break;
  case NetworkModuleMode::Automatic:
    Serial.println(F("Automatic"));
    modeStr = "AUTOMATIC";
    break;
  default:
    Serial.print(F("Unsupported or unreadable +CNMP mode: "));
    Serial.println(networkMode);
    modeStr = "UNKNOWN";
    break;
  }
}

void loop()
{
}
