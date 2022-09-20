#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>
#define LED_E15 15 //LED Builtin Board 4G
MAGELLAN_SIM7600E_MQTT magel;
void setup() 
{
  Serial.begin(115200);
  magel.begin(); 
  pinMode(LED_E15, OUTPUT);
  digitalWrite(LED_E15, LOW);

  //{1} JSON String
  magel.getControlJSON([](String payload){
    Serial.print("# Control incoming JSON: ");
    Serial.println(payload);
    String control = magel.deserializeControl(payload);
    if(control == "{\"Lamp1\":1}")
    {
      digitalWrite(LED_E15, HIGH);
      Serial.println("LED ON");
      magel.sensor.add("Lamp1", digitalRead(LED_E15));
      magel.control.ACK(magel.sensor.toJSONString()); //ACKNOWLEDGE control to magellan
    }
    else if (control == "{\"Lamp1\":0}")
    {
      digitalWrite(LED_E15, LOW);
      Serial.println("LED OFF");
      magel.sensor.add("Lamp1", digitalRead(LED_E15));
      magel.control.ACK(magel.sensor.toJSONString()); //ACKNOWLEDGE control to magellan
    }  
    else
    {
      magel.control.ACK(control); //ACKNOWLEDGE control to magellan if another control
    }

  });
  //or {2} JSON Object
  // magel.getControlJSON([](JsonObject docObject){
  //   String Lamp1 = docObject["Lamp1"]; // buffer value form control key "Lamp1" if not found this key value is "null"
  //   if(Lamp1.indexOf("null") == -1)
  //   {
  //       Serial.print("# Control incoming JSON Object: ");
  //       Serial.print("# [Key] => Lamp1: ");
  //       Serial.println(Lamp1);
  //       if(Lamp1 == "1")
  //       {
  //         digitalWrite(LED_E15, HIGH);
  //         Serial.println("LED ON");
  //         magel.sensor.add("Lamp1", digitalRead(LED_E15));
  //         magel.control.ACK(magel.sensor.toJSONString()); //ACKNOWLEDGE control to magellan
  //       }
  //       else if (Lamp1 == "0")
  //       {
  //         digitalWrite(LED_E15, LOW);
  //         Serial.println("LED OFF");
  //         magel.sensor.add("Lamp1", digitalRead(LED_E15));
  //         magel.control.ACK(magel.sensor.toJSONString()); //ACKNOWLEDGE control to magellan
  //       }
  //   }
  // });

  // prepare sensor to magellan by report control key with inial value
  magel.report.send("Lamp1","0");
}

void loop() 
{
  magel.loop();
  magel.subscribes([](){
    magel.subscribe.control(); // subscribe server config content type JSON
  });
  magel.interval(10,[](){ //time interval function inside every 10 second

  });
}
