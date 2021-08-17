#ifndef __GSM_CLIENT_SECURE_H__
#define __GSM_CLIENT_SECURE_H__

#include "Client.h"
#include "GSMClient.h"
#include "gsm_ssl_client.h"

class GSMClientSecure : public Client {   
    private:
        gsm_sslclient_context *sslclient = NULL;
        
    public:
        GSMClientSecure();
        ~GSMClientSecure();

        int connect(IPAddress ip, uint16_t port, int32_t timeout) ;
        int connect(const char *host, uint16_t port, int32_t timeout) ;

        // From Client class
        inline int connect(IPAddress ip, uint16_t port) {
            return connect(ip, port, 30 * 1000); // 30 sec
        }
        inline int connect(const char *host, uint16_t port) {
            return connect(host, port, 30 * 1000); // 30 sec
        }

        size_t write(uint8_t) ;
        size_t write(const uint8_t *buf, size_t size) ;
        int available() ;
        int read() ;
        int read(uint8_t *buf, size_t size) ;
        int peek() ;
        void flush() ;
        void stop() ;
        uint8_t connected() ;
        operator bool() ;

};

#endif
