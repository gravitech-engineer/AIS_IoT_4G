/*
 Publishing in the callback
  - connects to an MQTT server
  - subscribes to the topic "inTopic"
  - when a message is received, republishes it to "outTopic"
  This example shows how to publish messages within the
  callback function. The callback function header needs to
  be declared before the PubSubClient constructor and the
  actual callback defined afterwards.
  This ensures the client reference in the callback function
  is valid.

  original from https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_publish_in_callback/mqtt_publish_in_callback.ino
*/

#include <Arduino.h>
#include <SIM76xx.h>
#include <GSMClient.h>
#include <PubSubClient.h>

const char *server = "broker.hivemq.com";

// Callback function header
void callback(char* topic, byte* payload, unsigned int length);

GSMClient gsm_client;
PubSubClient client(server, 1883, callback, gsm_client);

// Callback function
void callback(char* topic, byte* payload, unsigned int length) {
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.

  // Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length);
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
  client.publish("outTopic", p, length);
  // Free the memory
  free(p);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Hello!");
  
  while(!GSM.begin()) {
    Serial.println("GSM setup fail");
    delay(2000);
  }

  if (client.connect("arduinoClient")) {
    client.publish("outTopic","hello world");
    client.subscribe("inTopic");
  }
}

void loop() {
  client.loop();
}
