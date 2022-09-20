#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>

MAGELLAN_SIM7600E_MQTT magel;

void setup() 
{
  Serial.begin(115200);
  magel.begin(); 
  //{1.} control receive key and value from control
  magel.getControl([](String key, String value){
    Serial.print("# Control incoming\n# [Key]: ");
    Serial.println(key);
    Serial.print("# [Value]: ");
    Serial.println(value);
    magel.control.ACK(key, value); //ACKNOWLEDGE control to magellan
  });
  //or {2.} control receive only value from focus key "Lamp1" control
  // magel.getControl("Lamp1", [](String value){ // focus only value form key "Lamp1"
  //   Serial.print("# Control incoming focus on [Key] Lamp1: ");
  //   Serial.println(value);
  //   magel.control.ACK("Lamp1", value); //ACKNOWLEDGE control to magellan
  // });
  
  // prepare sensor to magellan by report control key with inial value
  magel.report.send("Lamp1","0");
}


void loop() 
{
  magel.loop();
  magel.subscribes([](){
    magel.subscribe.control(PLAINTEXT); // subscribe server control content type PLAINTEXT
  });
  magel.interval(10,[](){ //time interval function inside every 10 second

  });
}
