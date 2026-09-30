/*
  WebSocket client for the PieSocket WebSocket tester.
  Replace YOUR_API_KEY with a key from your PieSocket account and use the same
  WebSocket URL in the browser tester to view messages from this device.
*/
#include <MAGELLAN_MQTT_4G_BOARD.h>
#include <ArduinoHttpClient.h> // Required ArduinoHttpClient, Install via Library manager
#include <ESP_SSLClient.h> // Required ESP_SSLClient, Install via Library manager

MAGELLAN_MQTT_4G_BOARD Board;
MAGELLAN_MQTT_4G_BOARD::ConnectivityModem &gsmBoard = Board.GSMModem;

const char *serverAddress = "echo.websocket.org";
const uint16_t serverPort = 443;
const char *websocketPath = "/.sse";
const unsigned long reconnectIntervalMs = 5000;
const unsigned long sendIntervalMs = 5000;

ESP_SSLClient secureClient;
WebSocketClient websocketClient(secureClient, serverAddress, serverPort);
unsigned long lastConnectAttemptMs = 0;
unsigned long lastMessageSentMs = 0;
bool hasAttemptedConnection = false;

void setup() {
  Serial.begin(115200);
  gsmBoard.begin();
  secureClient.setClient(&gsmBoard.getClient(), true);
  // TLS is encrypted, but the server certificate is not verified.
  secureClient.setInsecure();
}

void loop() {
  gsmBoard.handle();

  if (!websocketClient.connected()) {
    if (!hasAttemptedConnection || millis() - lastConnectAttemptMs >= reconnectIntervalMs) {
      hasAttemptedConnection = true;
      lastConnectAttemptMs = millis();
      Serial.println("Connecting to PieSocket...");
      int status = websocketClient.begin(websocketPath);
      if (status == 0) {
        Serial.println("WebSocket connected");
        lastMessageSentMs = millis();
      } else {
        Serial.print("WebSocket handshake failed, status: ");
        Serial.println(status);
        websocketClient.stop();
      }
    }
    delay(10);
    return;
  }

  int messageSize = websocketClient.parseMessage();
  if (messageSize > 0 && websocketClient.messageType() == TYPE_TEXT) {
    Serial.print("Received: ");
    Serial.println(websocketClient.readString());
  }

  if (millis() - lastMessageSentMs >= sendIntervalMs) {
    const char *message = "{\"event\":\"text\",\"data\":\"Hello from ESP32\"}";
    websocketClient.beginMessage(TYPE_TEXT);
    websocketClient.print(message);
    int sendStatus = websocketClient.endMessage();
    if (sendStatus == 0) {
      Serial.print("Sent: ");
      Serial.println(message);
    } else {
      Serial.println("WebSocket send failed");
    }
    lastMessageSentMs = millis();
  }

  delay(10);
}
