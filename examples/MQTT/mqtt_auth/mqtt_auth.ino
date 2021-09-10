/*
 Basic MQTT example with Authentication
  - connects to an MQTT server, providing username
    and password
  - publishes "hello world" to the topic "outTopic"
  - subscribes to the topic "inTopic"

  original from https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_auth/mqtt_auth.ino
*/

#include <Arduino.h>
#include <SIM76xx.h>
#include <GSMClient.h>
#include <PubSubClient.h>

const char *server = "broker.hivemq.com";

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

GSMClient gsm_client;
PubSubClient client(server, 1883, callback, gsm_client);

void setup() {
  Serial.begin(115200);
  Serial.println("Hello!");
  
  while(!GSM.begin()) {
    Serial.println("GSM setup fail");
    delay(2000);
  }

  // Note - the default maximum packet size is 128 bytes. If the
  // combined length of clientId, username and password exceed this use the
  // following to increase the buffer size:
  // client.setBufferSize(255);
  
  if (client.connect("arduinoClient", "testuser", "testpass")) {
    client.publish("outTopic","hello world");
    client.subscribe("inTopic");
  }
}

void loop() {
  client.loop();
}
