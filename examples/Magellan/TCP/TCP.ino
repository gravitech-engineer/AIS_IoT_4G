/*
  Raw TCP echo example using the TinyGSM client owned by
  MAGELLAN_MQTT_4G_BOARD. Connect to tcpbin.com:4242 and exchange text.
*/
#include <MAGELLAN_MQTT_4G_BOARD.h>

MAGELLAN_MQTT_4G_BOARD Board;
MAGELLAN_MQTT_4G_BOARD::ConnectivityModem &gsmBoard = Board.GSMModem;
TinyGsmClient &tcpClient = gsmBoard.getClient();

const char *serverAddress = "tcpbin.com";
const uint16_t serverPort = 4242;
const unsigned long reconnectIntervalMs = 5000;
const unsigned long sendIntervalMs = 5000;
const char *message = "Hello from AIS 4G board\r\n";

unsigned long lastConnectAttemptMs = 0;
unsigned long lastMessageSentMs = 0;
bool hasAttemptedConnection = false;

void setup() {
  Serial.begin(115200);
  gsmBoard.begin();
}

void loop() {
  gsmBoard.handle();

  if (!tcpClient.connected()) {
    if (!hasAttemptedConnection || millis() - lastConnectAttemptMs >= reconnectIntervalMs) {
      hasAttemptedConnection = true;
      lastConnectAttemptMs = millis();
      Serial.print("Connecting to ");
      Serial.print(serverAddress);
      Serial.print(':');
      Serial.println(serverPort);

      if (tcpClient.connect(serverAddress, serverPort)) {
        Serial.println("TCP connected");
        lastMessageSentMs = millis() - sendIntervalMs;
      } else {
        Serial.println("TCP connection failed");
        tcpClient.stop();
      }
    }
    delay(10);
    return;
  }

  while (tcpClient.available() > 0) {
    Serial.write(tcpClient.read());
  }

  if (millis() - lastMessageSentMs >= sendIntervalMs) {
    size_t bytesSent = tcpClient.print(message);
    if (bytesSent == strlen(message)) {
      Serial.print("Sent: ");
      Serial.println(message);
    } else {
      Serial.println("TCP send failed");
    }
    lastMessageSentMs = millis();
  }

  delay(10);
}
