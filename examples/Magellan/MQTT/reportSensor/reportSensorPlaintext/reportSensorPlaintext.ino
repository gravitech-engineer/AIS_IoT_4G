#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>
MAGELLAN_SIM7600E_MQTT magel;
void setup() 
{
  Serial.begin(115200);
  magel.begin(); 
  magel.getResponse(RESP_REPORT_PLAINTEXT, [](EVENTS events){  // optional for make sure report success CODE = 20000
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
    magel.subscribe.report.response(PLAINTEXT); // optional register for get Resp report
  });
  magel.interval(10, [](){   //time interval function inside every 10 second
    delay(50);
    String Temperature = String(magel.builtInSensor.readTemperature());
    String Humidity = String(magel.builtInSensor.readHumidity());
    magel.report.send("Temperature", Temperature); 
    magel.report.send("Humidity", Humidity);
  });
}

