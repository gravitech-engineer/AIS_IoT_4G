#include <Arduino.h>
#include "GPS_SIM7600E.h"

GPS_SIM7600E::GPS_SIM7600E()
{
    // _gpsData.lat = 0;
    // _gpsData.lng = 0;
    // _gpsData.alt = 0;
    // _gpsData.speed = 0;
    // _gpsData.course = 0;
    // _gpsData.utc = 0;
    // _gpsData.valid = false;
}

bool GPS_SIM7600E::gpsIsOn(TinyGsm &modem)
{
    modem.sendAT("+CGPS?");
    if (modem.waitResponse("+CGPS:") != 1)
        return false;

    int status = 0;
    // modem.stream.readStringUntil(':');
    status = modem.stream.parseInt();

    modem.waitResponse();
    return status == 1;
}

bool GPS_SIM7600E::available(TinyGsm &modem)
{
    if (!gpsIsOn(modem)) return false;
    GPS_Data temp;
    return gpsRead(modem, temp);
}

bool GPS_SIM7600E::gpsBegin(TinyGsm &modem)
{
    // modem.sendAT("+CGNSSMODE=3");   // GPS + GLONASS
    // modem.waitResponse();

    modem.sendAT("+CGPS=1");
    return modem.waitResponse() == 1;
}

bool GPS_SIM7600E::gpsEnd(TinyGsm &modem)
{
    modem.sendAT("+CGPS=0");
    return modem.waitResponse() == 1;
}

bool GPS_SIM7600E::gpsRead(TinyGsm &modem, GPS_Data &out)
{
    modem.sendAT("+CGPSINFO");
    if (modem.waitResponse("+CGPSINFO:") != 1)
        return false;

    String line = modem.stream.readStringUntil('\n');
    modem.waitResponse();

    // ตัวอย่าง:
    // 1325.1234,N,10030.5678,E,010125,083015.0,12.3,0.5,180.0

    char ns, we;
    float lat, lng;
    int d, m, y, hh, mm, ss;

    if (sscanf(line.c_str(),
               " %f,%c,%f,%c,%2d%2d%2d,%2d%2d%2d.%*c,%f,%f,%f",
               &lat, &ns, &lng, &we,
               &d, &m, &y,
               &hh, &mm, &ss,
               &out.alt, &out.speed, &out.course) < 10)
    {
        return false;
    }

    int lat_deg = lat / 100;
    float lat_min = lat - lat_deg * 100;
    int lng_deg = lng / 100;
    float lng_min = lng - lng_deg * 100;

    out.lat = lat_deg + lat_min / 60.0;
    out.lng = lng_deg + lng_min / 60.0;

    if (ns == 'S')
        out.lat *= -1;
    if (we == 'W')
        out.lng *= -1;

    struct tm t{};
    t.tm_mday = d;
    t.tm_mon = m - 1;
    t.tm_year = y + 100;
    t.tm_hour = hh;
    t.tm_min = mm;
    t.tm_sec = ss;

    out.utc = mktime(&t);
    out.valid = true;

    return true;
}

void GPS_SIM7600E::gpsInit(TinyGsm &modem)
{
    if (!this->gpsIsOn(modem))
    {
        this->gpsBegin(modem);
    }
}