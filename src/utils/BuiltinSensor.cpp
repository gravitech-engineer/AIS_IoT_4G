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
    String _location = "0.000000,0.000000";
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"readLocation\" allow \"true\" useBuiltinSensor in begin function"));
        return String(_location);
    }
    else
    {
        _location = String(readLatitude(), 6) + "," + String(readLongitude(), 6);
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
        if(!GPS_state)
        {
            // Serial.println(F("# GPS not available please check signal or antenna GPS connect to board"));
        }
    }
    return GPS_state;
}

void BuiltinSensor::setLocalTimeZone(int timeZone)
{
    char _lc[] = "00:00";
    char _lc_h[] = "00";
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't set \"setLocalTimeZone\" allow \"true\" useBuiltinSensor in begin function"));
    }
    else{
        this->local_timeZone = timeZone;
        if(local_timeZone >= 0)
        {
            sprintf(_lc_h, "%02i", local_timeZone);
            sprintf(_lc, "%03s:%02s", "+"+String(_lc_h), "00");
        }
        else
        {
            sprintf(_lc_h, "%03i", local_timeZone);
            sprintf(_lc, "%03s:%02s", String(_lc_h), "00");
        }
        Serial.println("# Setting local timezone on GPS GMT: " + String(_lc));
    }
}

int BuiltinSensor::getDay()
{
    int _day = 0;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"getDay\" allow \"true\" useBuiltinSensor in begin function"));
        return _day;
    }
    else
    {
        if(GPS.available())
        {
            _day = utls.convertUnix(GPS.getTime(), local_timeZone).tm_mday;
        }
        else{
            Serial.println(F("# GPS not ready"));
        }
    }

    return _day;
}

String BuiltinSensor::getDayToString()
{
    return String(getDay());
}

int BuiltinSensor::getMonth()
{
    int _month = 0;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"getMonth\" allow \"true\" useBuiltinSensor in begin function"));
        return _month;
    }
    else
    {
        if(GPS.available())
        {
            _month = utls.convertUnix(GPS.getTime(), local_timeZone).tm_mon;
        }
        else{
            Serial.println(F("# GPS not ready"));
        }
    }
    return _month;
}

String BuiltinSensor::getMonthToString()
{
    return String(getMonth());
}

int BuiltinSensor::getYear()
{
    int _year = 0;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"getYear\" allow \"true\" useBuiltinSensor in begin function"));
        return _year;
    }
    else
    {
    if(GPS.available())
        {
            _year = utls.convertUnix(GPS.getTime(), local_timeZone).tm_year + 1900;
        }
        else{
            Serial.println(F("# GPS not ready"));
        }
    }
    return _year;
}

String BuiltinSensor::getYearToString()
{
    return String(getYear());
}

int BuiltinSensor::getHour()
{
    int _hour = 0;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"getHour\" allow \"true\" useBuiltinSensor in begin function"));
        return _hour;
    }
    else
    {
    if(GPS.available())
    {
        
        _hour = utls.convertUnix(GPS.getTime(), local_timeZone).tm_hour;
    }
    else{
            Serial.println(F("# GPS not ready"));
        }
    }
    return _hour;
}

String BuiltinSensor::getHourToString()
{
    return String(getHour());
}

int BuiltinSensor::getMinute()
{
    int _minute = 0;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"getMinute\" allow \"true\" useBuiltinSensor in begin function"));
        return _minute;
    }
    else
    {
    if(GPS.available())
    {
        _minute = utls.convertUnix(GPS.getTime(), local_timeZone).tm_min;
    }
    else{
            Serial.println(F("# GPS not ready"));
        }
    }
    return _minute;
}

String BuiltinSensor::getMinuteToString()
{
    return String(getMinute());
}

int BuiltinSensor::getSecond()
{
    int _sec = 0;
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"getSecond\" allow \"true\" useBuiltinSensor in begin function"));
        return _sec;
    }
    else
    {
    if(GPS.available())
    {
        _sec = utls.convertUnix(GPS.getTime(), local_timeZone).tm_sec;
    }
    else{
        Serial.println(F("# GPS not ready"));
    }
    }
    return _sec;
}

String BuiltinSensor::getSecondToString()
{
    return String(getSecond());
}

String BuiltinSensor::getDateTimeString()
{
    tm buff_tm = utls.convertUnix(GPS.getTime(), local_timeZone);
    char dateSTR[] = "00/00/0000 00:00:00";
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"getDateTimeString\" allow \"true\" useBuiltinSensor in begin function"));
        return String(dateSTR);
    }
    else
    {
        if(GPS.available())
        {
            sprintf(dateSTR, "%02i/%02i/%04i %02i:%02i:%02i", buff_tm.tm_mday, buff_tm.tm_mon, buff_tm.tm_year + 1900, buff_tm.tm_hour, buff_tm.tm_min, buff_tm.tm_sec);
        }
        else{
            Serial.println(F("# GPS not ready"));
        }           
    }
    return String(dateSTR);
}

String BuiltinSensor::getUniversalTime()
{
    tm buff_tm = utls.convertUnix(GPS.getTime(), local_timeZone);
    char _utc[] = "0000-00-00T00:00:00";
    char _lc[] = "00:00";
    char _lc_h[] = "00";
    String result = "0000-00-00T00:00:00+00:00";
    if (!attr.useBuiltInSensor)
    {
        Serial.println(F("#[Warning] Can't read \"getUniversalTime\" allow \"true\" useBuiltinSensor in begin function"));
        return result;
    }
    else
    {
        if(GPS.available())
        {
            sprintf(_utc, "%04i-%02i-%02iT%02i:%02i:%02i", buff_tm.tm_year + 1900, buff_tm.tm_mon, buff_tm.tm_mday, 
            buff_tm.tm_hour,buff_tm.tm_min, buff_tm.tm_sec);

            if(local_timeZone >= 0)
            {
                sprintf(_lc_h, "%02i", local_timeZone);
                sprintf(_lc, "%03s:%02s", "+"+String(_lc_h), "00");
            }
            else{
                sprintf(_lc_h, "%03i", local_timeZone);
                sprintf(_lc, "%03s:%02s", String(_lc_h), "00");
            }
            result = String(_utc) + String(_lc);
        }
        else{
            Serial.println(F("# GPS not ready"));
        }      
    }
    return result;
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
