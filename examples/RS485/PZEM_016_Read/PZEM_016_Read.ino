#include <Arduino.h>
#include <RS485.h>

#include "Wire.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Hello !");

  // Setup RS485 with 9600 band and 8 bit data, 1 stop bit, No parity bit
  // More : https://www.arduino.cc/reference/en/language/functions/communication/serial/begin/
  RS485.begin(9600, SERIAL_8N1);
}

void loop() {
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
  
  Serial.println("--------------");
  Serial.printf("Voltage: %.02fV\n", voltage);
  Serial.printf("Current: %.02fA\n", current);
  Serial.printf("Power: %.02f W\n", power);
  Serial.printf("Energy: %.02f Wh\n", energy);
  Serial.printf("Frequency: %.01f Hz\n", frequency);
  Serial.printf("Power Factor: %.02f\n", pf);

  delay(2000);
}
