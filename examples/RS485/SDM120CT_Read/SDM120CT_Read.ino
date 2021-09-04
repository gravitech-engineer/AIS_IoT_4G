#include <Arduino.h>
#include <RS485.h>

void setup() {
  Serial.begin(115200);
  Serial.println("Hello !");

  // Setup RS485 with 2400 band and 8 bit data, 1 stop bit, No parity bit
  // More : https://www.arduino.cc/reference/en/language/functions/communication/serial/begin/
  RS485.begin(2400, SERIAL_8N1);
}

void loop() {
  delay(1000);

  // Read Input Register with Deivce ID: 1, Address: 0x0000 and data type is Float
  float voltage = RS485.inputRegisterReadFloat(1, 0x0000);
  delay(50);
  float current = RS485.inputRegisterReadFloat(1, 0x0006);
  delay(50);
  float active_power = RS485.inputRegisterReadFloat(1, 0x000C);
  delay(50);
  float power_factor = RS485.inputRegisterReadFloat(1, 0x001E);
  delay(50);
  float frequency = RS485.inputRegisterReadFloat(1, 0x0046);
  delay(50);
  float total_active_power = RS485.inputRegisterReadFloat(1, 0x0156);

  Serial.printf("Volt: %.02fV\n", voltage);
  Serial.printf("Current: %.02fA\n", current);
  Serial.printf("Active Power: %.02fkWh\n", active_power);
  Serial.printf("Power Factor: %.02f\n", power_factor);
  Serial.printf("Frequency: %.02fHz\n", frequency);
  Serial.printf("Total Active Power: %.02fkWh\n", total_active_power);
  Serial.printf("---------------------------\n");
}
