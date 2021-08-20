#include <SIM76xx.h>

void setup() {
  Serial.begin(115200);
  Serial.println("Start !");

  if (!GSM.begin()) {
    Serial.println("GSM setup fail");
    while(1) delay(100);
  }

  Serial.println("Normal mode");
  for (int i=0;i<20;i++) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("Low Power mode");
  GSM.lowPowerMode(); // Enter to Low Power Mode
  for (int i=0;i<20;i++) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("Back to Normal mode");
  GSM.noLowPowerMode(); // Exit Low Power Mode
  Serial.println("Finish");
}

void loop() {
  delay(1000);
}
