#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>

MAGELLAN_SIM7600E_MQTT magel;
#define DEFAULT_INTERVAL_TIME 15 // default 15 sec
#define LIMIT_INTERVAL_TIME 300  // limit interval time 60 sec
int IntervalSecond = DEFAULT_INTERVAL_TIME;
const char *KeyIntervalTime = "INTERVAL";

#define NOT_FOUND_SERVERCONFIG "40400"

void setup()
{
  Serial.begin(115200);
  magel.begin();

  magel.getServerConfig([](String key, String value)
                        {
                          Serial.print("# Config incoming\n# [Key]: ");
                          Serial.println(key);
                          Serial.print("# [Value]: ");
                          Serial.println(value);
                          if (key == KeyIntervalTime)
                          {
                            if (value == NOT_FOUND_SERVERCONFIG)
                            {
                              Serial.println("# [ServerConfig]Not found server config key: \""+ key + "\"");
                              Serial.println("# [ServerConfig]Please create server config key \""+ key+ "\" again.");
                              return;
                            }
                            else if (value.toInt() > 0 && value.toInt() <= LIMIT_INTERVAL_TIME)
                            {
                              Serial.print("# [ServerConfig]New interval time update from: ");
                              Serial.print(IntervalSecond); // previous interval time
                              IntervalSecond = value.toInt();
                              Serial.print(" -> ");
                              Serial.println(IntervalSecond); // current interval time
                              magel.clientConfig.add(KeyIntervalTime, IntervalSecond); //add new interval time to client config buffer
                            }
                            else if (value.toInt() < 0 || value.toInt() > LIMIT_INTERVAL_TIME)
                            {
                              Serial.print("# [ServerConfig]: value incoming is: ");
                              Serial.println(value);
                              IntervalSecond = DEFAULT_INTERVAL_TIME;
                              Serial.print("# [ServerConfig]: Invalid value, set to default interval time: ");
                              Serial.println(IntervalSecond); // current interval time
                              magel.clientConfig.add(KeyIntervalTime, IntervalSecond); //add new interval time to client config buffer
                            }
                             magel.clientConfig.save(); //send client config from client config buffer to server
                          } });

  magel.clientConfig.add(KeyIntervalTime, IntervalSecond);
  magel.clientConfig.save();
}

void loop()
{
  magel.loop();
  magel.subscribes([]()
                   {
                     magel.subscribe.serverConfig(PLAINTEXT);     // subscribe server config content type PLAINTEXT
                     magel.serverConfig.request(KeyIntervalTime); // request server config Key INTERVAL <- create server config profiles with key INTERVAL and set value
                   });
  magel.interval(IntervalSecond, []() { // time interval function inside every n sec
    magel.sensor.add("Temperature", magel.builtInSensor.readTemperature());
    magel.sensor.add("Humidity", magel.builtInSensor.readHumidity());
    magel.sensor.report(); // send data sensor with auto json build
  });
}