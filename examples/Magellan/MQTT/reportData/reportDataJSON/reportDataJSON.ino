#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>

MAGELLAN_SIM7600E_MQTT magel;
void setup() 
{
  Serial.begin(115200);
  magel.begin();
  magel.getResponse(RESP_REPORT_JSON, [](EVENTS events){  // optional for make sure report success CODE = 20000
    Serial.print("[RESP REPORT] code: ");
    Serial.println(events.CODE); // follow status code on https://magellan.ais.co.th/api-document/3/0 {Error code topic}
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
  magel.interval(10, [](){ //time interval function inside every 10 second
    //{1.} auto buildJSON and reportJSON
    magel.sensor.add("Location", "13.777864,100.544068");
    magel.sensor.add("random", (int)random(0, 100));
    magel.sensor.add("temperature", (int)random(25, 34));
    magel.sensor.report();  // this function serializeJson from addSensor to Json format and report

    //{2.} auto buildJSON but manaul report
    magel.sensor.add("GPS", "13.777864,100.544068");
    magel.sensor.add("random", (int)random(0, 100));
    magel.sensor.add("Humidity", (int)random(0, 100));
    String payload = magel.sensor.toJSONString();
    magel.report.send(payload);

    //{3.} manaul report
    magel.report.send("{\"hello\":\"world\"}");
  });
}
