#include <Arduino.h>
#include "./FileSystem.h"
#define DEBUG false  //set to true for debug output, false for no debug output
#define DEBUG_SERIAL if(DEBUG)Serial

void FileSystem::begin(boolean format_if_failed)
{
   if(!SPIFFS.begin(format_if_failed))
   {
     DEBUG_SERIAL.println("- FileSystem Mount Failed");
     return;
   }

}
ListFileString FileSystem::listDirectory(const char* dir_name, uint8_t level, fs::FS &fs)
{
    ListFileString list;
    DEBUG_SERIAL.printf("Listing directory: %s\r\n", dir_name);

   File root = fs.open(dir_name);
   if(!root){
      DEBUG_SERIAL.println("− failed to open directory");
      return list;
   }
   if(!root.isDirectory()){
      DEBUG_SERIAL.println(" − not a directory");
      return list;
   }

   File file = root.openNextFile();
   if(!file)
   {
    DEBUG_SERIAL.printf("- not found directory in directory: %s\r\n", dir_name);
   }
   while(file){
      if(file.isDirectory()){
         DEBUG_SERIAL.print("  DIR : ");
         DEBUG_SERIAL.println(file.name());
         list.push_back(file.name());
         if(level){
          listDirectory(file.name(), level -1);
         }
      }
      file = root.openNextFile();
   }
   return list;
}

ListFileString FileSystem::listFile(const char* dir_name, fs::FS &fs)
{
    ListFileString list;
    DEBUG_SERIAL.printf("Listing File: %s\r\n", dir_name);

   File root = fs.open(dir_name);
   if(!root){
      DEBUG_SERIAL.println("− failed to open directory");
      return list;
   }

   File file = root.openNextFile();
   if(!file)
   {
    DEBUG_SERIAL.printf("- not found file in directory: %s\r\n", dir_name);
   }
   while(file){
      if(!file.isDirectory()){
         DEBUG_SERIAL.print("  FILE: ");
         DEBUG_SERIAL.print(file.name());
         list.push_back(file.name());
         DEBUG_SERIAL.print("\tSIZE: ");
         DEBUG_SERIAL.println(file.size());
      }
      file = root.openNextFile();
   }
   return list;
}

String FileSystem::readFile(const char* path, fs::FS &fs)
{
    DEBUG_SERIAL.printf("Reading file: %s\r\n", path);
    char *buffer;
    File file = fs.open(path);
    if(!file || file.isDirectory())
    {
      DEBUG_SERIAL.println(F("- faild to open file for reading"));
      return "null";
    }
    DEBUG_SERIAL.println(F("- read from file:"));
    while(file.available())
    {
      buffer = new char[file.size() +2];
      strcpy(buffer, file.readString().c_str());
    }
    file.close();
    return String(buffer);
}

boolean FileSystem::writeFile(const char* path, const char * message, fs::FS &fs)
{
    DEBUG_SERIAL.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
      DEBUG_SERIAL.println("− failed to open file for writing");
      return false;
    }
    if(file.print(message)){
      DEBUG_SERIAL.println("− file written");
      file.close();
      return true;
    }else {
      DEBUG_SERIAL.println("− frite failed");
      return false;
    }
}

boolean FileSystem::appendFile(const char* path, const char * message, fs::FS &fs)
{
    DEBUG_SERIAL.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
      DEBUG_SERIAL.println("− failed to open file for appending");
      return false;
    }
     if(file.print(message)){
        DEBUG_SERIAL.println("− message appended");
        file.close();
        return true;
     } else {
        DEBUG_SERIAL.println("− append failed");
        return false;
     }
}

boolean FileSystem::renameFile(const char* old_path, const char * new_path, fs::FS &fs)
{
   DEBUG_SERIAL.printf("Renaming file %s to %s\r\n", old_path, new_path);
   if (fs.rename(old_path, new_path)) {
      DEBUG_SERIAL.println("− file renamed");
      return true;
   } else {
      DEBUG_SERIAL.println("− rename failed");
      return false;
   }
}

boolean FileSystem::deleteFile(const char* path, fs::FS &fs)
{
   DEBUG_SERIAL.printf("Deleting file: %s\r\n", path);
   if(fs.remove(path)){
      DEBUG_SERIAL.println("− file deleted");
      return true;
   } else {
      DEBUG_SERIAL.println("− delete failed");
      return false;
   }
}

boolean FileSystem::isFileExist(const char* path, fs::FS &fs)
{
  return fs.exists(path);
}

