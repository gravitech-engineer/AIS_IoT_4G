//lasted update  15/11/2022 - check lasted too if state configfile fail, before decide to ota must compare lasted with incoming fw info too 
#include "manageConfigOTAFile.h"

void manageConfigOTAFile::beginFileSystem(boolean formatIfFail)
{
    fileSys.begin(formatIfFail);
}

boolean manageConfigOTAFile::checkFileOTA()
{
    return fileSys.isFileExist(configOTAFilePath);
}

boolean manageConfigOTAFile::checkLastedOTA()
{
    return fileSys.isFileExist(lastedOTAPath);
}


boolean manageConfigOTAFile::createConfigFileOTA()
{
    String init_config = "{\"namefirmware\":\"null\",\"sizefirmware\":\"0\",\"versionfirmware\":\"null\",\"checksumAlgorithm\":\"null\",\"checksum\":\"null\",\"status\":\"null\"}";
    return fileSys.writeFile(configOTAFilePath, init_config.c_str());
}

boolean manageConfigOTAFile::createLastedOTA()
{
    String init_config = "{\"namefirmware\":\"null\",\"sizefirmware\":\"0\",\"versionfirmware\":\"null\",\"checksumAlgorithm\":\"null\",\"checksum\":\"null\"}";
    return fileSys.writeFile(lastedOTAPath, init_config.c_str());
}

String manageConfigOTAFile::readConfigFileOTA()
{
    return fileSys.readFile(configOTAFilePath);
}

String manageConfigOTAFile::readLastedOTA()
{
    return fileSys.readFile(lastedOTAPath);
}

JsonObject manageConfigOTAFile::readObjectConfigFileOTA()
{
  String buffReadConfigOTA = readConfigFileOTA();
  JsonObject buffer;
  OTAdoc.clear();
  if(buffReadConfigOTA != NULL)
  {
    DeserializationError error = deserializeJson(OTAdoc, buffReadConfigOTA);
    buffer = OTAdoc.as<JsonObject>();
    if(error)
      Serial.println("# Error to DeserializeJson readConfigFileOTA");
  }
  return buffer;
}
JsonObject manageConfigOTAFile::readObjectLastedOTA()
{
  String buffReadConfigOTA = readLastedOTA();
  JsonObject buffer;
  OTAdoc.clear();
  if(buffReadConfigOTA != NULL)
  {
    DeserializationError error = deserializeJson(OTAdoc, buffReadConfigOTA);
    buffer = OTAdoc.as<JsonObject>();
    if(error)
      Serial.println("# Error to DeserializeJson readLastedOTA");
  }
  return buffer;
}

String manageConfigOTAFile::readSpacificFromConfFile(String readKey)
{
    String result = readObjectConfigFileOTA()[readKey.c_str()];
    return result;
}

boolean manageConfigOTAFile::saveProfileOTA(JsonObject dataOTA, String stateOTA)
{  
    OTAdoc.clear();
    if(stateOTA != NULL && dataOTA.size() > 0)
    {
        OTAdoc = dataOTA;
        OTAdoc.remove("Code");
        OTAdoc["status"] = stateOTA.c_str();

        String contentToWrite;
        serializeJson(OTAdoc,  contentToWrite);

        bool saveFile = fileSys.writeFile(configOTAFilePath, contentToWrite.c_str());
        if(saveFile)
        {
            // Serial.println("# New Information OTA: "+contentToWrite);
            Serial.println(F("# Save firmware information success!"));      
        }
        return saveFile;
    }
    else
    {
        return false;
    }
}

boolean manageConfigOTAFile::saveLastedOTA(String lastedDataOTA)
{
  bool saveFile = false;
  JsonObject buffer;
  OTAdoc.clear();
  if(lastedDataOTA.c_str() != NULL)
  {
    DeserializationError error = deserializeJson(OTAdoc, lastedDataOTA.c_str());
    buffer = OTAdoc.as<JsonObject>();
    if(error)
    {
      Serial.println("# Error to DeserializeJson saveLastedOTA");
      return false;
    }
    else
    {
        buffer.remove("status");
        String contentLasted;
        serializeJson(buffer, contentLasted);
    
        saveFile = fileSys.writeFile(lastedOTAPath, contentLasted.c_str());
        if(saveFile)
        {
            // Serial.println("# New Information OTA: "+contentToWrite);
            //   Serial.println("# saveLastedOTA: "+contentLasted);
            Serial.println(F("# Save lastedOTA success!"));      
        }
      return saveFile;
    }
  }
  return saveFile;
}

