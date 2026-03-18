#include <Arduino.h>
#include <MAGELLAN_MQTT_4G_BOARD.h>

MAGELLAN_MQTT_4G_BOARD magel;

void setup()
{
  Serial.begin(115200);
  magel.begin();
  magel.getResponse(RESP_REPORT_JSON, [](EVENTS events) { // optional for make sure report success CODE = 20000
    Serial.print("[RESP REPORT] code: ");
    Serial.println(events.CODE); // follow status code on https://magellan.ais.co.th/api-document/3/0 {Error code topic}
    Serial.print("# [RESP REPORT] response message: ");
    Serial.println(events.RESP);
  });
  magel.gps.begin();
}

void loop()
{
  magel.loop();
  magel.subscribesHandler([](){
    Serial.println("=== Subscribe Handler ===");
    // doing once after reconnected
  });

  magel.interval(10, []() { // time interval function inside every 10000 millis
    Serial.println("=== Reporting ===");
    bool gpsAvailable = magel.gps.available();
    if(gpsAvailable){
      const int timezone = 7; // GMT+7
      unsigned long unixTime = magel.gps.getUnixTime();
      magel.sensor.add("GPS_Location", magel.gps.readLocation());
      magel.sensor.add("GPS_UnixTime", String(unixTime));
      magel.sensor.add("GPS_DateTime", magel.utils.toDateTimeString(unixTime, timezone));
      magel.sensor.add("GPS_Speed", magel.gps.readSpeed());
      magel.sensor.add("GPS_Altitude", magel.gps.readAltitude());
      magel.sensor.add("GPS_Course", magel.gps.readCourse());
      magel.sensor.add("GPS_Latitude", magel.gps.readLatitude());
      magel.sensor.add("GPS_Longitude", magel.gps.readLongitude());
    } else {
      Serial.println("GPS Not Available");
    } 
    magel.sensor.add("GPS_Available", gpsAvailable);
    magel.sensor.add("BoardTemp", magel.builtInSensor.readTemperature());
    magel.sensor.add("BoardHumid", magel.builtInSensor.readHumidity());
    magel.sensor.report();
  });
}
