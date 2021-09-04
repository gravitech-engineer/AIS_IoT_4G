#include <SIM76xx.h>

int S_PIN = 3; // Sx pin, eg. 3 or 6 or 41 or 43 or 77

void setup() {
  Serial.begin(115200);
  Serial.println("Hello!");
  
  if (!GSM.begin()) {
    Serial.println("Setup GSM fail");
    while(1) delay(1);
  }

  GSM.pinMode(S_PIN, OUTPUT);
}

void loop() {
  GSM.digitalWrite(S_PIN, HIGH);
  delay(500);
  GSM.digitalWrite(S_PIN, LOW);
  delay(500);
}
