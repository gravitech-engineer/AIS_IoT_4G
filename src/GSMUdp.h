#ifndef __GSM_UDP_H__
#define __GSM_UDP_H__

#include "Arduino.h"
#include "Udp.h"

#define UDP_WRITE_BUFFER 256
#define UDP_READ_BUFFER 256

class GSMUdp : public UDP {
    private:
        int8_t sock_id = -1;
        
        String write_host;
        uint16_t write_port = 0;
        uint8_t *write_buff = NULL;
        size_t write_len = 0;

        IPAddress remote_ip;
        uint16_t remote_port = 0;

    public:
        GSMUdp() ;
        ~GSMUdp() ; 

        uint8_t begin(uint16_t local_port) ;
        uint8_t beginMulticast(IPAddress ip, uint16_t local_port) ;

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

        void stop() ;

        IPAddress remoteIP() ;
        uint16_t remotePort() ;

};

#endif
