#include <Arduino.h>
#include <SIM76xx.h>
#include <GSMNetwok.h>

void setup() {
  Serial.begin(115200);
  Serial.println("Hello!");
  
  if (!GSM.begin()) {
    Serial.println("Setup GSM fail");
    while(1) delay(1);
  }

  Serial.print("Operator: ");
  Serial.print(Network.getCurrentCarrier());
  Serial.println();
}

void loop() {
  delay(1000);
}
