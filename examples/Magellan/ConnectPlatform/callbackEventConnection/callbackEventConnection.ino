#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>

MAGELLAN_SIM7600E_MQTT magel;

void setup()
{
  Serial.begin(115200);

  magel.onConnect([]()
                  {
                    Serial.println("[onConnect]Connected to Magellan IoT Platform!!");
                    // do something when connected
                  });
  magel.onDisconnect([]()
                     {
                       Serial.println("[onDisconnect]Disconnected from Magellan IoT Platform");
                       // do something when disconnected
                     });

  magel.onReconnect([]()
                    {
                      static int reconnectCount = 0;
                      reconnectCount++;
                      Serial.println(F("[onReconnect]Reconnected to Magellan IoT Platform"));
                      Serial.print(F("Reconnection attempt: "));
                      Serial.println(reconnectCount);
                      // do something when reconnected (attempts count max 10 times trigger every 3 seconds before Restart)
                    });
                    
  magel.begin(setting);
}

void loop()
{
  magel.loop();
  magel.subscribesHandler();
  magel.interval(10, []
                 {
                   magel.sensor.add("Board_Temp", magel.builtInSensor.readTemperature());
                   magel.sensor.add("Board_Humid", magel.builtInSensor.readHumidity());
                 
                   magel.sensor.report(); });
}
