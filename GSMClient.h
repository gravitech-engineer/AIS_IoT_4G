#include "Client.h"
#include "SIMBase.h"

class GSMClient : public Client {   
    private:
        uint8_t sock_id = -1;
        bool _connected = false;

    public:
        GSMClient();
        ~GSMClient();

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
