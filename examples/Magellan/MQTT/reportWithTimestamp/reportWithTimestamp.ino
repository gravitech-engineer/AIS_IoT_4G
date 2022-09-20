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
    Serial.println(unixTimeMG);
    });

    magel.getResponse(RESP_REPORT_TIMESTAMP, [](EVENTS events){  // optional for make sure report with timestamp success CODE = 20000
    Serial.print("[ReportWithTimestamp success code]: ");
    Serial.println(events.CODE);// follow status code on https://magellan.ais.co.th/api-document/3/0 {Error code topic}
    });
}


void loop() 
{
  magel.loop();
  magel.subscribes([]()
  {
    magel.subscribe.getServerTime(PLAINTEXT);
    magel.subscribe.reportWithTimestamp.response();
  });
  magel.interval(5, [](){  //time interval function inside every 5 second
    magel.getServerTime(); // request time from magellan server
    if(unixTimeMG > 0) //if get Timestamp from magellan send data
    {
      magel.sensor.add("Temperature", magel.builtInSensor.readTemperature()); 
      magel.sensor.add("Humidity", magel.builtInSensor.readHumidity()); 
      String payload = magel.sensor.toJSONString(); // build sensor value to json string
      //report send data sensor to magellan with timestamp.
      magel.report.send(unixTimeMG, payload);
      magel.sensor.clear(); unixTimeMG = 0; // clear buffer sensor and reset unixTimeMG
    }
  });
}
