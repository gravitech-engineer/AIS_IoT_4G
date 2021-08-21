#ifndef __GSM_SOCKET_H__
#define __GSM_SOCKET_H__

#include "Arduino.h"

struct Socket_TCPUDP {
    bool itUsing = false;
    bool connected = false;
    QueueHandle_t rxQueue = NULL;
    bool read_request_flag = false;
};

extern struct Socket_TCPUDP ClientSocketInfo[];

void setup_Socket() ;

#endif