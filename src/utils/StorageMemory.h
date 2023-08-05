#ifndef STORAGEMEMORY_H
#define STORAGEMEMORY_H
#include <Arduino.h>
#include <SIM76xx.h>
#include "SIMBase.h"
#include <Storage.h>
#include <Wire.h>
#include "utility.h"
class StorageMemory
{
private:
    GSMStorage *Storaged;
    String currentPath;
    String getRootPath(String path); //
public:
    StorageMemory();
    
    boolean writeFile(String path, String content);//
    String readFile(String path); //
    String readLargeFIle(String path);
    boolean deleteFile(String path); //
    boolean createDirectory(String path);
    boolean deleteDirectory(String path); //
    boolean isFileExist(String path); //
    ListFileString getListOfFiles(String path) ; //
    ListFileString getListOfDirectories(String path) ; //
    size_t getFileSize(String path); //

    boolean setCurrentPATH(String path); //
    String getCurrentRootPATH(); //
};
extern StorageMemory ext_mem;
#endif