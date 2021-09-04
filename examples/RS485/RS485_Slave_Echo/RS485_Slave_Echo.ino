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
  if (RS485.available()) { // Check new data available
    String dataIn = RS485.readString(); // Read data as String
    Serial.println("Recv: " + dataIn);

    RS485.beginTransmission(); // Start reply (SET DE/^RE => 1)
    RS485.print(dataIn); // Reply data (Send data to RS485 bus)
    RS485.endTransmission(); // END reply (SET DE/^RE => 0)
    RS485.read(); // Remove noise
  }

  delay(10);
}
