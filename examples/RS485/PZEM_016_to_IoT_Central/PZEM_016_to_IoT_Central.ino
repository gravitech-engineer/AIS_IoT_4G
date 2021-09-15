#include <Arduino.h>
#include <SIM76xx.h>
#include <AzureIoTCentral.h>
#include <Wire.h>
#include <RS485.h>

#define LED_E15 15

AzureIoTCentral iot;

void setup() {
  Serial.begin(115200);

  // Setup RS485 with 9600 band and 8 bit data, 1 stop bit, No parity bit
  // More : https://www.arduino.cc/reference/en/language/functions/communication/serial/begin/
  RS485.begin(9600, SERIAL_8N1);
  
  Serial.println("Setup GSM...");
  while (!GSM.begin()) { // Setup GSM (Power on and wait GSM ready)
      Serial.println("GSM setup fail");
      delay(2000);
  }
  Serial.println("GSM setup OK!");

  iot.configs(
    "<ID scope>",                   // ID scope
    "<Device ID>",                  // Device ID
    "<Primary key | Secondary key>" // SAS : Primary key or Secondary key
  );
}

void loop() {
  if (iot.isConnected()) {
    iot.loop();
    static uint32_t startTime = -5000;
    if ((millis() - startTime) >= 3000) {
      startTime = millis();

      Serial.print("Read Power Meter... ");
      uint16_t input_register_buffer[10];
      // Read Input Register Device ID: 1, Address: 0x0000 - 0x0009 (10 register)
      RS485.inputRegisterReadU16Array(1, 0, input_register_buffer, 10);

      // Convert data
      float voltage = input_register_buffer[0] * 0.1f;
      float current = (((uint32_t)input_register_buffer[2] << 16) | input_register_buffer[1]) * 0.001f;
      float power = (((uint32_t)input_register_buffer[4] << 16) | input_register_buffer[3]) * 0.1f;
      float energy = (((uint32_t)input_register_buffer[6] << 16) | input_register_buffer[5]) * 1.0f;
      float frequency = input_register_buffer[7] * 0.1f;
      float pf = ((float)input_register_buffer[8]) * 0.01f;
      Serial.println("OK!");
      
      Serial.print("Send Message... ");
      iot.setTelemetryValue("voltage", voltage);
      iot.setTelemetryValue("current", current);
      iot.setTelemetryValue("power", power);
      iot.setTelemetryValue("energy", energy);
      iot.setTelemetryValue("frequency", frequency);
      iot.setTelemetryValue("pf", pf);
      if (iot.sendMessage()) { // Send telemetry to Azure IoT Central
        Serial.println("OK!");
      } else {
        Serial.println("FAIL");
      }
    }
  } else {
    Serial.print("Reconnect to Azure... ");
    if (iot.connect()) {
      Serial.println("connected");
    } else {
      Serial.println("connect fail");
      delay(2000);
    }
  }
  delay(1);
}
