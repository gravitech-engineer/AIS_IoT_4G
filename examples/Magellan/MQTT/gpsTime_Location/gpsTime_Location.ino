#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>
MAGELLAN_SIM7600E_MQTT magel;
void setup()
{
    Serial.begin(115200);
    magel.begin();
}


void loop() 
{
  magel.loop();
  magel.interval(5, [](){  //time interval function inside every 5 second
    if(magel.gps.available())
    {
      Serial.println("# location Information ====================");
      Serial.println("Location: "+ magel.gps.readLocation());
      Serial.println("Latitude: "+ String(magel.gps.readLatitude(), 4));
      Serial.println("Longitude: "+ String(magel.gps.readLongitude(), 4));
      Serial.println("Altitude: "+ String(magel.gps.readAltitude()));
      Serial.println("Speed: "+ String(magel.gps.readSpeed()));
      Serial.println("Course: "+ String(magel.gps.readCourse()));
      Serial.println("\n# Time Information ========================");
      Serial.println("getUnixTime: "+ String(magel.gps.getUnixTime()));
    }
    else{
      Serial.println(F("GPS not ready."));
    }
  });
}
