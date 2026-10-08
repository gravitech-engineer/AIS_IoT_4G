#include <Arduino.h>
#include "Wire.h"
#include <MAGELLAN_SIM7600E_MQTT.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

MAGELLAN_SIM7600E_MQTT magel;

#define defaultInterval 15
unsigned int valueInterval = defaultInterval;
const String keyConf_interval = "INTERVAL";

#define GPIO_E18 18
#define GPIO_E19 19
#define GPIO_E23 23
#define GPIO_E25 25
#define GPIO_E26 26
#define GPIO_E27 27
#define GPIO_E32 32
#define GPIO_E33 33
#define GPIO_E33 33
#define GPIO_E05 5 // Strapping PIN
#define GPIO_E15 15 //Strapping PIN (LED_E15)

enum GPIO_TYPE
{
    DIGI_OUT = -1,
    PWM = -2
};

struct MAPPING_GPIO
{
    String StringKey;
    int GPIO;
    GPIO_TYPE Type;
};

MAPPING_GPIO controlGPIOs[] = {
    {"Relay1", GPIO_E18, DIGI_OUT},
    {"Relay2", GPIO_E19, DIGI_OUT},
    {"Relay3", GPIO_E23, DIGI_OUT},
    {"Relay4", GPIO_E25, DIGI_OUT},
    {"Relay5", GPIO_E26, DIGI_OUT},
    {"Relay6", GPIO_E27, DIGI_OUT},
    {"Relay7", GPIO_E32, DIGI_OUT},
    {"Relay8", GPIO_E33, DIGI_OUT},
    {"Lamp1", GPIO_E15, DIGI_OUT},
};

void initializeControlGPIOs()
{
    for (int i = 0; i < sizeof(controlGPIOs) / sizeof(controlGPIOs[0]); i++)
    {
        if (controlGPIOs[i].GPIO >= 0)
        {
            Serial.println("Initializing GPIO: " + String(controlGPIOs[i].StringKey) + " on pin [" + String(controlGPIOs[i].GPIO) + "]");
            pinMode(controlGPIOs[i].GPIO, OUTPUT);
            digitalWrite(controlGPIOs[i].GPIO, LOW);
        }
    }
}

void controlGPIO(String key, String value)
{
    bool foundSensorKey = false;
    for (int i = 0; i < sizeof(controlGPIOs) / sizeof(controlGPIOs[0]); i++)
    {
        if (key == controlGPIOs[i].StringKey)
        {
            foundSensorKey = true;
            if (controlGPIOs[i].GPIO <= 0)
            {
                Serial.println("Invalid GPIO number for control: " + controlGPIOs[i].StringKey + ".  GPIO value: " + String(controlGPIOs[i].GPIO));
                return;
            }
            int val = value.toInt();
            if (controlGPIOs[i].Type == PWM)
            {
                Serial.println("Control GPIO [" + controlGPIOs[i].StringKey + "] is a DAC type.");
                if (val >= 0 && val <= 255)
                {
                    // dacWrite(controlGPIOs[i].GPIO, val);
                    analogWrite(controlGPIOs[i].GPIO, val);
                }
                else
                {
                    Serial.println("Invalid DAC value for GPIO control: " + value);
                }
                magel.control.ACK(key, value);
            }
            else if (controlGPIOs[i].Type == DIGI_OUT)
            {
                if (val == 1)
                {
                    digitalWrite(controlGPIOs[i].GPIO, HIGH);
                }
                else if (val == 0)
                {
                    digitalWrite(controlGPIOs[i].GPIO, LOW);
                }
                if (val == 1 || val == 0)
                    Serial.println("Control GPIO [" + String(controlGPIOs[i].GPIO) + "] [" + controlGPIOs[i].StringKey + "] set to " + value);
                magel.control.ACK(key, String(digitalRead(controlGPIOs[i].GPIO)));
            }
            else
            {
                Serial.println("Invalid value for GPIO control: " + value);
            }
            return;
        }
    }
    if (!foundSensorKey)
    {
        Serial.println("Control key in condition not found: " + key + "Acknowledging with value: " + value);
        magel.control.ACK(key, value);
    }
}


bool flagClientConfig = false;
void reportControlGPIOsStatus()
{
    for (int i = 0; i < sizeof(controlGPIOs) / sizeof(controlGPIOs[0]); i++)
    {
        if (controlGPIOs[i].GPIO >= 0)
        {
            magel.sensor.add(controlGPIOs[i].StringKey, digitalRead(controlGPIOs[i].GPIO));
            if (!flagClientConfig){
                magel.clientConfig.add(controlGPIOs[i].StringKey.c_str(), (int)controlGPIOs[i].GPIO);
            }
        }
    }
    if (!flagClientConfig)
        magel.clientConfig.save();
    flagClientConfig = true;
    magel.sensor.report();
}

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    Serial.begin(115200);
    Serial.println(F("Firmware 4G Board GPIO Mapper."));
    initializeControlGPIOs();

    magel.begin(setting);
    magel.getControl([](String key, String value) { // focus only value form key "Lamp1"
        Serial.print((F("# Control incoming on [Key]: ")));
        Serial.print(key);
        Serial.print((F(" [Value]: ")));
        Serial.println(value);
        controlGPIO(key, value);
    });

    magel.getServerConfig([](String keyConf, String valueConf)
                          {
        Serial.println("# Raw data serverConfig[key]: "+keyConf+" - [value]: "+ valueConf);
        if(keyConf == keyConf_interval)
        {
            int valueConf_int = valueConf.toInt();
            valueConf.trim();
            if((valueConf.indexOf("40400") == -1) && valueConf_int > 0)
            {     
                Serial.println("# Found Server config");
                int prv_valueInterval = valueInterval;
                valueInterval = valueConf_int;         
                Serial.println("# Set value Interval to new value from :"+String(prv_valueInterval)+ " -> "+String(valueInterval));
            }
            else if (valueConf.indexOf("40300") != -1 || valueConf.indexOf("40301") != -1 || valueConf.indexOf("40401") != -1)
            {
                Serial.println("# ["+keyConf+"] Error code Server config requested already exists.");
                Serial.println("# Not found Config value Interval set to default: "+ String(defaultInterval));
                valueInterval = defaultInterval;
            }
            magel.clientConfig.add(keyConf_interval.c_str(), (int)valueInterval);
        }
        magel.clientConfig.save(); });
    magel.gps.begin();

    reportControlGPIOsStatus();

    magel.onConnect([]()
    { Serial.println("[onConnect]Connected to Magellan server."); 
        magel.serverConfig.request(keyConf_interval); 
    });
}

void loop()
{
    static unsigned long time_previous = 0;
    magel.loop();
    magel.subscribesHandler();
    magel.interval(valueInterval, []
                   {
        magel.sensor.add("Board_Temp", magel.builtInSensor.readTemperature());
        magel.sensor.add("Board_Humid", magel.builtInSensor.readHumidity());
        bool isGPSAvailable = magel.gps.available();
        magel.sensor.add("GPS_Available", isGPSAvailable);
        if (isGPSAvailable) {
          String location = magel.gps.readLocation();
          magel.sensor.add("GPS_Location", location);
        } else {
          Serial.println(F("[Warn] No GPS signal. Check antenna connection and outdoor sky view."));
        }
        magel.sensor.report(); });

    if (millis() - time_previous > 300000) // update Control every 5 minutes
    {
        time_previous = millis(); // update time
        reportControlGPIOsStatus();
    }
}
