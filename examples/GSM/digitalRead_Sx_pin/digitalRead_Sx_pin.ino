#include <SIM76xx.h>

int S_PIN = 77; // Sx pin, eg. 3 or 6 or 41 or 43 or 77

void setup() {
  Serial.begin(115200);
  Serial.println("Hello!");
  
  if (!GSM.begin()) {
    Serial.println("Setup GSM fail");
    while(1) delay(1);
  }

  GSM.pinMode(S_PIN, INPUT);
}

void loop() {
  Serial.println(GSM.digitalRead(S_PIN));
  delay(100);
}
