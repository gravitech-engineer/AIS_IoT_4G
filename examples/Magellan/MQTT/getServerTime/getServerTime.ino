#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>
MAGELLAN_SIM7600E_MQTT magel;
int unixTimeMG;
void setup()
{
    Serial.begin(115200);
    magel.begin();
    
    magel.getResponse(UNIXTIME, [](EVENTS events){  // for get unixTime from magellan
    unixTimeMG = events.Payload.toInt();
    Serial.print("[unixTimeMG from magellan]: ");
    Serial.println(unixTimeMG);// this unixTime to use!
    });
}


void loop() 
{
  magel.loop();
  magel.subscribes([]()
  {
    magel.subscribe.getServerTime(PLAINTEXT);
  });
  magel.interval(5, [](){  //time interval function inside every 5 second
    magel.getServerTime(); // request time from magellan server
  });
}
