#ifndef GPS_SIM7600E_H
#define GPS_SIM7600E_H
#include <Arduino.h>
#include "./TinyGSM/src/TinyGSM.h"
#include <time.h>

struct GPS_Data
{
    float lat = 0;
    float lng = 0;
    float alt = 0;
    float speed = 0;
    float course = 0;
    int accuracy = 0;
    time_t utc = 0;
    bool valid = false;
};

class GPS_SIM7600E
{
public:
    // GPS_Data _gpsData;
    GPS_SIM7600E();
    // TinyGsm *internalModem = nullptr;
    bool gpsIsOn(TinyGsm &modem);
    bool available(TinyGsm &modem); // Check if GPS has valid data
    bool gpsBegin(TinyGsm &modem);
    bool gpsEnd(TinyGsm &modem);
    bool gpsRead(TinyGsm &modem, GPS_Data &out);
    void gpsInit(TinyGsm &modem);
};
// extern GPS_Data _gpsData;
#endif