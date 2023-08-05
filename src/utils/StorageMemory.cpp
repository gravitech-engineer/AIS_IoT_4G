#include <Arduino.h>
#include "StorageMemory.h"

StorageMemory::StorageMemory()
{
    Storaged = new GSMStorage;
}

boolean StorageMemory::writeFile(String path, String content)
{
    // GSMStorage *Storaged = new GSMStorage;
    setCurrentPATH(getRootPath(path));
    bool result = Storaged->fileWrite(path, *&content);
    Serial.println("# Write file in \""+path+"\" is: "+((result)?"Success":"Fail"));
    return result;
}

String StorageMemory::readFile(String path)
{
    // GSMStorage *Storaged = new GSMStorage;
    setCurrentPATH(getRootPath(path));
    String res = Storaged->fileRead(path);
    return res;
}

String StorageMemory::readLargeFIle(String path)
{
    // GSMStorage *Storaged = new GSMStorage;
    setCurrentPATH(getRootPath(path));
    return Storaged->readBigFile(path);
}

boolean StorageMemory::deleteFile(String path)
{
    // GSMStorage *Storaged = new GSMStorage;
    setCurrentPATH(getRootPath(path));
    bool result = Storaged->remove(path);
    Serial.println("# Delete file in \""+path+"\" is: "+((result)?"Success":"Fail"));
    return result;
}

boolean StorageMemory::createDirectory(String path)
{
    // GSMStorage *Storaged = new GSMStorage;
    setCurrentPATH(getRootPath(path));
    bool result = Storaged->mkdir(path);
    Serial.println("# Create Directory in \""+path+"\" is: "+((result)?"Success":"Fail"));
    return result;
}

boolean StorageMemory::deleteDirectory(String path)
{
    // GSMStorage *Storaged = new GSMStorage;
    setCurrentPATH(getRootPath(path));
    bool result = Storaged->rmdir(path);
    Serial.println("# Remove Directory in \""+path+"\" is: "+((result)?"Success":"Fail"));
    return result;
}

boolean StorageMemory::isFileExist(String path)
{
    // GSMStorage *Storaged = new GSMStorage;
    setCurrentPATH(getRootPath(path));
    bool result = Storaged->isFileExist(path);
    Serial.println("# file \""+path+"\" is file exist: "+((result)?"Found":"Not Found"));
    return result;
}

ListFileString StorageMemory::getListOfFiles(String path) 
{
    // GSMStorage *Storaged = new GSMStorage;
    setCurrentPATH(path);
    return Storaged->getListOfFiles(path);
}

ListFileString StorageMemory::getListOfDirectories(String path) 
{
    // GSMStorage *Storaged = new GSMStorage;
    setCurrentPATH(path);
    return Storaged->getListOfDirectories(path);
}

size_t StorageMemory::getFileSize(String path)
{
    // GSMStorage *Storaged = new GSMStorage;
    setCurrentPATH(getRootPath(path));
    return Storaged->getFileSize(path);
}

String StorageMemory::getCurrentRootPATH()
{
    String result;
    String valid_path = currentPath.substring(0,1); //valid first charecter
    valid_path.toUpperCase();
    if(valid_path == "D")
    {
        if(currentPath.startsWith("D:/") && currentPath == "D:/")
        {
            result = currentPath;
        }
        else if(currentPath.startsWith("D:/") && currentPath.length() > 3)
        {
            result = currentPath + "/";
        }
    }
    else if(valid_path == "C")
    {
        if(currentPath == "C:/")
        {
            result = currentPath;
        }
        else if(currentPath.startsWith("C:/") && currentPath.length() > 3)
        {
            result = currentPath + "/";
        }
    }
    else{  // if use etc path like E: F:
        result = currentPath;
    }
    return result;
}

boolean StorageMemory::setCurrentPATH(String path){
    if (!_SIM_Base.sendCommandFindOK("AT+FSCD=" + path)) 
    {
        Serial.println("# something wrong");
        Serial.println("# can't not set current path: "+path);
        return false;
    }

    currentPath = path;

    // Serial.println("# move to path on: "+currentPath);
    return true;
}

String StorageMemory::getRootPath(String path)
{
    String basePath = "D:/";
    int pathLength = path.length()+1;
    const int index = path.lastIndexOf("/");
    if(index == 2)
    {
        basePath = path.substring(0, index + 1);
    }
    else if(index > 2)
    {
        basePath = path.substring(0, index);
    }
    // Serial.println(basePath);
    // Serial.println("basePath is: "+basePath);
    // Serial.println("index /: "+String(index));
    return basePath;
}
