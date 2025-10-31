#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>
#include <WiFi.h>

WiFiClient Wifi_client;
const char ssid[] = "youNetworkName"; // edite here
const char pass[] = "youNetworkPass"; // edite here

const char ThingIden[] = "8966032XXXXXXXXXXXX"; // ICCID edite here
const char ThingSecret[] = "52003XXXXXXXXXX";   // IMSI edite here
const char IMEI[] = "861123XXXXXXXXX";          // IMEI edite here

MAGELLAN_SIM7600E_MQTT magel(Wifi_client);

struct WIFI_SETTING
{
  String SSID = "UNKNOWN";
  String PASS = "UNKNOWN";
} WiFiSetting;

unsigned long prv_millis = 0;
unsigned long intervalConnect = 5000;
unsigned long intervalReconnect = 10000;
boolean wifiDisconnect = false;

unsigned long time_previous = 0; // previous time for using timer with millis()

void connectWiFi(WIFI_SETTING &sWiFi_setting);
void reconnectWiFi(MAGELLAN_SIM7600E_MQTT &mqttClient);
void connectWiFi(String SSID, String PASS);
String getSSID();

void setup()
{

  Serial.begin(115200);
  WiFiSetting.SSID = String(ssid);
  WiFiSetting.PASS = String(pass);
  Serial.println("Connecting to WiFi...");
  connectWiFi(WiFiSetting); // connect to WiFi

  // TestDemoTracking
  setting.ThingIdentifier = String(ThingIden); // set your Thing Identifier
  setting.ThingSecret = String(ThingSecret);   // set your Thing Secret
  magel.begin(setting);
}

void loop()
{
  if (!magel.isConnected())
  {
    reconnectWiFi(magel);
  }
  magel.loop();
  magel.subscribes([]()
                   { magel.subscribe.control(PLAINTEXT); });

  magel.interval(15, []()
                 {
        float BoardTemp = magel.builtInSensor.readTemperature();
        float BoardHumid = magel.builtInSensor.readHumidity();
        if (magel.gps.available()) {
            String Location = magel.gps.readLocation();
            magel.sensor.add("GPS", Location);
        }
 
        magel.sensor.add("BoardTemp", BoardTemp);
        magel.sensor.add("BoardHumid", BoardHumid);

 
        magel.sensor.report(); });
}

void connectWiFi(WIFI_SETTING &sWiFi_setting)
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(sWiFi_setting.SSID.c_str(), sWiFi_setting.PASS.c_str());
  Serial.print(F("# Connecting to WIFI network"));
  Serial.print(" SSID: " + WiFiSetting.SSID);
  wifiDisconnect = (WiFi.status() != WL_CONNECTED ? true : false);
  while (wifiDisconnect)
  {
    wifiDisconnect = (WiFi.status() != WL_CONNECTED ? true : false);
    if ((millis() - prv_millis >= intervalConnect))
    {
      Serial.print(F("."));

      // #ifdef ESP32
      WiFi.disconnect();
      WiFi.reconnect();

      prv_millis = millis();
    }
    if (!wifiDisconnect)
    {
      break;
    }
  }

  Serial.println(F("\n# Wifi Connected!"));
  Serial.print(F("# Connected to WiFi network with IP Address: "));
  Serial.println(WiFi.localIP());
}

String getSSID()
{
  return WiFiSetting.SSID;
}

void connectWiFi(String SSID, String PASS)
{
  WiFiSetting.SSID = SSID;
  WiFiSetting.PASS = PASS;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WiFiSetting.SSID.c_str(), WiFiSetting.PASS.c_str());
  Serial.print(F("# Connecting to WIFI network"));
  Serial.print(" SSID: " + WiFiSetting.SSID);
  wifiDisconnect = (WiFi.status() != WL_CONNECTED ? true : false);
  while (wifiDisconnect)
  {
    wifiDisconnect = (WiFi.status() != WL_CONNECTED ? true : false);
    if ((millis() - prv_millis >= intervalConnect))
    {
      Serial.print(F("."));
      WiFi.disconnect();
      WiFi.reconnect();
      prv_millis = millis();
    }
    if (!wifiDisconnect)
    {
      break;
    }
  }
  Serial.println(F("\n# Wifi Connected!"));
  Serial.print(F("# Connected to WiFi network with IP Address: "));
  Serial.println(WiFi.localIP());
}

void reconnectWiFi(MAGELLAN_SIM7600E_MQTT &mqttClient)
{
  wifiDisconnect = (WiFi.status() != WL_CONNECTED ? true : false);
  if (wifiDisconnect)
  {
    Serial.print(F("# Reconnecting to Wifi..."));
  }
  while (wifiDisconnect)
  {
    wifiDisconnect = (WiFi.status() != WL_CONNECTED ? true : false);
    if ((millis() - prv_millis >= intervalReconnect))
    {
      Serial.print(F("."));
      WiFi.disconnect();
      WiFi.reconnect();
      prv_millis = millis();
    }
    if (!wifiDisconnect)
    {
      break;
    }
  }

  if ((!wifiDisconnect) && (!mqttClient.isConnected()))
  {
    magel.begin(setting);
  }
}