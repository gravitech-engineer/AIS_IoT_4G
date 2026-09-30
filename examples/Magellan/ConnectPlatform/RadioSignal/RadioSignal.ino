#include <Arduino.h>

#include <MAGELLAN_SIM7600E_MQTT.h>
MAGELLAN_SIM7600E_MQTT board;
MAGELLAN_SIM7600E_MQTT::ConnectivityModem &gsmBoard = board.GSMModem;

void setup()
{
  Serial.begin(115200);
  gsmBoard.begin();

  NetworkModuleMode NetworkMode = board.GSMModem.getNetworkMode();
  Serial.println(F("==== Network Module Mode Report ==="));
  Serial.print(F("Network Module Mode: "));
  String NetworkModeStr = board.GSMModem.networkModeToString(NetworkMode);
  Serial.println(NetworkModeStr);
  Serial.println(F("==================================="));
}

void loop()
{
  gsmBoard.handle();         // check modem status and reconnect if needed
  LTE_Signal_INFO signalInfo = board.radioSignal.getDetailedSignal(); // get signal strength and quality
  Serial.print("=================================");
  Serial.print("Signal Mode: ");
  Serial.println(signalInfo.mode);
  Serial.print("Signal Band: ");        
  Serial.println(signalInfo.band);
  Serial.print("Signal RSRQ: ");
  Serial.println(signalInfo.rsrq);
  Serial.print("Signal RSRP: ");
  Serial.println(signalInfo.rsrp);
  Serial.print("Signal RSSI: ");
  Serial.println(signalInfo.rssi);
  Serial.print("Signal SINR: ");
  Serial.println(signalInfo.sinr);
  Serial.print("=================================");
  delay(5000); // wait for 5 seconds before checking again
}
