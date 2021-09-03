#include <Arduino.h>
#include <Wire.h>
#include <SHT40.h>

void setup() {
  Serial.begin(115200);
  Serial.println("Hello !");

  Wire.begin();
  SHT40.begin();
}

void loop() {
  delay(1000);
  
  Serial.print("Temperature: ");
  Serial.print(SHT40.readTemperature());
  Serial.print(" *C\t");
  Serial.print("Humidity: ");
  Serial.print(SHT40.readHumidity());
  Serial.print(" %RH");
  Serial.println();
}
