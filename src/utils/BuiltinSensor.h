#ifndef BUILTINSENSOR_H
#define BUILTINSENSOR_H

#include <Arduino.h>
#include <GPS.h>
#include <Wire.h>
#include <SIM76xx.h>
#include <SHT40.h>
#include "./Attribute_MQTT_core.h"
#include <time.h>
#include "./utility.h"
struct info_gps{
    float latitude = 0.0000f;
    float longitude = 0.0000f;
    float altitude = 0.0000f;
    float speed = 0.0000f;
    float course = 0.0000f;
};
class BuiltinSensor: private utility
{
    private:
        const int secPerHour = 3600;
        void gps_begin();
        int local_timeZone = 7;
        info_gps getGPS_info();
    public:
        void begin();
        boolean GPSavailable();
        
        float readLatitude();
        float readLongitude();
        float readAltitude();
        float readSpeed();
        float readCourse();
        float readTemperature();
        float readHumidity();
        String readLocation();
        
        unsigned long getUnixTime(); //*
        
};

extern BuiltinSensor mySensor;

#endif