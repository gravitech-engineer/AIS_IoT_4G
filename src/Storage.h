#ifndef __GSM_STORAGE_H__
#define __GSM_STORAGE_H__

#include "Arduino.h"

class GSMStorage {
    private:

    public:
        GSMStorage() ;

        bool fileWrite(String path, String content) ;
        String fileRead(String path) ;

};

extern GSMStorage Storage;

#endif
