#ifndef __GSM_CLIENT_SECURE_H__
#define __GSM_CLIENT_SECURE_H__

#include "Client.h"
#include "GSMClient.h"
#include "gsm_ssl_client.h"

class GSMClientSecure : public Client {   
    private:
        gsm_sslclient_context *sslclient = NULL;

        const char *CA_cert = NULL;
        const char *cert = NULL;
        const char *private_key = NULL;
        const char *pskIdent = NULL;
        const char *psKey = NULL;
        bool insecure = false;
        
    public:
        GSMClientSecure() ;
        ~GSMClientSecure() ;

        void setInsecure() ;
        void setPreSharedKey(const char *pskIdent, const char *psKey) ; // psKey in Hex
        void setCACert(const char *rootCA) ;
        void setCertificate(const char *client_ca) ;
        void setPrivateKey (const char *private_key) ;
        bool verify(const char* fingerprint, const char* domain_name) ;
        void setHandshakeTimeout(unsigned long handshake_timeout);

        int connect(IPAddress ip, uint16_t port, int32_t timeout) ;
        int connect(const char *host, uint16_t port, int32_t timeout) ;

        // From Client class
        inline int connect(IPAddress ip, uint16_t port) {
            return connect(ip, port, 30 * 1000); // 30 sec
        }
        inline int connect(const char *host, uint16_t port) {
            return connect(host, port, 30 * 1000); // 30 sec
        }

        int connect(IPAddress ip, uint16_t port, const char *rootCABuff, const char *cli_cert, const char *cli_key) ;
        int connect(const char *host, uint16_t port, const char *rootCABuff, const char *cli_cert, const char *cli_key) ;
        int connect(IPAddress ip, uint16_t port, const char *pskIdent, const char *psKey) ;
        int connect(const char *host, uint16_t port, const char *pskIdent, const char *psKey) ;

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
