#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>
#include <WiFi.h>

WiFiClient Wifi_client;
const char ssid[] = "youNetworkName"; //edite here
const char pass[] = "youNetworkPass"; //edite here

const char ThingIden[] = "8966032XXXXXXXXXXXX"; // ICCID edite here
const char ThingSecret[] = "52003XXXXXXXXXX"; // IMSI edite here
const char IMEI[] = "861123XXXXXXXXX"; // IMEI edite here

MAGELLAN_SIM7600E_MQTT magel(Wifi_client);

void initWiFi() 
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.print(F("Connecting to WIFI network"));
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(F("\nWifi Connected!"));
  Serial.print(F("Connected to WiFi network with IP Address: "));
  Serial.println(WiFi.localIP());
}

void setup() 
{
  Serial.begin(115200);
  initWiFi();
  magel.begin(ThingIden, ThingSecret, IMEI); 
}

void loop() 
{
  magel.loop();
  magel.interval(10,[]()
  {
    int temperature = (int)random(25, 35);
    int humidity = (int)random(0, 100);
    magel.sensor.add("temperature", temperature);
    magel.sensor.add("humidity", humidity);
    String payload = magel.sensor.toJSONString();
    magel.report.send(payload);
  });
}