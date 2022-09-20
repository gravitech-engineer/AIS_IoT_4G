#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>
MAGELLAN_SIM7600E_MQTT magel;
void setup() 
{
  Serial.begin(115200);
  magel.begin(); 
  magel.getResponse(RESP_REPORT_JSON, [](EVENTS events){  // optional for make sure report success CODE = 20000
    Serial.print("[RESP REPORT] code: ");
    Serial.println(events.CODE);// follow status code on https://magellan.ais.co.th/api-document/3/0 {Error code topic}
    Serial.print("# [RESP REPORT] response message: ");
    Serial.println(events.RESP);
  });
}

void loop() 
{
  magel.loop();
  magel.subscribes([]()
  {
    magel.subscribe.report.response(); // optional register for get Resp report
  });
  magel.interval(10, [](){   //time interval function inside every 10 second
    //{1} 
    delay(50);
    String Temperature = String(magel.builtInSensor.readTemperature());
    String Humidity = String(magel.builtInSensor.readHumidity());
    magel.sensor.add("Temperature", magel.builtInSensor.readTemperature()); 
    magel.sensor.add("Humidity", magel.builtInSensor.readHumidity());
    magel.sensor.report(); //send data sensor with auto json build
    //or {2}
    delay(50);
    magel.report.send("{\"Temperature_manual\":"+Temperature+",\"Humidity_manual\":"+Humidity+"}"); //send data sensor with manual json format
    //or {3}
    delay(50);
    magel.sensor.add("Temperature_buff", magel.builtInSensor.readTemperature()); 
    magel.sensor.add("Humidity_buff", magel.builtInSensor.readHumidity());
    String buffer_data = magel.sensor.toJSONString(); //json build sensor to buffer
    magel.report.send(buffer_data); //send data sensor with buffer json format
  });
}

