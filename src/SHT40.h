#ifndef __SHT40_H__
#define __SHT40_H__

#include "Arduino.h"
#include "Wire.h"

enum {
    CELSIUS,
    FAHRENHEIT
};

class SHT40Class {
    private:
        TwoWire *wire = NULL;
        uint8_t dev_addr = 0x00;

        float t_degC;
        float rh_pRH;

        bool read() ;

    public:
        SHT40Class(TwoWire *wire = &Wire) ;

        void begin(uint8_t address = 0x44) ;
        float readTemperature(int units = CELSIUS) ;
        float readHumidity() ;
        void end() ;

};

extern SHT40Class SHT40;

#endif
