#include <Arduino.h>
#include <SIM76xx.h>
#include <GSMClientSecure.h>

GSMClientSecure client;

void setup() {
  Serial.begin(115200);
  Serial.println("Hello!");
  
  GSM.begin();

  client.setInsecure(); // Disable Check CA
  Serial.println(client.connect("fireboard.xyz", 443));

  if (client.connected()) {
    client.print("GET / HTTP/1.1\r\nHost: fireboard.xyz\r\nConnection: close\r\n\r\n");

    delay(100);

    while(client.connected()) {
      // Serial.println("Data in buffer: " + String(client.available()));
      if (client.available()) {
        client.setTimeout(100);
        Serial.print(client.readString());
      }
    }

    client.stop();
  }
}

void loop() {
  Serial.println("Loop");
  delay(1000);
}
