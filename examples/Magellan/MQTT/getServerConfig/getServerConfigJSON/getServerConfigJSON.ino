#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>

MAGELLAN_SIM7600E_MQTT magel;

void setup() 
{
  Serial.begin(115200);
  magel.begin(); 
  //{1.} json string
  magel.getServerConfigJSON([](String payload){
    Serial.print("# Config incoming JSON: ");
    Serial.println(payload);
  });
  //or {2.} json object
  // magel.getServerConfigJSON([](JsonObject docObject){
  //   String buffer = docObject["DELAY"]; // buffer value form config key "DELAY" if not found this key value is "null"
  //   if(buffer.indexOf("null") == -1)
  //   {
  //       Serial.print("# Config incoming JSON Object: ");
  //       Serial.print("# [Key] => Delay: ");
  //       Serial.println(buffer);
  //   }
  // });
}


void loop() 
{
  magel.loop();
  magel.subscribes([](){
    magel.subscribe.serverConfig(); // subscribe server config content type JSON
  });
  magel.interval(10,[](){ //time interval function inside every 10 second
    magel.serverConfig.request(); // request server config content type JSON
  });
}
