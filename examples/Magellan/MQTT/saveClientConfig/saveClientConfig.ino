#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>
MAGELLAN_SIM7600E_MQTT magel;
void setup() 
{
  Serial.begin(115200);
  magel.begin(); 
  magel.clientConfig.add("Device_number", 142);
  magel.clientConfig.add("Location", "19.346641, 52.245513");
  magel.clientConfig.add("Installation_date", "2022-07-01");
  magel.clientConfig.add("Interval_setting", "10sec");
  magel.clientConfig.save();
}

void loop() 
{
  magel.loop();
}