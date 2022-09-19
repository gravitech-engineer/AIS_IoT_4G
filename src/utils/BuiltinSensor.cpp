#include <Arduino.h>
#include "./BuiltinSensor.h"


int cnt_initGPS = 0;
int timeoutInitGPS = 300; // 30 sec 300 * 100 ms = 3000 ms

void BuiltinSensor::gps_begin()
{
    if(attr.clientNetInterface == useExternalClient)
    {
        while (!GSM.begin())
        {
           Serial.println(F("# Try to connect GPS..."));
        }    
    }
    while (!GPS.begin())
    {
        if (cnt_initGPS >= timeoutInitGPS)
        {
            Serial.println(F("GPS Setup timeout exist from initial GPS..."));
            break;
        }
        Serial.println(F("GPS Setup fail"));
        delay(100);
        cnt_initGPS++;
    }
}

void BuiltinSensor::begin()
{
    Wire.begin();
    SHT40.begin();
    gps_begin();
}

info_gps BuiltinSensor::getGPS_info()
{
    info_gps buffer_infoGPS;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("# Please allow \"true\" useBuiltinSensor in begin function"));
        buffer_infoGPS.latitude = 0.00000f;
        buffer_infoGPS.longitude = 0.00000f;
        buffer_infoGPS.altitude = 0.00000f;
        buffer_infoGPS.speed = 0.00000f;
        buffer_infoGPS.course = 0.00000f;
        return buffer_infoGPS;
    }
    if (GPS.available())
    {
        buffer_infoGPS.latitude = GPS.latitude();
        buffer_infoGPS.longitude = GPS.longitude();
        buffer_infoGPS.altitude = GPS.altitude();
        buffer_infoGPS.speed = GPS.speed();
        buffer_infoGPS.course = GPS.course();
    }
    else
    {
        Serial.println("# GPS not ready");
        buffer_infoGPS.latitude = 0.00000f;
        buffer_infoGPS.longitude = 0.00000f;
        buffer_infoGPS.altitude = 0.00000f;
        buffer_infoGPS.speed = 0.00000f;
        buffer_infoGPS.course = 0.00000f;
    }
    return buffer_infoGPS;
}

float BuiltinSensor::readLatitude()
{
    float _lat = 0.000000f;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"readLatitude\" allow \"true\" useBuiltinSensor in begin function"));
        return _lat;
    }
    else
    {
        _lat = getGPS_info().latitude;
    }
    return _lat;
}
float BuiltinSensor::readLongitude()
{
    float _lng = 0.000000f;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"readLongitude\" allow \"true\" useBuiltinSensor in begin function"));
        return _lng;
    }
    else
    {
        _lng = getGPS_info().longitude;
    }
    return _lng;
}

String BuiltinSensor::readLocation()
{
    // String _location = "0.000000,0.000000";
    char _location[] = "0.000000,0.000000";
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"readLocation\" allow \"true\" useBuiltinSensor in begin function"));
        return String(_location);
    }
    else
    {
        sprintf(_location, "%.4f,%.4f", readLatitude(), readLongitude());
    }
    return _location;
}

float BuiltinSensor::readSpeed()
{
    float _speed = 0.000000f;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"readSpeed\" allow \"true\" useBuiltinSensor in begin function"));
        return _speed;
    }
    else
    {
        _speed = getGPS_info().speed;
    }
    return _speed;
}

float BuiltinSensor::readAltitude()
{
    float _alt = 0.000000f;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"readAltitude\" allow \"true\" useBuiltinSensor in begin function"));
        return _alt;
    }
    else
    {
        _alt = getGPS_info().altitude;
    }
    return _alt;
}

float BuiltinSensor::readCourse()
{
    float _course = 0.000000f;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"readCourse\" allow \"true\" useBuiltinSensor in begin function"));
        return _course;
    }
    else
    {
        _course = getGPS_info().course;
    }
    return _course;
}

float BuiltinSensor::readTemperature()
{
    float temp = -1;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"Temperature\" allow \"true\" useBuiltinSensor in begin function"));
        return temp;
    }
    else
    {
        temp = SHT40.readTemperature();
    }
    return temp;
}
float BuiltinSensor::readHumidity()
{
    float humid = -1;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"Humidity\" allow \"true\" useBuiltinSensor in begin function"));
        return humid;
    }
    else
    {
        humid = SHT40.readHumidity();
    }
    return humid;
}

boolean BuiltinSensor::GPSavailable()
{
    bool GPS_state = false;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] GPS not available please allow \"true\" useBuiltinSensor in begin function"));
    }
    else{
        GPS_state = GPS.available();
        // if(!GPS_state)
        // {
            // Serial.println(F("# GPS not available please check signal or antenna GPS connect to board"));
        // }
    }
    return GPS_state;
}

unsigned long BuiltinSensor::getUnixTime()
{
    unsigned long _unix = (long)0000000000;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"getUnixTime\" allow \"true\" useBuiltinSensor in begin function"));
        return _unix;
    }
    else
    {
        if(GPS.available())
        {
        tm t;
        tm buff_tm = utls.convertUnix(GPS.getTime(), 0);
        t.tm_mday = buff_tm.tm_mday;
        t.tm_mon = buff_tm.tm_mon -1;
        t.tm_year = buff_tm.tm_year;
        t.tm_hour = buff_tm.tm_hour;
        t.tm_min = buff_tm.tm_min;
        t.tm_sec = buff_tm.tm_sec;
        t.tm_isdst = -1;
        _unix = mktime(&t);
        }
        else{
            Serial.println(F("# GPS not ready"));     
        }
    }
    return _unix;
}
