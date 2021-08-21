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
  
  /*
  Network.getSignalStrength():
    0       – -113 dBm or less
    1       – -111 dBm
    2...30  – -109 ... -53 dBm
    31      – -51 dBm or greater
    99      – not known or not detectable
    100     – -116 dBm or less
    101     – -115 dBm
    102…191 – -114 ... -26dBm
    191     – -25 dBm or greater
    199     – not known or not detectable
    100…199 – expand to TDSCDMA, indicate RSCP rece
    -1      – error
  */

  Serial.print("Signal Strength: ");
  Serial.print(Network.getSignalStrength());
  Serial.println();
}

void loop() {
  delay(1000);
}
