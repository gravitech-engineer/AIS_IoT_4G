#include <Arduino.h>
#include <SIM76xx.h>
#include <GSMClient.h>

GSMClient client;

void setup() {
  Serial.begin(115200);
  Serial.println("Hello!");
  
  GSM.begin();

  Serial.println(client.connect("185.78.164.23", 80));

  if (client.connected()) {
    client.print("GET / HTTP/1.1\r\nHost: 185.78.164.23\r\nConnection: close\r\n\r\n");

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
