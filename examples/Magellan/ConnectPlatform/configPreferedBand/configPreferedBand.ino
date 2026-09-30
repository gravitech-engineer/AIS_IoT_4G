#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>

MAGELLAN_SIM7600E_MQTT magel;

void setup()
{
  Serial.begin(115200);
  magel.begin(setting);

  NetworkModuleMode mode = magel.GSMModem.getNetworkMode();
  Serial.println(F("==== Network Module Mode Report ==="));
  String networkMode = "UNKNOWN";

  if (mode != NetworkModuleMode::LTE_4G_Only)
  {
    Serial.println(F("Forcing to Only [LTE 4G] mode"));
    magel.GSMModem.setNetworkMode(NetworkModuleMode::LTE_4G_Only);
    ESP.restart();
  }

  networkMode = magel.GSMModem.networkModeToString(mode);
  Serial.print(F("Network Mode: "));
  Serial.println(networkMode);
  Serial.println(F("=================================="));
}

void loop()
{
  magel.loop();
  magel.subscribesHandler();
  magel.interval(10, []
                 {
                   magel.sensor.add("Board_Temp", magel.builtInSensor.readTemperature());
                   magel.sensor.add("Board_Humid", magel.builtInSensor.readHumidity());
                 
                   magel.sensor.report();
  });
}
