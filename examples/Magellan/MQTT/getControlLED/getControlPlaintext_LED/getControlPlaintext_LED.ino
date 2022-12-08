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
  //{1} control get any key and value
  magel.getControl([](String key, String value){
    Serial.print("# Control incoming\n# [Key]: ");
    Serial.println(key);
    Serial.print("# [Value]: ");
    Serial.println(value);
    if(key == "Lamp1")
    {
      if(value == "1")
      {
        digitalWrite(LED_E15, HIGH);
        Serial.println("LED ON");
        magel.control.ACK("Lamp1", String(digitalRead(LED_E15))); //ACKNOWLEDGE control to magellan
      }
      else if (value == "0")
      {
        digitalWrite(LED_E15, LOW);
        Serial.println("LED OFF");
        magel.control.ACK("Lamp1", String(digitalRead(LED_E15))); //ACKNOWLEDGE control to magellan
      }   
    }
    else // ACKNOWLEDGE control to magellan form another control focus
    {
      magel.control.ACK(key, value); //ACKNOWLEDGE control to magellan  
    }
  });
  //or {2} get control by focus on 1 key optional
  // magel.getControl("Lamp1", [](String value){ // focus only value form key "Lamp1"
  //   Serial.print("# Control incoming focus on [Key] Lamp1: ");
  //   Serial.println(value);
  //   if(value == "1")
  //   {
  //     digitalWrite(LED_E15, HIGH);
  //     Serial.println("LED ON");
  //     magel.control.ACK("Lamp1", String(digitalRead(LED_E15))); //ACKNOWLEDGE control to magellan
  //   }
  //   else if (value == "0")
  //   {
  //     digitalWrite(LED_E15, LOW);
  //     Serial.println("LED OFF");
  //     magel.control.ACK("Lamp1", String(digitalRead(LED_E15))); //ACKNOWLEDGE control to magellan
  //   }  
  // });

  // prepare sensor to magellan by report control key with inial value
  magel.report.send("Lamp1","0");
}

void loop() 
{
  magel.loop();
  magel.subscribes([](){
    magel.subscribe.control(PLAINTEXT); // subscribe server config content type JSON
  });
  magel.interval(10,[](){ //time interval function inside every 10 second

  });
}
