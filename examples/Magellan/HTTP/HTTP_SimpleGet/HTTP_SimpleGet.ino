/*
  Simple GET client for ArduinoHttpClient library
  Connects to server once every five seconds, sends a GET request

  created 14 Feb 2016
  modified 22 Jan 2019
  by Tom Igoe
  modified 10 Sep 2021
  by Advanced Info Service Public Company Limited
  
  this example is in the public domain
 */
#include <MAGELLAN_MQTT_4G_BOARD.h>
#include <ArduinoHttpClient.h> // Required ArduinoHttpClient, Install via Library manager

MAGELLAN_MQTT_4G_BOARD Board;
MAGELLAN_MQTT_4G_BOARD::ConnectivityModem &gsmBoard = Board.GSMModem;

const char *serverAddress = "185.78.164.23";
const int port = 80;

void setup() {
  Serial.begin(115200);
  gsmBoard.begin();
}

void loop() {
  gsmBoard.handle();
  Serial.println("making GET request");

  TinyGsmClient &gsmClient = gsmBoard.getClient();
  HttpClient client(gsmClient, serverAddress, port);
  client.get("/");

  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  client.stop();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
  Serial.println("Wait five seconds");
  delay(5000);
}
