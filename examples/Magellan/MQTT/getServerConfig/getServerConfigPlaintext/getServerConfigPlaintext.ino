#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>

MAGELLAN_SIM7600E_MQTT magel;

void setup() 
{
  Serial.begin(115200);
  magel.begin(); 
  //{1.}
  magel.getServerConfig([](String key, String value){
    Serial.print("# Config incoming\n# [Key]: ");
    Serial.println(key);
    Serial.print("# [Value]: ");
    Serial.println(value);
  });
  //or {2.} focus on key "DELAY"
  // magel.getServerConfig("DELAY", [](String value){ // focus only value form key "Delay"
  //   Serial.print("# Config incoming focus on [Key] Delay: ");
  //   Serial.println(value);
  // });
}

void loop() 
{
  magel.loop();
  magel.subscribes([](){
    magel.subscribe.serverConfig(PLAINTEXT); // subscribe server config content type PLAINTEXT
  });
  magel.interval(10,[](){ //time interval function inside every 10 second
    magel.serverConfig.request("DELAY"); // request server config content type PLAINTEXT
    magel.serverConfig.request("INTERVAL"); // request server config content type PLAINTEXT
    magel.serverConfig.request("SSID"); // request server config content type PLAINTEXT
  });
}
