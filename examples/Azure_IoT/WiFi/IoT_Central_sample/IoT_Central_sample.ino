#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <AzureIoTCentral.h>
#include <Azure_CA.h>
#include <Wire.h>
#include <SHT40.h>

#define LED_E15 15

const char* ssid     = "your-ssid";     // your network SSID (name of wifi network)
const char* password = "your-password"; // your network password

WiFiClientSecure client;

time_t getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return 0;
  }
  time(&now);
  return now;
}

AzureIoTCentral iot(client, getTime);

void setup() {
  Serial.begin(115200);

  pinMode(LED_E15, OUTPUT);

  // Setup Sensor
  Wire.begin();
  SHT40.begin();

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);
  }

  Serial.print("Connected to ");
  Serial.println(ssid);

  configTime(0, 0, "pool.ntp.org"); // Config NTP server

  client.setCACert(AZURE_ROOT_CA);

  iot.configs(
    "<ID scope>",                   // ID scope
    "<Device ID>",                  // Device ID
    "<Primary key | Secondary key>" // SAS : Primary key or Secondary key
  );
  
  iot.addCommandHandle("light", [](String payload) { // Add handle 'light' command
    int light = payload.toInt(); // Convart payload in String to Number (int)
    
    digitalWrite(LED_E15, light);
    Serial.print("Light set to ");
    Serial.print(light);
    Serial.println();
  });
}

void loop() {
  if (iot.isConnected()) {
    iot.loop();
    static uint32_t startTime = -5000;
    if ((millis() - startTime) >= 3000) {
      startTime = millis();

      float t = SHT40.readTemperature();
      float h = SHT40.readHumidity();

      iot.setTelemetryValue("temperature", t);
      iot.setTelemetryValue("humidity", h);
      if (iot.sendMessage()) { // Send telemetry to Azure IoT Central
        Serial.println("Send!");
      } else {
        Serial.println("Send error!");
      }
    }
  } else {
    iot.connect();
  }
  delay(1);
}
