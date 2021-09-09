#include <Arduino.h>
#include <SIM76xx.h>
#include <AzureIoTHub.h>
#include <Wire.h>
#include <SHT40.h>

AzureIoTHub iot;

void setup() {
  Serial.begin(115200);
  
  // Setup Sensor
  Wire.begin();
  SHT40.begin();

  if (!GSM.begin()) { // Setup GSM (Power on and wait GSM ready)
      Serial.println("GSM setup fail");
      while(1) delay(100);
  }

  iot.configs(
    "<Hub Hostname>.azure-devices.net", // Hostname
    "<Device ID>",                      // Device ID
    "<Primary key | Secondary key>"     // SAS : Primary key or Secondary key
  );
  
  iot.addCommandHandle("light", [](String payload) { // Add handle 'light' command
    int light = payload.toInt(); // Convart payload in String to Number (int)
    
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
