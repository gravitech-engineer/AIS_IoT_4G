#include <Arduino.h>
#include <RS485.h>

void setup() {
  Serial.begin(115200);
  Serial.println("Hello !");

  // Setup RS485 with 9600 band and 8 bit data, 1 stop bit, No parity bit
  // More : https://www.arduino.cc/reference/en/language/functions/communication/serial/begin/
  RS485.begin(9600, SERIAL_8N1);
}

void loop() {
  // Read Input Register Device ID: 1, Address: 1
  long temp_int16 = RS485.inputRegisterRead(1, 1);
  delay(50);
  // Read Input Register Device ID: 1, Address: 2
  long humi_int16 = RS485.inputRegisterRead(1, 2);

  // Convert data, 325 => 32.5 , 643 => 64.3
  float temp_float = temp_int16 / 10.0;
  float humi_float = humi_int16 / 10.0;

  Serial.print("Temperature: ");
  Serial.print(temp_float);
  Serial.print(" *C\t");
  Serial.print("Humidity: ");
  Serial.print(humi_float);
  Serial.println(" %RH");
  delay(1000);
}
