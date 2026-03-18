#include <Arduino.h>
#include <MAGELLAN_MQTT_4G_BOARD.h>

#define LED_BUILTIN 15

MAGELLAN_MQTT_4G_BOARD magel;
OTA_state checkStatusUpdate = OTA_state::UNKNOWN_STATE;

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  magel.begin();
  magel.getResponse(RESP_REPORT_JSON, [](EVENTS events) { // optional for make sure report success CODE = 20000
    Serial.print("[RESP REPORT] code: ");
    Serial.println(events.CODE); // follow status code on https://magellan.ais.co.th/api-document/3/0 {Error code topic}
    Serial.print("# [RESP REPORT] response message: ");
    Serial.println(events.RESP);
  });

  magel.getControl([](String key, String value)
  {
    Serial.println("Control incoming - [Key]:" + key + " [Value]" + value);
    // Control turn ON OFF Relay1
    if (key == "Relay1" || key == "Lamp1")
    {
      if (value == "1")
      {
        digitalWrite(LED_BUILTIN, HIGH); // set status ON OFF Relay Channel 1
      }
      else
      {
        digitalWrite(LED_BUILTIN, LOW); // set status ON OFF Relay Channel 1
      }

    magel.control.ACK(key, value); // ACKNOWLEDGE Control back to magellan
    }
  });
}

void loop()
{
  magel.loop();
  magel.subscribesHandler([](){
    Serial.println("=== Subscribe Handler ===");
    // doing once after reconnected
  });
  magel.heartbeat(10); // send heartbeat every 10 sec

  magel.interval(10, []() { // time interval function inside every 10000 millis
    Serial.println("=== Reporting ===");
    magel.sensor.add("GPS", "13.777864,100.544068");
    magel.sensor.add("random", (int)random(0, 100));
    magel.sensor.add("Humidity", (int)random(0, 100));
    magel.sensor.report();
  });
}
