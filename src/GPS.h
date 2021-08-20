#ifndef __GPS_H__
#define __GPS_H__

#include "Arduino.h"

class GPSClass {
    private:
        bool isOn() ;

    public:
        GPSClass();

        int begin() ;
        inline int begin(int mode) {
            return begin();
        }
        void end();

        int available();

        float latitude();
        float longitude();
        float speed(); // Speed over the ground in kph
        float course(); // Track angle in degrees
        float variation(); // Magnetic Variation : Not Support
        float altitude();
        int satellites(); // Not support

        unsigned long getTime();

        bool standby() ;
        bool wakeup() ;

};

extern GPSClass GPS;

#endif
