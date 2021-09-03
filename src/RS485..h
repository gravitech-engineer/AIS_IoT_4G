#ifndef __RS485_H__
#define __RS485_H__

#include "Arduino.h"

#define RS485_RX  17
#define RS485_TX  16
#define RS485_DIR 4

class RS485Class : public HardwareSerial {
    private:
        int dir_pin = -1;

    public:
        RS485Class(int n) ;

        void begin(unsigned long baudrate, uint32_t config = SERIAL_8N1, int rx_pin = RS485_RX, int tx_pin = RS485_TX, int dir_pin = RS485_DIR) ;
        void beginTransmission() ;
        void endTransmission() ;
        void receive() ;
        void noReceive() ;

};

extern RS485Class RS485;

#endif
