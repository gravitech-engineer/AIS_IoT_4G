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
        // tm convertUnix(unsigned long unix, int timeZone = 7);
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

        void setLocalTimeZone(int timeZone);
        int getDay();
        String getDayToString();
        int getMonth();
        String getMonthToString();
        int getYear();
        String getYearToString();
        int getHour();
        String getHourToString();
        int getMinute();
        String getMinuteToString();
        int getSecond();
        String getSecondToString();
        String getDateTimeString();//*
        String getUniversalTime();//*
        unsigned long getUnixTime(); //*
        
};

extern BuiltinSensor mySensor;

#endif