boolean manageConfigOTAFile::saveSuccessOrFail(String stateOTA)
{
    JsonObject bufferProfile = readObjectConfigFileOTA();

    StaticJsonDocument<512> docsBuffer = bufferProfile;
    bufferProfile.remove("Code");
    docsBuffer["status"] = stateOTA.c_str();
    String ProfileUpdate;
    serializeJson(docsBuffer, ProfileUpdate);
    return fileSys.writeFile(configOTAFilePath, ProfileUpdate.c_str());
}

boolean manageConfigOTAFile::compareFirmwareOTA(JsonObject dataOTA)
{
    JsonObject bufferProfile = readObjectConfigFileOTA();
    String status = readSpacificFromConfFile("status");

    String readFW_name = bufferProfile["namefirmware"];
    String incomingFW_name = dataOTA["namefirmware"];

    size_t readFW_size = bufferProfile["sizefirmware"];
    size_t incomingFW_size = dataOTA["sizefirmware"];

    String readFW_checksum = bufferProfile["checksum"];
    String incomingFW_checksum = dataOTA["checksum"];

    String readFW_version = bufferProfile["versionfirmware"];
    String incomingFW_version = dataOTA["versionfirmware"];

    String readFW_csAlg = bufferProfile["checksumAlgorithm"];
    String incomingFW_csAlg = dataOTA["checksumAlgorithm"];

    String readFW_cs = bufferProfile["checksum"];
    String incomingFW_cs = dataOTA["checksum"];

    // Lasted check
    JsonObject buffLasted = readObjectLastedOTA();

    String LastedFW_name = buffLasted["namefirmware"];
    size_t LastedFW_Size = buffLasted["sizefirmware"];
    String LastedFW_checksum = buffLasted["checksum"];
    // Lasted check
    if(readFW_name == "null" || readFW_size <= 0 || readFW_checksum == "null")
    {
        Serial.println(F(""));
        Serial.println(F("# Device is unknow version in file system"));
        Serial.println(F("# Save firmware information"));
        Serial.println("# ======== Information OTA ========");
        // Serial.println("# firmware name: "+ incomingFW_name);
        // Serial.println("# firmware size: "+ String(incomingFW_size));
        Serial.println("# firmware version: "+ incomingFW_version);
        // Serial.println("# firmware checksumAlgorithm: "+ incomingFW_csAlg);
        Serial.println("# firmware checksum: "+ incomingFW_checksum);
        Serial.println(F(""));
        saveProfileOTA(*&dataOTA, "initialize");
        return false; //false mean firmware not match
    }
    if(readFW_name == incomingFW_name && readFW_size == incomingFW_size &&
    readFW_checksum == incomingFW_checksum)
    {
        if(status == "done")
        {
            Serial.println(F(""));
            Serial.println(F("# ======== Firmware is up to date ========"));
            // Serial.println("# firmware name: "+ incomingFW_name);
            // Serial.println("# firmware size: "+ String(incomingFW_size));
            Serial.println("# Firmware device version: "+ incomingFW_version);
            Serial.println(F("# ========================================="));
            Serial.println(F(""));
            return true;
        }
        else if(status == "fail" && !(LastedFW_name == incomingFW_name && LastedFW_Size == incomingFW_size && LastedFW_checksum == incomingFW_checksum))
        {
            Serial.println(F(""));
            Serial.println(F("#[OTA Failed] Try to OTA firmware again"));
            Serial.println(F("# ======== Information OTA ========"));
            // Serial.println("# firmware name: "+ incomingFW_name);
            // Serial.println("# firmware size: "+ String(incomingFW_size));
            Serial.println("# firmware version: "+ incomingFW_version);
            Serial.println(F(""));
            saveProfileOTA(*&dataOTA, "initialize");
            return false; //false mean firmware not match            
        }
        else{
            Serial.println(F(""));
            Serial.println(F("# Keep going to OTA firmware again"));
            Serial.println(F("# ======== Information OTA ========"));
            // Serial.println("# firmware name: "+ incomingFW_name);
            // Serial.println("# firmware size: "+ String(incomingFW_size));
            Serial.println("# firmware version: "+ incomingFW_version);
            Serial.println(F(""));
            return false; //false mean firmware not match
        }
    }
    else{ // fw not match incoming != file => new version
        if(status == "done")
        {
            Serial.println(F(""));
            Serial.println(F("# Device have new version"));
            Serial.println(F("# ======== Information OTA change ========"));
            // Serial.println("# firmware name: "+ readFW_name +" ==> "+incomingFW_name);
            // Serial.println("# firmware size: "+ String(readFW_size) +" ==> "+  String(incomingFW_size));
            Serial.println("# firmware version: "+ readFW_version + " ==> "+ incomingFW_version);
            Serial.println(F(""));
            saveProfileOTA(*&dataOTA, "initialize");
            return false; //false mean firmware not match
        }
        else if(status != "done" && (LastedFW_name == incomingFW_name && LastedFW_Size == incomingFW_size && LastedFW_checksum == incomingFW_checksum))
        {
            Serial.println(F(""));
            Serial.println(F("# ======== Firmware is up to date ========"));
            // Serial.println("# firmware name: "+ incomingFW_name);
            // Serial.println("# firmware size: "+ String(incomingFW_size));
            Serial.println("# Firmware device version: "+ incomingFW_version);
            Serial.println(F("# ========================================="));
            Serial.println(F(""));
            return true;
        }
        else{
            Serial.println(F(""));
            Serial.println("# Previous version OTA does not success status is: \""+ status +"\"# but have new version");
            Serial.println(F("# ======== Information OTA change ========"));
            // Serial.println("# firmware name: "+ readFW_name +" ==> "+incomingFW_name);
            // Serial.println("# firmware size: "+ String(readFW_size) +" ==> "+  String(incomingFW_size));
            Serial.println("# firmware version: "+ readFW_version + " ==> "+ incomingFW_version);
            Serial.println(F(""));
            saveProfileOTA(*&dataOTA, "initialize");
            return false; //false mean firmware not match
        }
    }
}

