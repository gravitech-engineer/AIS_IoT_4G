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
  magel.interval(10, [](){ //time interval function inside every 10 second
    magel.report.send("temperature", String(random(25, 34)));
    magel.report.send("hello", "world");
  });
}
