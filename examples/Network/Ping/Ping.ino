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

  Serial.print("Ping www.ais.th... ");
  if (Network.pingIP("www.ais.th")) {
    Serial.print("OK");
  } else {
    Serial.print("FAIL");
  }
  Serial.println();
}

void loop() {
  delay(1000);
}
