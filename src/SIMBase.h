#ifndef __SIM_BASE_H__
#define __SIM_BASE_H__

#include "Arduino.h"
#include <functional>

#define COMMAND_TIMEOUT 500

typedef std::function<void(String data)> URCHandlerFunction;

class SIMBase : public HardwareSerial {
    public:
        SIMBase(int uart_nr) : HardwareSerial(uart_nr) { }

        bool readStringWithTimeout(String *out, uint32_t size, uint32_t timeout = COMMAND_TIMEOUT) ;
        bool readEndsWith(String *out, uint16_t max_size, String endswith, uint32_t timeout = COMMAND_TIMEOUT) ;

        bool wait(String str, uint32_t timeout = COMMAND_TIMEOUT) ;

        bool send(String str) ; // Send to Serial
        bool send(uint8_t* data, uint16_t len) ; // Send to Serial
        bool sendCommand(String cmd, uint32_t timeout = COMMAND_TIMEOUT) ; // Send and wait echo
        bool sendCommandFindOK(String cmd, uint32_t timeout = COMMAND_TIMEOUT) ; // Send , wait echo , find OK
        bool sendCommandGetRespondOneLine(String cmd, String* respond, uint32_t timeout = COMMAND_TIMEOUT) ;
        bool sendCommandCheckRespond(String cmd, uint32_t timeout = COMMAND_TIMEOUT) ;

        // Receive manager
        bool URCServiceStart() ;
        int8_t waitOKorERROR(uint32_t timeout = COMMAND_TIMEOUT) ;
        bool URCRegister(String start, URCHandlerFunction callback) ;
        bool URCDeregister(String start) ;
        void setWaitURC(String start) ;
        bool waitURC(uint32_t timeout = COMMAND_TIMEOUT) ;
        uint16_t getDataAfterIt(uint8_t *buff, uint16_t len, uint32_t timeout = COMMAND_TIMEOUT) ;

};

extern SIMBase _SIM_Base;

#endif
