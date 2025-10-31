#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>
MAGELLAN_SIM7600E_MQTT magel;

OTA_state checkStatusUpdate = OTA_state::UNKNOWN_STATE;
void setup() 
{
  Serial.begin(115200);
  magel.OTA.autoUpdate(); // this function ENABLED by default unless you set FALSE
  setting.clientBufferSize = defaultOTABuffer; // set buffer size compatible for OTA
  magel.begin(setting); 
}

void loop() 
{
  magel.loop();
  magel.subscribes([](){
    checkStatusUpdate = magel.OTA.checkUpdate();
    // subscribe function here!
  });
  magel.interval(10,[](){ //time interval function inside every 10000 millis
    // doing function something every 10 sec here!
    switch (checkStatusUpdate)
    {
    case OTA_state::UP_TO_DATE:
      Serial.print(F("checkStatusUpdate: "));
      Serial.println("# UP_TO_DATE");
      break;
    case OTA_state::OUT_OF_DATE:
      Serial.print(F("checkStatusUpdate: "));
      Serial.println(F("# OUT_OF_DATE"));
      break;   
    default:
      Serial.print(F("checkStatusUpdate: "));
      Serial.println("# UNKNOWN");
      break;
    }
  });
}