boolean manageConfigOTAFile::compareFirmwareIsUpToDate(JsonObject dataOTA)
{
    JsonObject buffLastedOTA = readObjectLastedOTA();
    
    String readFW_name = buffLastedOTA["namefirmware"];
    String incomingFW_name = dataOTA["namefirmware"];

    size_t readFW_size = buffLastedOTA["sizefirmware"];
    size_t incomingFW_size = dataOTA["sizefirmware"];

    String readFW_checksum = buffLastedOTA["checksum"];
    String incomingFW_checksum = dataOTA["checksum"];

    String readFW_version = buffLastedOTA["versionfirmware"];
    String incomingFW_version = dataOTA["versionfirmware"];

    String readFW_csAlg = buffLastedOTA["checksumAlgorithm"];
    String incomingFW_csAlg = dataOTA["checksumAlgorithm"];

    String readFW_cs = buffLastedOTA["checksum"];
    String incomingFW_cs = dataOTA["checksum"];

    if(readFW_name == "null" || readFW_size <= 0 || readFW_checksum == "null")
    {
        Serial.println(F(""));
        Serial.println(F("# ======== Firmware is out of date ========"));
        Serial.println(F("# ======== Device unknown version ========"));
        Serial.println("# New version is available: "+ incomingFW_version);
        Serial.println(F("# ========================================"));
        Serial.println(F(""));
        return false;
    }
    if(readFW_name == incomingFW_name && readFW_size == incomingFW_size &&
    readFW_checksum == incomingFW_checksum)
    {
        Serial.println(F(""));
        Serial.println(F("# ======== Firmware is up to date ========"));
        Serial.println("# Firmware device version: "+ readFW_version);
        Serial.println(F("# ========================================="));
        Serial.println(F(""));
        return true;
    }
    else{
        Serial.println(F(""));
        Serial.println(F("# ======== Firmware is out of date ========"));
        Serial.println(F("# ======== NEW firmware available ========"));
        Serial.println("# New version is available: "+ incomingFW_version);
        Serial.println("# Firmware device version: "+ readFW_version);
        Serial.println(F("# ========================================"));
        Serial.println(F(""));
        return false;
    }
}