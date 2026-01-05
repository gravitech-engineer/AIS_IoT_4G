#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include <Arduino.h>

#include "FS.h"
#include <vector>

typedef std::vector<String> ListFileString;

#ifdef MG_USE_SPIFFS
    #include "SPIFFS.h"
    #define FS_SYS SPIFFS
#else
    #include "LittleFS.h"
    #define FS_SYS LittleFS

#endif

class FileSystem
{
private:
public:
    void begin(boolean format_if_failed = true);
    ListFileString listDirectory(const char *dir_name, uint8_t level, fs::FS &fs = FS_SYS);
    ListFileString listFile(const char *dir_name, fs::FS &fs = FS_SYS);
    String readFile(const char *path, fs::FS &fs = FS_SYS); // limit filesize can read 60081 can't 63207
    String readBigFile(const char *path, fs::FS &fs = FS_SYS);
    ListFileString readLargeFile(const char *path, fs::FS &fs = FS_SYS);
    boolean writeFile(const char *path, const char *message, fs::FS &fs = FS_SYS);
    boolean appendFile(const char *path, const char *message, fs::FS &fs = FS_SYS);
    boolean renameFile(const char *old_path, const char *new_path, fs::FS &fs = FS_SYS);
    boolean deleteFile(const char *path, fs::FS &fs = FS_SYS);
    boolean isFileExist(const char *path, fs::FS &fs = FS_SYS);
};
extern FileSystem fileSys;
#endif