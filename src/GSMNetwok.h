#ifndef __GSM_NETWORK_H__
#define __GSM_NETWORK_H__

#include "Arduino.h"
#include "SIMBase.h"

class GSMNetwork {
    public:
        GSMNetwork() ;
        
        // PDP
        bool isNetworkOpen() ;
        bool networkOpen(uint32_t timeout = 30000) ;
        bool networkClose() ;

};

extern GSMNetwork Network;

#endif

