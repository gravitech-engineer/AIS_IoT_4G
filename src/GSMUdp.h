#ifndef __GSM_UDP_H__
#define __GSM_UDP_H__

#include "Arduino.h"
#include "Udp.h"

#define UDP_WRITE_BUFFER 256

class GSMUdp : public UDP {
    private:
        char *write_host = NULL;
        uint16_t write_port = 0;
        uint8_t *write_buff = NULL;
        size_t write_len = 0;

    public:
        GSMUdp();

        uint8_t begin(uint16_t) ;
        uint8_t beginMulticast(IPAddress, uint16_t) ;

        int beginPacket(IPAddress ip, uint16_t port) {
            return beginPacket(ip.toString().c_str(), port);
        }
        int beginPacket(const char *host, uint16_t port) ;

        int endPacket() ;
        size_t write(uint8_t) ;
        size_t write(const uint8_t *buffer, size_t size) ;

        int parsePacket() ;
        int available() ;
        int read() ;
        int read(unsigned char* buffer, size_t len) ;
        int read(char* buffer, size_t len) ;
        int peek() ;
        void flush() ;

        IPAddress remoteIP() ;
        uint16_t remotePort() ;

};

#endif
