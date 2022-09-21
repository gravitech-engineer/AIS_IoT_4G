#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>
MAGELLAN_SIM7600E_MQTT magel;
void setup() 
{
  Serial.begin(115200);
  magel.begin(); 
  magel.getResponse(RESP_HEARTBEAT_JSON, [](EVENTS events){ // focus only Event RESP_HEARTBEAT_JSON
    Serial.print("# Response incoming focus on [HEARTBEAT] Code: ");
    Serial.println(events.CODE);// follow status code on https://magellan.ais.co.th/api-document/3/0 {Error code topic}  
    Serial.print("# [HEARTBEAT] response message: ");
    Serial.println(events.RESP);
  });
}

void loop() 
{
  magel.loop();
  magel.subscribes([](){
    magel.subscribe.heartbeat.response(); // subscribe server config content type JSON
  });
  magel.interval(10,[](){ //time interval function inside every 10 second

  });
  magel.heartbeat(10); // tringger heartbeat to magellan every 10 second
}
