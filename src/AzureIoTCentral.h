#ifndef __AZURE_IOT_CENTRAL_H__
#define __AZURE_IOT_CENTRAL_H__

#include "AzureIoTHub.h"

class AzureIoTCentral : public AzureIoTHub {
    protected:
        String idScopt = "";
        String deviceId = "";

        bool dpsGotInfo = false;

    public:
        AzureIoTCentral();
        AzureIoTCentral(Client &c);

        bool configs(String idScopt, String deviceId, String symmetricKey) ;
        bool connect() ;

};


#endif
