#ifndef __GSM_STORAGE_H__
#define __GSM_STORAGE_H__

#include "Arduino.h"

class GSMStorage {
    private:
        bool selectCurrentDirectory(String path) ;
        char _current_drive = 'C';

    public:
        GSMStorage() ;

        bool fileWrite(String path, String content) ;
        String fileRead(String path) ;

        bool mkdir(String path) ;
        bool rmdir(String path) ;
        bool remove(String path) ;

};

extern GSMStorage Storage;

#endif
