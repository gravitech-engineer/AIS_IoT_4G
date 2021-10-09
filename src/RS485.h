#ifndef __RS485_H__
#define __RS485_H__

#include "Arduino.h"

#define RS485_RX  16
#define RS485_TX  17
#define RS485_DIR 4

class RS485Class : public Stream {
    private:
        int dir_pin = -1;
        HardwareSerial *uart;

        uint16_t CRC16(uint8_t *buf, int len) ;

    public:
        RS485Class(HardwareSerial *uart) ;

        void begin(unsigned long baudrate, uint32_t config = SERIAL_8N1, int rx_pin = RS485_RX, int tx_pin = RS485_TX, int dir_pin = RS485_DIR) ;
        
        int available() ;
        int read() ;
        int peek() ;
        void flush() ;
        size_t write(uint8_t) ;
        size_t write(const uint8_t *buffer, size_t size) ;
        int availableForWrite() ;

        void beginTransmission() ;
        void endTransmission() ;
        void receive() ;
        void noReceive() ;

        // Modbus RTU
        int coilRead(int address) ;
        int coilRead(int id, int address) ;
        int discreteInputRead(int address) ;
        int discreteInputRead(int id, int address) ;
        long holdingRegisterRead(int address) ;
        long holdingRegisterRead(int id, int address) ;
        long inputRegisterRead(int address) ;
        long inputRegisterRead(int id, int address) ;
        int coilWrite(int address, uint8_t value) ;
        int coilWrite(int id, int address, uint8_t value) ;
        int holdingRegisterWrite(int address, uint16_t value) ;
        int holdingRegisterWrite(int id, int address, uint16_t value) ;

        float inputRegisterReadFloat(int address) ;
        float inputRegisterReadFloat(int id, int address) ;
        int inputRegisterReadU16Array(int id, int address, uint16_t *register_value, uint16_t quantity) ;

        int holdingRegisterWrite(int address, uint16_t *value, size_t len) ;
        int holdingRegisterWrite(int id, int address, uint16_t *value, size_t len) ;

};

extern RS485Class RS485;

#endif
