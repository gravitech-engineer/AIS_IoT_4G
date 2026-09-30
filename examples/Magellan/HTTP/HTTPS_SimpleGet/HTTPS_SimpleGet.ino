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
#include <ESP_SSLClient.h> // Required ESP_SSLClient, Install via Library manager

MAGELLAN_MQTT_4G_BOARD Board;
MAGELLAN_MQTT_4G_BOARD::ConnectivityModem &gsmBoard = Board.GSMModem;

const char *serverAddress = "reqres.in";
const int port = 443;
const char *requestPath = "/api/users/2";

ESP_SSLClient secureClient;

void setup() {
  Serial.begin(115200);
  gsmBoard.begin();
  secureClient.setClient(&gsmBoard.getClient(), true);
  // Preserve the original example behavior; TLS is encrypted but the server certificate is not verified.
  secureClient.setInsecure();
}

void loop() {
  gsmBoard.handle();
  Serial.println("making GET request");

  HttpClient client(secureClient, serverAddress, port);
  client.get(requestPath);

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
