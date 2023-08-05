
#ifndef UTILITY_H
#define UTILITY_H
#include <Arduino.h>
    class utility
    {
    private:

    public:
        String toDateTimeString(unsigned long unixtTime, int timeZone);
        String toUniversalTime(unsigned long unixtTime, int timeZone);
        unsigned long toUnix(tm time_);
        tm convertUnix(unsigned long unix, int timeZone);
        boolean StringIsDigit(String validate_str);
    };
    extern utility utls;    
#endif