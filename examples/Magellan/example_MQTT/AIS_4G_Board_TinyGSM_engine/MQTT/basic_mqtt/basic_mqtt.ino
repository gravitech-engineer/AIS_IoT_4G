#include <Arduino.h>
#include <MAGELLAN_MQTT_4G_BOARD.h>

MAGELLAN_MQTT_4G_BOARD Board;
PubSubClient mqttClient;

const char *MQTT_Server = "broker.hivemq.com";
const int MQTT_port = 1883;
const char *MQTT_User = "username";
const char *MQTT_Pass = "password";

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect()
{
  // Loop until we're reconnected
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    String clientID = "ais4gboard-" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientID.c_str(), MQTT_User, MQTT_Pass))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("outTopic", "hello world");
      // ... and resubscribe
      mqttClient.subscribe("inTopic");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Board.InitGSM();
  TinyGsmClient &gsmClient = Board.getGSMClient();
  mqttClient.setClient(gsmClient);
  mqttClient.setServer(MQTT_Server, MQTT_port);
  mqttClient.setBufferSize(512); //up to 8192 bytes หาใช้งานกับ payload ขนาดใหญ่ แนะนำให้ใช้ 4096 ขึ้นไป - 8192
  mqttClient.setCallback(callback);
}

int counter = 0;
void loop()
{
  if (!mqttClient.connected())
  {
    Board.checkModem();
    reconnect();
  }
  mqttClient.loop();
  static unsigned long prvMillis = 0;
  if (millis() - prvMillis > 5000)
  {
    prvMillis = millis();
    Serial.println("Publishing message...");
    String message = "hello world count: " + String(counter);
    mqttClient.publish("outTopic", message.c_str());
    counter++;
    Serial.print("Published count: ");
    Serial.println(counter);
  }
}
