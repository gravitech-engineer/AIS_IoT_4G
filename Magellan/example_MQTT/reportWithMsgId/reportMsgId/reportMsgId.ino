#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>

MAGELLAN_SIM7600E_MQTT magel;
void setup() 
{
  Serial.begin(115200);
  magel.begin();
  magel.getResponse(RESP_REPORT_JSON, [](EVENTS events){  // optional for make sure report success CODE = 20000
    Serial.println("\n =============== Callback ============== ");
    Serial.print("# [RESP REPORT] code: ");
    Serial.println(events.CODE); // follow status code on https://magellan.ais.co.th/api-document/3/0 {Error code topic}
    Serial.print("# [RESP REPORT] response message: ");
    Serial.println(events.RESP);
    Serial.print("# [MsgId] : ");
    Serial.println(events.MsgId);
    Serial.println(" =============== Callback ============== \n");
  });
}

void loop() 
{
  magel.loop();
  magel.subscribes([]()
  {
    magel.subscribe.report.response(); // optional register for get Resp report
  });
  magel.interval(10, [](){

    //{1.} auto buildJSON but manual report
    int MsgId = magel.report.generateMsgId(); //generate message id
    magel.sensor.add("GPS", "13.777864,100.544068");
    magel.sensor.add("random", (int)random(0, 100));
    magel.sensor.add("Humidity", (int)random(0, 100));
    String payload = magel.sensor.toJSONString();
    magel.sensor.clear();
    magel.report.send(payload, MsgId);

    //{2.} manual report
    MsgId = magel.report.generateMsgId(); //generate message id
    magel.report.send("{\"hello\":\"world\"}", MsgId);
  });
}