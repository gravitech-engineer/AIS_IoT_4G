#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>
MAGELLAN_SIM7600E_MQTT board;
MAGELLAN_SIM7600E_MQTT::ConnectivityModem &gsmBoard = board.GSMModem;
void setup()
{
  Serial.begin(115200);
  gsmBoard.begin();
  board.gps.begin();
  Serial.println("Setup complete.");
}

void loop()
{
  gsmBoard.handle(); // check modem status and reconnect if needed
  if (board.gps.available())
  {
    Serial.print("===================================\n");
    Serial.print("Latitude: ");
    GPS_Data gpsData = board.gps.getCurrentGPSData();
    Serial.println(gpsData.lat, 6);
    Serial.print("Longitude: ");
    Serial.println(gpsData.lng, 6);
    Serial.print("Altitude: ");
    Serial.println(gpsData.alt, 2);
    Serial.print("Speed: ");
    Serial.println(gpsData.speed, 2);
    Serial.print("Course: ");
    Serial.println(gpsData.course, 2);
    Serial.print("Accuracy: ");
    Serial.println(gpsData.accuracy);
    Serial.print("UTC Time: ");
    Serial.println(gpsData.utc);
    Serial.print("===================================\n");
  }
  else
  {
    Serial.println("GPS data not available.");
    Serial.print("===================================\n");
  }
  delay(5000); // wait for 5 seconds before checking again
}
