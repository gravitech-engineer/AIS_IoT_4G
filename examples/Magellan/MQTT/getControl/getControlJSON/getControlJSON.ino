#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>

MAGELLAN_SIM7600E_MQTT magel;
void setup() 
{
  Serial.begin(115200);
  magel.begin(); 
  //{1.} json string
  magel.getControlJSON([](String payload){
    Serial.print("# Control incoming JSON: ");
    Serial.println(payload);
    String control = magel.deserializeControl(payload);
    magel.control.ACK(control); //ACKNOWLEDGE control to magellan
  });
  //or {2.} json object
  // magel.getControlJSON([](JsonObject docObject){
  //   String Lamp1 = docObject["Lamp1"]; // buffer value form control key "Lamp1" if not found this key value is "null"
  //   if(Lamp1.indexOf("null") == -1)
  //   {
  //       Serial.print("# Control incoming JSON Object: ");
  //       Serial.print("# [Key] => Lamp1: ");
  //       Serial.println(Lamp1);
  //       magel.sensor.add("Lamp1", Lamp1);
  //       magel.control.ACK(magel.sensor.toJSONString()); //ACKNOWLEDGE control to magellan
  //   }
  // });

  // prepare sensor to magellan by report control key with inial value
  magel.report.send("Lamp1","0");
}

void loop() 
{
  magel.loop();
  magel.subscribes([](){
    magel.subscribe.control(); // subscribe server control content type JSON
  });
  magel.interval(10,[](){ //time interval function inside every 10 second

  });
}
