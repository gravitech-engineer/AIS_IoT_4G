#include <SIM76xx.h>
#include <GPS.h>

void setup() {
  Serial.begin(115200);
  Serial.println("Start !");

  if (!GSM.begin()) {
    Serial.println("GSM setup fail");
    while(1) delay(100);
  }

  if (!GPS.begin()) {
    Serial.println("GPS setup fail");
    while(1) delay(100);
  }
}

void loop() {
  if (GPS.available()) {
    Serial.printf("Unix Timestamp (UTC): %lu\n", GPS.getTime());
  } else {
    Serial.println("GPS not fixed");
  }
  delay(1000);
}
