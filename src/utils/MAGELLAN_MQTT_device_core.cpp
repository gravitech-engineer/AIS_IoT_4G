/*
Copyright (c) 2020, Advanced Wireless Network
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
support SIMCOM SIM7600E(AIS 4G Board)

Author:(POC Device Magellan team)
Create Date: 25 April 2022.
Modified: 22 may 2023.
Released for private usage.
*/

#include "MAGELLAN_MQTT_device_core.h"
#if ARDUINOJSON_VERSION_MAJOR >= 7
// Code สำหรับ Version 7
JsonDocument intern_docJSON;
#else
// Code สำหรับ Version 6
StaticJsonDocument<512> intern_docJSON;
#endif
// StaticJsonDocument<512> intern_docJSON;
boolean Attribute_MQTT_core::isBypassAutoUpdate = false; // false = autoUpdate, true = unuse autoUpdate
boolean Attribute_MQTT_core::usingCheckUpdate = false;
boolean Attribute_MQTT_core::checkFirmwareUptodate = false;
boolean Attribute_MQTT_core::isFirmwareUptodate = false;
boolean Attribute_MQTT_core::flagAutoOTA = true;
size_t Attribute_MQTT_core::calculate_chunkSize = 0; // calculate part size from clientBuffer
size_t Attribute_MQTT_core::incomingChunkSize = 0;
unsigned int Attribute_MQTT_core::fw_total_size = 0;
unsigned int Attribute_MQTT_core::fw_count_chunk = 0;
unsigned int Attribute_MQTT_core::chunk_size = 0;
unsigned int Attribute_MQTT_core::totalChunk = 0;
unsigned int Attribute_MQTT_core::current_chunk = 0;
unsigned int Attribute_MQTT_core::current_size = 0;
unsigned int Attribute_MQTT_core::default_chunk_size = 4096;
int Attribute_MQTT_core::clientNetInterface;
Client *Attribute_MQTT_core::ClientNET = NULL;
PubSubClient *Attribute_MQTT_core::mqtt_client = NULL; // MQTT Client
boolean Attribute_MQTT_core::ctrl_regis_key = false;
boolean Attribute_MQTT_core::ctrl_regis_pta = false;
boolean Attribute_MQTT_core::ctrl_regis_json = false;
boolean Attribute_MQTT_core::conf_regis_key = false;
boolean Attribute_MQTT_core::conf_regis_pta = false;
boolean Attribute_MQTT_core::conf_regis_json = false;
boolean Attribute_MQTT_core::resp_regis = false;
boolean Attribute_MQTT_core::ctrl_jsonOBJ = false;
boolean Attribute_MQTT_core::conf_jsonOBJ = false;
boolean Attribute_MQTT_core::using_Checksum = false;
boolean Attribute_MQTT_core::useAdvanceCallback = false;
String Attribute_MQTT_core::ext_Token;
String Attribute_MQTT_core::ext_EndPoint;
boolean Attribute_MQTT_core::inProcessOTA = false;
boolean Attribute_MQTT_core::useBuiltInSensor = false;
boolean Attribute_MQTT_core::remain_ota_fw_info_match = false;
boolean Attribute_MQTT_core::flag_remain_ota = false;
String Attribute_MQTT_core::valid_remain_fw_name = "";
unsigned int Attribute_MQTT_core::valid_remain_fw_size = 0;
boolean Attribute_MQTT_core::triggerRemainOTA = true;
boolean Attribute_MQTT_core::triggerRemainSub = true;
boolean Attribute_MQTT_core::remind_Event_GET_FW_infoOTA = false;
boolean Attribute_MQTT_core::startReqDownloadOTA = false;
boolean Attribute_MQTT_core::checkTimeout_request_download_fw = false;
unsigned int Attribute_MQTT_core::timeout_req_download_fw = 180000;
unsigned long Attribute_MQTT_core::prv_cb_timeout_millis = 0;
#if ARDUINOJSON_VERSION_MAJOR >= 7
// Code สำหรับ Version 7
JsonDocument Attribute_MQTT_core::docClientConf;
JsonDocument *Attribute_MQTT_core::adjDoc = new JsonDocument();
JsonDocument *Attribute_MQTT_core::docSensor = new JsonDocument();

#else
// Code สำหรับ Version 6
StaticJsonDocument<512> Attribute_MQTT_core::docClientConf;
DynamicJsonDocument *Attribute_MQTT_core::adjDoc = new DynamicJsonDocument(256);
DynamicJsonDocument *Attribute_MQTT_core::docSensor = new DynamicJsonDocument(1024);
#endif
// StaticJsonDocument<512> Attribute_MQTT_core::docClientConf;
// DynamicJsonDocument *Attribute_MQTT_core::adjDoc = new DynamicJsonDocument(256);
// DynamicJsonDocument *Attribute_MQTT_core::docSensor = new DynamicJsonDocument(1024);
// 1.1.2
unsigned int Attribute_MQTT_core::delayRequest_download = 0;
boolean Attribute_MQTT_core::checkUpdate_inside = false;
unsigned int Attribute_MQTT_core::delayCheckUpdate_inside = 60000;
size_t Attribute_MQTT_core::buffer_JSON_bufferSize = 1024;
size_t Attribute_MQTT_core::max_payload_report = 50000;
int Attribute_MQTT_core::matchMsgId_cb = -1;
int Attribute_MQTT_core::matchMsgId_send = -1;
int Attribute_MQTT_core::maxRetransmit = 5;
boolean Attribute_MQTT_core::isMatchMsgId = false;
boolean Attribute_MQTT_core::reqRetransmit = false;

OTA_INFO MAGELLAN_MQTT_device_core::OTA_info;
func_callback_registerList MAGELLAN_MQTT_device_core::duplicate_subs_list;

boolean attemp_download_1 = false;
boolean attemp_download_2 = false;

// 1.2.1
SubscribesCheckLists Attribute_MQTT_core::sub_check_list;

// 1.2.2
unsigned long Attribute_MQTT_core::refPercentOTA = 0;
bool Attribute_MQTT_core::flagPrintProgressOTA = false;
std::function<void()> Attribute_MQTT_core::cb_before_restart = nullptr;

static void ensureJsonDocPointersReady()
{
#if ARDUINOJSON_VERSION_MAJOR >= 7
  if (attr.adjDoc == NULL)
  {
    attr.adjDoc = new JsonDocument();
  }
  if (attr.docSensor == NULL)
  {
    attr.docSensor = new JsonDocument();
  }
#endif
}

String b2str(byte *payload, unsigned int length) // convert byte* to String
{
  // Use String(const char*, length) to avoid VLA stack allocation risk on large payloads
  return String(reinterpret_cast<const char *>(payload), length);
}

typedef struct
{
  String registerKey;
  ctrl_handleCallback ctrl_key_callback;
  ctrl_Json_handleCallback ctrl_Json_callback;
  ctrl_PTAhandleCallback ctrl_pta_callback;
  ctrl_JsonOBJ_handleCallback ctrl_obj_callback;

  conf_handleCallback conf_key_callback;
  conf_Json_handleCallback conf_json_callback;
  conf_PTAhandleCallback conf_pta_callback;
  conf_JsonOBJ_handleCallback conf_obj_callback;
  resp_callback resp_h_callback;

  void *next;
  unsigned int Event;
  unsigned int RESP_Events;

} regisAPI;

regisAPI *_startRegis = NULL;         // buffer callback <void String payload>
regisAPI *_startRegisPTA = NULL;      // buffer callback <void String key, String value>
regisAPI *_startRegisJSON = NULL;     // buffer callback <void String key, String value>
regisAPI *_startRegisConf = NULL;     // buffer callback <void String payload>
regisAPI *_startRegisPTAConf = NULL;  // buffer callback <void String key, String value>
regisAPI *_startRegisJSONConf = NULL; // buffer callback <void String key, String value>
regisAPI *_startRESP = NULL;

regisAPI *_startOBJ_CTRL = NULL;
regisAPI *_startOBJ_CONF = NULL;

// boolean ext_useAdvanceCallback = false; //duplicate var

Centric centric;

void (*cb_internal)(EVENTS events, char *);

JsonObject deJson(String jsonContent)
{
  JsonObject buffer;
  intern_docJSON.clear();
  if (jsonContent != NULL && jsonContent != "clear")
  {
    DeserializationError error = deserializeJson(intern_docJSON, jsonContent);
    buffer = intern_docJSON.as<JsonObject>();
    if (error)
      MG_LOG_E("# Error to DeserializeJson Control");
  }
  return buffer;
}

// Unified helper: parse Magellan JSON envelope and extract the inner payload field.
// Avoids redundant indexOf() on raw string after JSON is already parsed.
// Returns "40300" if Code != "20000" or the key is not present.
static String deJsonExtract(const String &jsonContent, const char *primaryKey, const char *fallbackKey = nullptr)
{
  JsonObject buffdoc = deJson(jsonContent);
  const char *statusCode = buffdoc["Code"] | "";
  if (strcmp(statusCode, "20000") != 0)
    return String("40300");
  if (!buffdoc[primaryKey].isNull())
    return buffdoc[primaryKey].as<String>();
  if (fallbackKey != nullptr && !buffdoc[fallbackKey].isNull())
    return buffdoc[fallbackKey].as<String>();
  return String("40300");
}

// Delegates to unified helper — Delta or Sensor field from control envelope
String deControl(String jsonContent) { return deJsonExtract(jsonContent, "Delta", "Sensor"); }
// Delegates to unified helper — Config field from config envelope
String deConfig(String jsonContent) { return deJsonExtract(jsonContent, "Config"); }
/////////// Feature OTA function none member in class //////////////////////

// Helper: build "api/v2/thing/<token>/<suffix>" into a stack buffer.
// bufSize must be >= strlen(token) + strlen(suffix) + 16.
static inline void buildMgTopic(char *buf, size_t bufSize, const char *suffix)
{
  snprintf(buf, bufSize, "api/v2/thing/%s/%s", attr.ext_Token.c_str(), suffix);
}

boolean pubClientConfig(String payload) // for external function member
{
  char topic[100];
  buildMgTopic(topic, sizeof(topic), "config/persist");
  boolean Pub_status = attr.mqtt_client->publish(topic, payload.c_str());
  const char *_debug_ = Pub_status ? "Success" : "Failure";
  MG_LOG_I_S("# Save ClientConfig: " + String(_debug_));
  MG_LOG_I_S("# [Clientconfigs]: " + payload);
  return Pub_status;
}

boolean sub_InfoOTA()
{
  attr.sub_check_list.SetSubscription(SubFirmwareInfo, true);
  char topic[100];
  buildMgTopic(topic, sizeof(topic), "firmwareinfo/resp");
  boolean Sub_status = attr.mqtt_client->subscribe(topic);
  // Serial.println(topic);
  String Debug = (Sub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Subscribe Firmware Information: " + Debug);
  return Sub_status;
}

boolean unsub_InfoOTA()
{
  attr.sub_check_list.SetSubscription(SubFirmwareInfo, false);
  char topic[100];
  buildMgTopic(topic, sizeof(topic), "firmwareinfo/resp");
  boolean Sub_status = attr.mqtt_client->unsubscribe(topic);
  // Serial.println(topic);
  String Debug = (Sub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Unsubscribe Firmware Information: " + Debug);
  return Sub_status;
}

boolean pub_Info()
{
  char topic[100];
  buildMgTopic(topic, sizeof(topic), "firmwareinfo/req");
  boolean Pub_status = attr.mqtt_client->publish(topic, " ");
  // Serial.println(topic);
  String Debug = (Pub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Request Firmware Information: " + Debug);
  return Pub_status;
}

boolean sub_DownloadOTA()
{
  attr.sub_check_list.SetSubscription(SubFirmwareDownload, true);
  char topic[100];
  buildMgTopic(topic, sizeof(topic), "firmwaredownload/resp/+");
  boolean Sub_status = attr.mqtt_client->subscribe(topic);
  // Serial.println(topic);
  String Debug = (Sub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Subscribe Firmware Download: " + Debug);
  return Sub_status;
}

boolean unsub_DownloadOTA()
{
  attr.sub_check_list.SetSubscription(SubFirmwareDownload, false);
  char topic[100];
  buildMgTopic(topic, sizeof(topic), "firmwaredownload/resp/+");
  boolean Sub_status = attr.mqtt_client->unsubscribe(topic);
  // Serial.println(topic);
  String Debug = (Sub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Unsubscribe Firmware Download: " + Debug);
  return Sub_status;
}

boolean pub_Download(unsigned int fw_chunk, size_t chunk_size)
{
  if (fw_chunk == 0)
    attr.startReqDownloadOTA = true;
  attr.checkTimeout_request_download_fw = true;
  char topic[128];
  snprintf(topic, sizeof(topic),
           "api/v2/thing/%s/firmwaredownload/req/%u?filesize=%u",
           attr.ext_Token.c_str(), fw_chunk, (unsigned)chunk_size);
  boolean Pub_status = attr.mqtt_client->publish(topic, " ");
  // Serial.println(topic);
  String Debug = (Pub_status == true) ? "Success" : "Failure";
  MG_LOG_D_S("# ->Request Firmware Download on chunk: " + String(fw_chunk) + " Status: " + Debug);
  MG_LOG_D_S("# ->Chunk size request: " + String(chunk_size));
  return Pub_status;
}
// pre ver.1.1.2
boolean pub_Download(unsigned int fw_chunk, size_t chunk_size, String versionName)
{
  if (fw_chunk == 0)
    attr.startReqDownloadOTA = true;
  attr.checkTimeout_request_download_fw = true;
  char topic[128];
  snprintf(topic, sizeof(topic),
           "api/v2/thing/%s/firmwaredownload/req/%u?filesize=%u",
           attr.ext_Token.c_str(), fw_chunk, (unsigned)chunk_size);
  char payload_buf[80];
  snprintf(payload_buf, sizeof(payload_buf), "{\"FirmwareVersion\":\"%s\"}", versionName.c_str());
  boolean Pub_status = attr.mqtt_client->publish(topic, payload_buf);
  // Serial.println(topic);
  String Debug = (Pub_status == true) ? "Success" : "Failure";
  MG_LOG_D_S("# ->Request Firmware Download by version name \"" + versionName + "\"on chunk: " + String(fw_chunk) + " Status: " + Debug);
  MG_LOG_D_S("# ->Chunk size request: " + String(chunk_size));
  return Pub_status;
}

// Formats {"description":"Firmware <version>"} onto a stack buffer — avoids String alloc at every OTA progress call site
static inline void fmtFWDesc(char *buf, size_t sz)
{
  snprintf(buf, sz, "{\"description\":\"Firmware %s\"}",
           MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion.c_str());
}

// Formats {"errordescription":"Firmware <version> <detail>"} onto a stack buffer
static inline void fmtFWErrDesc(char *buf, size_t sz, const char *detail)
{
  snprintf(buf, sz, "{\"errordescription\":\"Firmware %s %s\"}",
           MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion.c_str(), detail);
}

boolean pub_UpdateProgress(const char *FOTA_State, const char *description)
{
  // Reduced from 3000ms — allows MQTT broker to process before next OTA step
  delay(500);
  char topic[128];
  snprintf(topic, sizeof(topic),
           "api/v2/thing/%s/fotaupdateprogress/req/?FOTAState=%s",
           attr.ext_Token.c_str(), FOTA_State);
  boolean Pub_status = false;
  if (strstr(description, "description") != nullptr || strstr(description, "Version") != nullptr)
  {
    // Single publish — duplicate was redundant and doubled broker load
    Pub_status = attr.mqtt_client->publish(topic, description);
    MG_LOG_I("# STATE OTA Description: %s", description);
  }
  else
  {
    Pub_status = attr.mqtt_client->publish(topic, "");
  }

  const char *Debug = Pub_status ? "Success" : "Failure";
  MG_LOG_I("# Update Progress OTA state: \"%s\" Status: %s", FOTA_State, Debug);
  return Pub_status;
}

boolean check_remain_fw_isMatch(String validate_fw_name, unsigned int validate_fw_size, String descriptionWhenFail)
{

  if (MAGELLAN_MQTT_device_core::OTA_info.firmwareName == validate_fw_name &&
      MAGELLAN_MQTT_device_core::OTA_info.firmwareTotalSize == validate_fw_size)
  {
    MG_LOG_I("# Check firmware information incoming is match OTA still working"); // re new description
    return true;
  }
  else
  {
    MG_LOG_E("# Check firmware information does not match after reconnect");
    // pub_UpdateProgress("FAILED","{\"errordescription\":\""+descriptionWhenFail+"(version. "+ MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion+")\"}");
    char _pb[128];
    snprintf(_pb, sizeof(_pb), "{\"errordescription\":\"Downloading firmware %s %s\"}",
             MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion.c_str(), descriptionWhenFail.c_str());
    pub_UpdateProgress("FAILED", _pb);
    configOTAFile.saveSuccessOrFail("fail");

    return false;
  }
}

void checkUpdate(String topic, String payload)
{
  if (topic.indexOf("/firmwareinfo/resp") != -1)
  {
    MG_LOG_I("# Check incoming firmware update");
    if (payload != "{}" && payload.indexOf("20000") != -1)
    {
      JsonObject fw_doc = deJson(payload);
      String name = fw_doc["namefirmware"];
      unsigned int size = fw_doc["sizefirmware"];
      String version = fw_doc["versionfirmware"];
      String c_sum = fw_doc["checksum"];
      String al_c_sum = fw_doc["checksumAlgorithm"];
      attr.valid_remain_fw_name = name;
      attr.valid_remain_fw_size = size;
      if (name == "null" && size <= 0)
      {
        MG_LOG_E("# [warning]Firmware Information is wrong or empty!");
      }
      else
      {
        attr.checkFirmwareUptodate = configOTAFile.compareFirmwareIsUpToDate(fw_doc);

        MAGELLAN_MQTT_device_core::OTA_info.firmwareName = name;
        MAGELLAN_MQTT_device_core::OTA_info.firmwareTotalSize = size;
        MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion = version;
        MAGELLAN_MQTT_device_core::OTA_info.checksum = c_sum;
        MAGELLAN_MQTT_device_core::OTA_info.checksumAlgorithm = al_c_sum;

        if (attr.checkFirmwareUptodate)
        {
          // MAGELLAN_MQTT_device_core::OTA_info.firmwareIsUpToDate = UP_TO_DATE;
          MAGELLAN_MQTT_device_core::OTA_info.firmwareIsUpToDate = OTA_state::UP_TO_DATE; // 1.2.1
        }
        else
        {
          // MAGELLAN_MQTT_device_core::OTA_info.firmwareIsUpToDate = OUT_OF_DATE;
          MAGELLAN_MQTT_device_core::OTA_info.firmwareIsUpToDate = OTA_state::OUT_OF_DATE; // 1.2.1
        }
        attr.usingCheckUpdate = false;
      }
    }
    else if (payload.indexOf("40400") != -1)
    {
      MAGELLAN_MQTT_device_core::OTA_info.firmwareIsUpToDate = OTA_state::NOT_AVAILABLE_STATE; // 1.2.1
      attr.usingCheckUpdate = false;
    }
  }
}

void save_fw_info(String topic, String payload)
{
  // Serial.println("-save_fw_info: "+payload);
  if (topic.indexOf("/firmwareinfo/resp") != -1)
  {
    MG_LOG_I("# Detect incoming Firmware Information");
    if (payload != "{}" && payload.indexOf("20000") != -1)
    {
      JsonObject fw_doc = deJson(payload);
      String name = fw_doc["namefirmware"];
      unsigned int size = fw_doc["sizefirmware"];
      String version = fw_doc["versionfirmware"];
      String c_sum = fw_doc["checksum"];
      String al_c_sum = fw_doc["checksumAlgorithm"];
      attr.valid_remain_fw_name = name;
      attr.valid_remain_fw_size = size;
      if (name == "null" && size <= 0)
      {
        MG_LOG_E("# [warning]Firmware Information is wrong or empty!");
      }
      else // validate data pass
      {
        if ((attr.inProcessOTA) && (attr.flag_remain_ota)) // inprocess but remain connect with broker
        {
          attr.remain_ota_fw_info_match = check_remain_fw_isMatch(attr.valid_remain_fw_name,
                                                                  attr.valid_remain_fw_size, "is mismatch from server"); // OTA renew description

          attr.flag_remain_ota = false;
          MG_LOG_I("# Validate from reconnect Firmware OTA Information #");
          // Serial.println("  ->Firmware Name: "+MAGELLAN_MQTT_device_core::OTA_info.firmwareName);
          // Serial.println("  ->Firmware total size: "+String(MAGELLAN_MQTT_device_core::OTA_info.firmwareTotalSize));
          MG_LOG_I_S("  ->Firmware version: " + MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion);
          // Serial.println("  ->Firmware checksum Algorithm: "+MAGELLAN_MQTT_device_core::OTA_info.checksumAlgorithm);
          // Serial.println("  ->Firmware checksum: "+MAGELLAN_MQTT_device_core::OTA_info.checksum);
          if (!attr.remain_ota_fw_info_match)
          {
            MG_LOG_E("# [ERROR] Device must restart because firmware change #");
            MG_LOG_E("# firmware not match validate OTA information after reconnect");
            delay(5000);
            ESP.restart();
          }
          else
          {
            pub_Download(attr.fw_count_chunk, attr.chunk_size);
          }
        }
        else if ((attr.inProcessOTA) && (!attr.triggerRemainOTA)) // inprocess get info fw
        {
          attr.remain_ota_fw_info_match = check_remain_fw_isMatch(
              attr.valid_remain_fw_name,
              attr.valid_remain_fw_size,
              "is obsolete"); // re new OTA description
          if (!attr.remain_ota_fw_info_match)
          {
            MG_LOG_I("# Firmware OTA Information Incoming While inProcessOTA #");
            // Serial.println("  ->Firmware Name: "+MAGELLAN_MQTT_device_core::OTA_info.firmwareName);
            // Serial.println("  ->Firmware total size: "+String(MAGELLAN_MQTT_device_core::OTA_info.firmwareTotalSize));
            MG_LOG_I_S("  ->Firmware version: " + MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion);
            // Serial.println("  ->Firmware checksum Algorithm: "+MAGELLAN_MQTT_device_core::OTA_info.checksumAlgorithm);
            MG_LOG_I_S("  ->Firmware checksum: " + MAGELLAN_MQTT_device_core::OTA_info.checksum);
            MG_LOG_E("# [ERROR] Device must restart because firmware change #");
            delay(5000);
            ESP.restart();
          }
        }

        if (!attr.inProcessOTA) // first get info and save to variable
        {
          attr.isFirmwareUptodate = configOTAFile.compareFirmwareOTA(fw_doc);

          MAGELLAN_MQTT_device_core::OTA_info.firmwareIsUpToDate = ((attr.isFirmwareUptodate == true) ? OTA_state::UP_TO_DATE : OTA_state::OUT_OF_DATE);
          // MAGELLAN_MQTT_device_core::OTA_info.firmwareName = name;
          MAGELLAN_MQTT_device_core::OTA_info.firmwareName = configOTAFile.readSpacificFromConfFile("namefirmware");
          // MAGELLAN_MQTT_device_core::OTA_info.firmwareTotalSize = size;
          size_t buffReadSizefirmware = configOTAFile.readSpacificFromConfFile("sizefirmware").toInt();
          attr.fw_total_size = buffReadSizefirmware;
          MAGELLAN_MQTT_device_core::OTA_info.firmwareTotalSize = buffReadSizefirmware;
          // MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion = version;
          MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion = configOTAFile.readSpacificFromConfFile("versionfirmware");
          // MAGELLAN_MQTT_device_core::OTA_info.checksum = c_sum;
          MAGELLAN_MQTT_device_core::OTA_info.checksum = configOTAFile.readSpacificFromConfFile("checksum");
          // MAGELLAN_MQTT_device_core::OTA_info.checksumAlgorithm = al_c_sum;
          MAGELLAN_MQTT_device_core::OTA_info.checksumAlgorithm = configOTAFile.readSpacificFromConfFile("checksumAlgorithm");

          unsigned int b_cal_chunk_todo = (attr.fw_total_size / attr.chunk_size) + 1;

          // ver 1.1.2 UPDATED
          if (attr.isFirmwareUptodate)
          {
            JsonObject infoCurrentFW = configOTAFile.readObjectLastedOTA();
            String fw_ver = infoCurrentFW["versionfirmware"];
            String fw_size = infoCurrentFW["sizefirmware"];
            //   pub_UpdateProgress("UPDATED","{\"description\":\"Device already uptodate firmware version: "+
            //   String(fw_ver)+
            // " firmware size: "+String(fw_size)+"\"}");
            char _pb[128];
            snprintf(_pb, sizeof(_pb), "{\"description\":\"Firmware %s is Up to Date\",\"Version\":\"%s\"}",
                     fw_ver.c_str(), fw_ver.c_str());
            pub_UpdateProgress("UPDATED", _pb);
          }
          // ver 1.1.2 UPDATED

          attr.totalChunk = b_cal_chunk_todo;

          if (!attr.isFirmwareUptodate)
          {
            MG_LOG_I_S("# Estimate OTA toltal request chunk : " + String(attr.totalChunk));
            // pub_UpdateProgress("INITIALIZE","{\"description\":\"Initialize firmware version: "+MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion+
            // " size: "+ String(attr.fw_total_size)+"\"}");
            char _pb[96];
            fmtFWDesc(_pb, sizeof(_pb));
            pub_UpdateProgress("INITIALIZE", _pb);
            MAGELLAN_MQTT_device_core::OTA_info.isReadyOTA = true;
            MG_LOG_I("# Firmware OTA information available #");
            // Serial.println("  ->Firmware Name: "+MAGELLAN_MQTT_device_core::OTA_info.firmwareName);
            // Serial.println("  ->Firmware total size: "+String(MAGELLAN_MQTT_device_core::OTA_info.firmwareTotalSize));
            MG_LOG_I_S("  ->Firmware version: " + MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion);
            // Serial.println("  ->Firmware checksum Algorithm: "+MAGELLAN_MQTT_device_core::OTA_info.checksumAlgorithm);
            MG_LOG_I_S("  ->Firmware checksum: " + MAGELLAN_MQTT_device_core::OTA_info.checksum);

            // save Client config when firmware change and file ota config change

            attr.remind_Event_GET_FW_infoOTA = false;
          }
        }
      }
    }
    else
    {
      MG_LOG_E("# Fail to get firmware Information or you don't have firmware OTA");
      MG_LOG_D_S("# Detail: " + payload);
    }
  }
}

String ERORRdescriptionUpdate()
{
  switch (Update.getError())
  {
  case 0:
    return "UPDATE_ERROR_OK";
  case 1:
    return "UPDATE_ERROR_WRITE";
  case 2:
    return "UPDATE_ERROR_ERASE";
  case 3:
    return "UPDATE_ERROR_READ";
  case 4:
    return "UPDATE_ERROR_SPACE";
  case 5:
    return "UPDATE_ERROR_SIZE";
  case 6:
    return "UPDATE_ERROR_STREAM";
  case 7:
    return "UPDATE_ERROR_MD5";
  case 8:
    return "UPDATE_ERROR_MAGIC_BYTE";
  case 9:
    return "UPDATE_ERROR_ACTIVATE{firmware is mismatch this board}";
  case 10:
    return "UPDATE_ERROR_NO_PARTITION";
  case 11:
    return "UPDATE_ERROR_BAD_ARGUMENT";
  case 12:
    return "UPDATE_ERROR_ABORT";
  default:
    return "ERROR_UNKNOWN";
  }
}

void validate_lostOTA_Data_incoming()
{
  if (attr.current_chunk + 1 < attr.totalChunk)
  {
    if (attr.incomingChunkSize < attr.chunk_size)
    {
      MG_LOG_E("# [Warning]Lost some data while in process OTA");
      MG_LOG_E("# [Warning]Device must restart");
      //  pub_UpdateProgress("FAILED","{\"errordescription\":\"Data incoming lost or incorrect (version. "+ MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion+")\"}");
      char _pb3[128];
      snprintf(_pb3, sizeof(_pb3), "{\"errordescription\":\"Downloading firmware %s is incorrect or lost data\"}",
               MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion.c_str());
      pub_UpdateProgress("FAILED", _pb3);
      configOTAFile.saveSuccessOrFail("fail");

      delay(5000);
      ESP.restart();
    }
  }
}

// v1.2.2
void updatePercentProgressOTA(unsigned int percent)
{
  if (percent % 10 == 0 && percent <= 100 && attr.refPercentOTA != percent)
  {
    attr.flagPrintProgressOTA = true;
    attr.refPercentOTA = percent;
  }
  if (!attr.flagPrintProgressOTA)
    return;
  // snprintf avoids 3 temporary String allocations on each progress tick
  char msgProgress[96];
  snprintf(msgProgress, sizeof(msgProgress),
           "{\"description\":\"[%u%%] FW: %s\"}",
           percent, MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion.c_str());
  pub_UpdateProgress("DOWNLOADING", msgProgress);
  attr.flagPrintProgressOTA = false;
}

void updateFirmware(uint8_t *data, size_t len)
{
  Update.write(data, len);
  attr.current_size += len;
  attr.incomingChunkSize = (int)len;
  MG_LOG_D_S("# <-Incoming chunk size: " + String(attr.incomingChunkSize));
  unsigned int calc_percent = map(attr.current_size, 0, attr.fw_total_size, 0, 100);
  MG_LOG_D_S("# <-Current firmware size: " + String(attr.current_size) + "/" + String(attr.fw_total_size) + " => [" + String(calc_percent) + " %]");
  updatePercentProgressOTA(calc_percent);
  validate_lostOTA_Data_incoming();
  if (attr.current_size != attr.fw_total_size)
  {
    return;
  }

  if (Update.end(true))
  {
    // pub_UpdateProgress("DOWNLOADED","");
    char _dl_pb[96];
    fmtFWDesc(_dl_pb, sizeof(_dl_pb));
    pub_UpdateProgress("DOWNLOADED", _dl_pb);
    if (attr.using_Checksum)
    {
      pub_UpdateProgress("VERIFIED", "");
    }
    MG_LOG_I("# OTA done!");
    if (Update.isFinished())
    {
      MG_LOG_I("# Update successfully completed. Rebooting.");
      configOTAFile.saveSuccessOrFail("done");

      String readfileConfig = configOTAFile.readConfigFileOTA();
      configOTAFile.saveLastedOTA(readfileConfig);

      String fw_infoInFIleSys;
      JsonObject fw_last = configOTAFile.readObjectLastedOTA();
      int bufferFW_size = fw_last["sizefirmware"];

      fw_last.remove("namefirmware");
      fw_last.remove("sizefirmware");
      fw_last.remove("checksumAlgorithm");
      String bufferFW_v = fw_last["versionfirmware"];

      char _up_pb[64];
      snprintf(_up_pb, sizeof(_up_pb), "{\"Version\":\"%s\"}", bufferFW_v.c_str());
      pub_UpdateProgress("UPDATED", _up_pb);

      fw_last["firmwareVersion"] = bufferFW_v;
      fw_last.remove("versionfirmware");
      serializeJson(fw_last, fw_infoInFIleSys);

      if (fw_infoInFIleSys.indexOf("null") == -1)
      {
        pubClientConfig(fw_infoInFIleSys);
      }
      else if ((bufferFW_v.length() > 4 || bufferFW_v.indexOf("null") == -1) && (fw_infoInFIleSys.indexOf("null") != -1)) // handle if fw version !null but some key value found null is still pub client config
      {
        pubClientConfig(fw_infoInFIleSys);
      }

      else if ((bufferFW_v.indexOf("null") != -1) && (bufferFW_size > 0))
      {
        pubClientConfig(fw_infoInFIleSys);
      }

      // Serial.println("#Debug: "+ configOTAFile.readConfigFileOTA());
      MG_LOG_I("# OTA complete, rebooting to new firmware...");
      // Engine-specific modem shutdown before restart (e.g. SIM7600E calls GSM.shutdown()).
      // TinyGSM engine leaves cb_before_restart nullptr to avoid xEventGroupClearBits
      // assert crash (_gsm_udp_flags is NULL when GSMUdp is never constructed in PPP mode).
      if (attr.cb_before_restart)
        attr.cb_before_restart();
      delay(1000);
      ESP.restart();
    }
    else
    {
      //  pub_UpdateProgress("FAILED","{\"errordescription\":\"something_went_wrong (version. "+ MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion+")\"}");
      char _f1_pb[128];
      fmtFWErrDesc(_f1_pb, sizeof(_f1_pb), "Something went wrong (UNKNOWN ERROR)");
      pub_UpdateProgress("FAILED", _f1_pb);

      MG_LOG_E("# Update not finished? Something went wrong!");
      configOTAFile.saveSuccessOrFail("fail");
    }
  }
  else
  {
    String error_des = ERORRdescriptionUpdate();
    MG_LOG_E_S("# OTA Fail Error Occurred. Error #: " + error_des + " # Error Enum {" + String(Update.getError()) + "}");
    // pub_UpdateProgress("FAILED","{\"errordescription\":\""+ error_des +" (version. "+ MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion+")\"}");
    char _f2_pb[160];
    snprintf(_f2_pb, sizeof(_f2_pb), "{\"errordescription\":\"Firmware %s : %s\"}",
             MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion.c_str(), error_des.c_str());
    pub_UpdateProgress("FAILED", _f2_pb);
    configOTAFile.saveSuccessOrFail("fail");
  }
  delay(5000);
  ESP.restart();
}

void hook_FW_download(String topic, uint8_t *payload, unsigned int length)
{
  // Reuse the result of the first indexOf to avoid scanning the string twice
  const int fwRespIdx = topic.indexOf("/firmwaredownload/resp/");
  if (fwRespIdx != -1)
  {
    // "/firmwaredownload/resp/" is 23 chars; chunk number follows directly
    String crrnt_part = topic.substring(fwRespIdx + 23); // crrnt_part is part start from index 0
    attr.current_chunk = crrnt_part.toInt();
    // MAGELLAN_MQTT_device_core::OTA_info.currentChunk = attr.current_chunk+1;
    MG_LOG_D_S("# <-Firmware current chunk: " + String(attr.current_chunk + 1) + "/" + String(attr.totalChunk));
    if (length > 0 && (attr.fw_count_chunk <= attr.totalChunk))
    {
      attr.checkTimeout_request_download_fw = false;
      attr.prv_cb_timeout_millis = millis();
      updateFirmware(payload, length);
      attr.fw_count_chunk++;
      attemp_download_1 = false;
      attemp_download_2 = false;

      if (attr.delayRequest_download > 0)
      {
        delay(attr.delayRequest_download);
        MG_LOG_D_S("# Using delay download every chunk in: " + String(attr.delayRequest_download) + " ms.");
        pub_Download(attr.fw_count_chunk, attr.chunk_size);
      }
      else
      {
        pub_Download(attr.fw_count_chunk, attr.chunk_size);
      }

      if (!attr.inProcessOTA)
      {
        // pub_UpdateProgress("DOWNLOADING", "{\"description\":\"downloading firmware version: "+MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion+
        // " size: "+ String(attr.fw_total_size)+"\"}");
        char _dling_pb[96];
        fmtFWDesc(_dling_pb, sizeof(_dling_pb));
        pub_UpdateProgress("DOWNLOADING", _dling_pb);
      }
      attr.inProcessOTA = true;
      MAGELLAN_MQTT_device_core::OTA_info.inProcessOTA = attr.inProcessOTA;
    }
    if (attr.fw_count_chunk == attr.totalChunk)
    {
      if (attr.current_size != attr.fw_total_size)
      {
        MG_LOG_E("#[Warning] Complete Request chunk but lost or incorrect DATA from OTA");
        MG_LOG_E("#[Warning] Must restart board");
        // pub_UpdateProgress("FAILED","{\"errordescription\":\"Complete request total of chunk but lost or incorrect DATA from OTA (version. "+ MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion+")\"}");
        char _df_pb[128];
        snprintf(_df_pb, sizeof(_df_pb), "{\"errordescription\":\"Downloaded firmware %s is incorrect or lost data\"}",
                 MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion.c_str());
        pub_UpdateProgress("FAILED", _df_pb);
        configOTAFile.saveSuccessOrFail("fail");

        delay(3000);
        ESP.restart();
      }
    }
  }
}

/////////// Feature OTA function none member inclass //////////////////////

// ver.1.1.2
//  @attention function compare between MsgId_cb(RCV from cb) and MsgId_send  when using reqRetransmission
void checkRequestRetransmit()
{
  if (attr.reqRetransmit)
  {
    attr.isMatchMsgId = (attr.matchMsgId_send == attr.matchMsgId_cb) ? true : false;
    if (attr.isMatchMsgId)
    {
      MG_LOG_I_S("\n# ======================================== Received MsgId: " + String(attr.matchMsgId_send) + " retransmit from server ========================================");

      attr.reqRetransmit = false;
    }
  }
}

void msgCallback_internalHandler(char *topic, byte *payload, unsigned int length)
{
  String action = "ERROR";
  unsigned int buffEvent = M_ERROR;
  String b_topic = String(topic);
  String _payload = b2str(payload, length);
  String key = "null"; // if this topic is'nt PLAINTEXT
  String code = "0";
  int _MsgId = -1;
  // Serial.println("DEBUG");
  // Serial.println(b_topic);
  // Serial.println(_payload);
  // Serial.println("DEBUG");
  EVENTS intern_EVENT;
  intern_EVENT.RESP = "EMPTY";

  regisAPI *handleRegisPTA = _startRegisPTA;
  regisAPI *handleRegisJSON = _startRegisJSON;
  regisAPI *handleRegisJSON_CTRL_OBJ = _startOBJ_CTRL;
  regisAPI *handleRegisKEY = _startRegis;

  regisAPI *handleRegisPTAConf = _startRegisPTAConf;
  regisAPI *handleRegisJSONConf = _startRegisJSONConf;
  regisAPI *handleRegisJSON_CONF_OBJ = _startOBJ_CONF;
  regisAPI *handleRegisKEYConf = _startRegisConf;

  regisAPI *handleRESP = _startRESP;
  // OTA Feature /////////////////
  if ((attr.usingCheckUpdate) && (!attr.inProcessOTA))
  {
    checkUpdate(b_topic, _payload);
  }
  else if (!attr.usingCheckUpdate && attr.flagAutoOTA)
  {
    save_fw_info(b_topic, _payload);
  }
  hook_FW_download(b_topic, payload, length);
  ///////////////////////////////
  char *b_payload = (char *)_payload.c_str(); // payload for advance_cb and endpoint centric

  if (b_topic.indexOf("/auth/resp/") != -1)
  {
    buffEvent = TOKEN;
    action = "TOKEN";
    attr.ext_Token = *&_payload;
  }
  if (b_topic.indexOf("/delta/resp/pta") != -1)
  {
    int indexfound2 = b_topic.indexOf("="); // b_topic is already String — no copy needed
    String keyOnTopic = b_topic.substring(indexfound2 + 1);
    key = keyOnTopic;
    buffEvent = CONTROL_PLAINTEXT;
    action = "CONTROL_PLAINTEXT";

    if ((_payload == "40300" || (_payload == "40400") && (_payload.length() == 5)))
    {
      code = _payload;
      intern_EVENT.RESP = "FAIL";
    }
    else
    {
      code = "20000";
      intern_EVENT.RESP = "SUCCESS";
    }

    if (attr.ctrl_regis_key)
    {
      while (handleRegisKEY != NULL)
      {
        if (handleRegisKEY->registerKey == key)
        {
          break;
        }
        else
        {
          handleRegisKEY = (regisAPI *)handleRegisKEY->next;
        }
      }
      if (handleRegisKEY != NULL)
      {
        if (handleRegisKEY->registerKey == key)
        {
          handleRegisKEY->ctrl_key_callback(_payload);
        }
      }
    }
    if (attr.ctrl_regis_pta)
    {
      while (handleRegisPTA != NULL)
      {
        if (handleRegisPTA->Event == buffEvent)
        {
          break;
        }
        else
        {
          handleRegisPTA = (regisAPI *)handleRegisPTA->next;
        }
      }
      if (handleRegisPTA != NULL)
      {
        handleRegisPTA->ctrl_pta_callback(key, _payload);
      }
    }
  }

  else if (b_topic.indexOf("/delta/resp") != -1)
  {
    buffEvent = CONTROL_JSON;
    action = "CONTROL_JSON";

    // Cache indexOf result — avoid scanning the same string twice
    const int _codeIdx_ctrl = _payload.indexOf("\":\"");
    if (_payload.indexOf("20000") != -1)
    {
      code = _payload.substring(_codeIdx_ctrl + 3, _codeIdx_ctrl + 8);
      intern_EVENT.RESP = "SUCCESS";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }
    else
    {
      code = _payload.substring(_codeIdx_ctrl + 3, _payload.length() + 8);
      intern_EVENT.RESP = "FAIL";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }

    if (attr.ctrl_regis_json)
    {
      while (handleRegisJSON != NULL)
      {
        if (handleRegisJSON->Event == buffEvent)
        {
          break;
        }
        else
        {
          handleRegisJSON = (regisAPI *)handleRegisJSON->next;
        }
      }
      if (handleRegisJSON != NULL)
      {
        handleRegisJSON->ctrl_Json_callback(_payload);
      }
    }

    if (attr.ctrl_jsonOBJ)
    {
      while (handleRegisJSON_CTRL_OBJ != NULL)
      {
        if (handleRegisJSON_CTRL_OBJ->Event == buffEvent)
        {
          break;
        }
        else
        {
          handleRegisJSON_CTRL_OBJ = (regisAPI *)handleRegisJSON_CTRL_OBJ->next;
        }
      }
      if (handleRegisJSON_CTRL_OBJ != NULL)
      {
        // Serial.println(_payload);
        String buffDocs = deControl(_payload);
        // Serial.println(buffDocs);
        JsonObject Docs = deJson(buffDocs);
        handleRegisJSON_CTRL_OBJ->ctrl_obj_callback(Docs);
      }
    }
  }

  if (b_topic.indexOf("/config/resp/pta/?config=") != -1)
  {
    int indexfound2 = b_topic.indexOf("="); // b_topic is already String — no copy needed
    String keyOnTopic = b_topic.substring(indexfound2 + 1);
    key = keyOnTopic;
    buffEvent = CONFIG_PLAINTEXT;
    action = "CONFIG_PLAINTEXT";

    if ((_payload == "40300" || (_payload == "40400") && (_payload.length() == 5)))
    {
      code = _payload;
      intern_EVENT.RESP = "FAIL";
    }
    else
    {
      code = "20000";
      intern_EVENT.RESP = "SUCCESS";
    }

    if (attr.conf_regis_key)
    {
      while (handleRegisKEYConf != NULL)
      {
        if (handleRegisKEYConf->registerKey == key)
        {
          break;
        }
        else
        {
          handleRegisKEYConf = (regisAPI *)handleRegisKEYConf->next;
        }
      }
      if (handleRegisKEYConf != NULL)
      {
        if (handleRegisKEYConf->registerKey == key)
        {
          handleRegisKEYConf->conf_key_callback(_payload);
        }
      }
    }

    if (attr.conf_regis_pta)
    {
      while (handleRegisJSON != NULL)
      {
        if (handleRegisPTAConf->Event == buffEvent)
        {
          break;
        }
        else
        {
          handleRegisPTAConf = (regisAPI *)handleRegisPTAConf->next;
        }
      }
      if (handleRegisPTAConf != NULL)
      {
        handleRegisPTAConf->conf_pta_callback(key, _payload);
      }
    }
  }
  else if (b_topic.indexOf("/config/resp") != -1)
  {
    buffEvent = CONFIG_JSON;
    action = "CONFIG_JSON";

    // Cache indexOf result — avoid scanning the same string twice
    const int _codeIdx_conf = _payload.indexOf("\":\"");
    if (_payload.indexOf("20000") != -1)
    {
      code = _payload.substring(_codeIdx_conf + 3, _codeIdx_conf + 8);
      intern_EVENT.RESP = "SUCCESS";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }
    else
    {
      code = _payload.substring(_codeIdx_conf + 3, _payload.length() + 8);
      intern_EVENT.RESP = "FAIL";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }

    if (attr.conf_regis_json)
    {
      while (handleRegisJSONConf != NULL)
      {
        if (handleRegisJSONConf->Event == buffEvent)
        {
          break;
        }
        else
        {
          handleRegisJSONConf = (regisAPI *)handleRegisJSONConf->next;
        }
      }
      if (handleRegisJSONConf != NULL)
      {
        handleRegisJSONConf->conf_json_callback(_payload);
      }
    }

    if (attr.conf_jsonOBJ)
    {
      while (handleRegisJSON_CONF_OBJ != NULL)
      {
        if (handleRegisJSON_CONF_OBJ->Event == buffEvent)
        {
          break;
        }
        else
        {
          handleRegisJSON_CONF_OBJ = (regisAPI *)handleRegisJSON_CONF_OBJ->next;
        }
      }
      if (handleRegisJSON_CONF_OBJ != NULL)
      {
        // Serial.println(_payload);
        String buffDocs = deConfig(_payload);
        // Serial.println(buffDocs);
        JsonObject Docs = deJson(buffDocs);
        handleRegisJSON_CONF_OBJ->conf_obj_callback(Docs);
      }
    }
  }

  if (b_topic.indexOf("dateTime") != -1)
  {
    buffEvent = UNIXTIME;
    action = "UNIXTIME";
  }
  if (b_topic.indexOf("/report/resp/pta/?") != -1)
  {
    buffEvent = RESP_REPORT_PLAINTEXT;
    action = "RESP_REPORT_PLAINTEXT";

    int valid_found_only_msgId = b_topic.indexOf("?id="); // only id
    int valid_sensor = b_topic.indexOf("?sensor=");       // focus only sensor
    int valid_msgId = b_topic.indexOf("&id=");            // start with key sensor and id

    if (valid_found_only_msgId != -1) // found only msg id {?id}
    {
      int indexfound = valid_found_only_msgId;
      String msgIdOnTopic = b_topic.substring(indexfound + 4);
      _MsgId = msgIdOnTopic.toInt();
    }

    if (valid_msgId != -1) // found MsgId
    {
      int indexfoundKey = valid_sensor;
      int indexfoundId = valid_msgId;
      String keyOnTopic = b_topic.substring(indexfoundKey + 8, indexfoundId);
      String msgIdOnTopic = b_topic.substring(indexfoundId + 4);
      // Serial.println("_payload: "+_payload);
      // Serial.println("MsgIdontopic: "+msgIdOnTopic);
      key = keyOnTopic;
      _MsgId = msgIdOnTopic.toInt();
    }
    else if (valid_msgId == -1 && valid_sensor != -1) // not found MsgId , found only key sensor
    {
      String keyOnTopic = b_topic.substring(valid_sensor + 8);
      key = keyOnTopic;
      _MsgId = -1;
    }

    if (_payload.indexOf("20000") != -1)
    {
      code = _payload;
      intern_EVENT.RESP = "SUCCESS";
    }
    else
    {
      code = _payload;
      intern_EVENT.RESP = "FAIL";
    }
  }
  else if (b_topic.indexOf("/report/resp") != -1)
  {
    buffEvent = RESP_REPORT_JSON;
    action = "RESP_REPORT_JSON";

    JsonObject OBJ_report_resp = deJson(_payload);
    String _code = OBJ_report_resp["Code"];

    if (_code != "null")
    {
      code = _code;
    }

    if (_payload.indexOf("id") != -1)
    {
      String _msg_id = OBJ_report_resp["id"];
      if (_msg_id != "null")
      {
        _MsgId = _msg_id.toInt();
      }
    }
    else
    {
      _MsgId = -1;
    }

    if (_payload.indexOf("20000") != -1)
    {
      // code = _payload.substring(_payload.indexOf("\":\"") + 3, _payload.length() + 8);
      intern_EVENT.RESP = "SUCCESS";
    }
    else
    {
      // code = _payload.substring(_payload.indexOf("\":\"") + 3, _payload.length() + 8);
      intern_EVENT.RESP = "FAIL";
    }
  }

  if (b_topic.indexOf("/heartbeat/resp/pta") != -1)
  {
    buffEvent = RESP_HEARTBEAT_PLAINTEXT;
    action = "RESP_HEARTBEAT_PLAINTEXT";

    if (_payload.indexOf("20000") != -1)
    {
      code = _payload;
      intern_EVENT.RESP = "SUCCESS";
    }
    else
    {
      code = _payload;
      intern_EVENT.RESP = "FAIL";
    }
  }
  else if (b_topic.indexOf("/heartbeat/resp") != -1)
  {
    buffEvent = RESP_HEARTBEAT_JSON;
    action = "RESP_HEARTBEAT_JSON";

    if (_payload.indexOf("20000") != -1)
    {
      code = _payload.substring(_payload.indexOf("\":\"") + 3, _payload.length() - 2);
      intern_EVENT.RESP = "SUCCESS";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }
    else
    {
      code = _payload.substring(_payload.indexOf("\":\"") + 3, _payload.length() - 2);
      intern_EVENT.RESP = "FAIL";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }
  }
  if (b_topic.indexOf("/report/timestamp/resp") != -1)
  {
    buffEvent = RESP_REPORT_TIMESTAMP;
    action = "RESP_REPORT_TIMESTAMP";
    if (_payload.indexOf("20000") != -1)
    {
      code = _payload.substring(_payload.indexOf("\":\"") + 3, _payload.length() - 2);
      intern_EVENT.RESP = "SUCCESS";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }
    else
    {
      code = _payload.substring(_payload.indexOf("\":\"") + 3, _payload.length() - 2);
      intern_EVENT.RESP = "FAIL";
      // Serial.println("test CODE ->"+ String(code) +" RESP :"+ String(intern_EVENT.RESP));
    }
  }
  if (b_topic.indexOf("/server/destination/response") != -1)
  {
    buffEvent = GET_ENDPOINT;
    action = "GET_ENDPOINT";
    attr.ext_EndPoint = b_payload;
  }
  // Serial.println("#INSIDE ");
  // Serial.println(action);
  // Serial.println(_payload);
  // Serial.println("#INSIDE ");

  intern_EVENT.CODE = code.toInt();
  intern_EVENT.Topic = b_topic;
  intern_EVENT.Payload = _payload;
  intern_EVENT.PayloadLength = length;
  intern_EVENT.Action = action;
  intern_EVENT.Key = key;
  intern_EVENT.MsgId = _MsgId;
  attr.matchMsgId_cb = intern_EVENT.MsgId;
  checkRequestRetransmit();

  // if (ext_useAdvanceCallback) // routing to onMessage Callback when use setMessageListener(Callback)
  if (attr.useAdvanceCallback) // routing to onMessage Callback when use setMessageListener(Callback)
  {
    cb_internal(intern_EVENT, b_payload);
  }

  if (attr.resp_regis)
  {
    while (handleRESP != NULL)
    {
      if (handleRESP->Event == buffEvent)
      {
        break;
      }
      else
      {
        handleRESP = (regisAPI *)handleRESP->next;
      }
    }
    // Serial.println("# IN RESP :"+ String(handleRESP->Event));
    // Serial.println("# IN RESP2 :"+ String(buffEvent));
    if (handleRESP != NULL)
    {
      if (handleRESP->Event == buffEvent)
      {
        handleRESP->resp_h_callback(intern_EVENT);
      }
    }
  }
  // Serial.println("#DEBUG INSIDE :"+intern_EVENT.Action);
  // Serial.println("#DEBUG INSIDE :"+intern_EVENT.Payload);
}

MAGELLAN_MQTT_device_core::MAGELLAN_MQTT_device_core(Client &client)
{
  ensureJsonDocPointersReady();
  prev_time = 0;
  now_time = millis();
  HB_prev_time = 0;
  HB_now_time = millis();

  attr.clientNetInterface = useExternalClient;
  Client *newClient = &client;
  attr.ClientNET = *&newClient;
  attr.mqtt_client = new PubSubClient(*attr.ClientNET);
  this->client = *&attr.mqtt_client;
}

MAGELLAN_MQTT_device_core::MAGELLAN_MQTT_device_core()
{
  // Never used directly: only exists to satisfy MAGELLAN_MQTT_TEMP's implicit base-subobject construction.
  ensureJsonDocPointersReady();
  prev_time = 0;
  now_time = millis();
  HB_prev_time = 0;
  HB_now_time = millis();
}

String MAGELLAN_MQTT_device_core::getHostName()
{
  return this->host;
}

void MAGELLAN_MQTT_device_core::setMQTTBufferSize(uint16_t sizeBuffer)
{
  // Serial.println(F("# SetBufferSize: "));
  // Serial.println(sizeBuffer);
  this->_default_bufferSize = sizeBuffer;
}

void MAGELLAN_MQTT_device_core::setAuthMagellan(String _thingIden, String _thingSecret, String _imei)
{
  MG_LOG_I("#====== Setting Magellan Authentication =======");
  if (!(CheckString_isDigit(_thingIden) && CheckString_isDigit(_thingSecret)))
  {
    MG_LOG_E_S("# ERROR Can't connect to Magellan | Parameter invalid [thingIdentify]=> " + String(_thingIden) + "   [thingSecret]=> " + String(_thingSecret));
    MG_LOG_E("# Invalid Parameter!! Please check [thingIdentify] and [thingSecret]");
    while (true)
    {
      MG_LOG_I_S(".");
      delay(300);
      this->cnt_fail++;
      if (cnt_fail >= 100) // timeout Restart board 30 sec
      {
        ESP.restart();
      }
    }
  }
  this->thingIden = _thingIden;
  this->thingSecret = _thingSecret;
  this->imei = _imei;
  // Serial.println(F("#Set Auth Success"));
  // Serial.println(thingIden);
  // Serial.println(thingSecret);
  // Serial.println(imei);
}

int mapRSSIToDBm(int rssi)
{
  if (rssi == 99)
    return -113; // Not detectable

  return -113 + (rssi * 2); // Map RSSI to dBm
}

String getSignalStrengthCategory(int dBm)
{
  if (dBm <= -113)
    return "Very Poor";
  else if (dBm > -113 && dBm <= -85)
    return "Poor";
  else if (dBm > -85 && dBm <= -70)
    return "Fair";
  else if (dBm > -70 && dBm <= -55)
    return "Good";
  else if (dBm > -55)
    return "Excellent";

  return "Unknown";
}

// void getRadio()
// {
//   if (attr.clientNetInterface == useGSMClient)
//   {
//     MG_LOG_I("#========= Radio Quality information ==========");
//     int rssiNomalized = Network.getSignalStrength();
//     int rssiDbm = mapRSSIToDBm(rssiNomalized);
//     MG_LOG_I_S("Signal Strength: " + String(rssiNomalized));
//     MG_LOG_I_S("Signal Strength(Dbm): " + String(rssiDbm));
//     MG_LOG_I_S("Description: " + String(getSignalStrengthCategory(rssiDbm)));
//   }
// }

void MAGELLAN_MQTT_device_core::initialBoard()
{
  delay(1000);
  setAuthMagellan(GSM.getICCID(), GSM.getIMSI(), GSM.getIMEI());
  MG_LOG_D("#====== Initializing Board =======");
  MG_LOG_D_S("ICCID: " + String(thingIden));
  MG_LOG_D_S("IMSI: " + String(thingSecret));
  MG_LOG_D_S("IMEI: " + String(imei));
}

void MAGELLAN_MQTT_device_core::getBoardInfo()
{
  MG_LOG_D("#====== Board information =========");
  MG_LOG_D_S("ICCID: " + String(thingIden));
  MG_LOG_D_S("IMSI: " + String(thingSecret));
  MG_LOG_D_S("IMEI: " + String(imei));
  // getRadio();
}

String MAGELLAN_MQTT_device_core::getICCID()
{
  return this->thingIden;
}

String MAGELLAN_MQTT_device_core::getIMEI()
{
  return this->imei;
}

String MAGELLAN_MQTT_device_core::getIMSI()
{
  return this->thingSecret;
}

String MAGELLAN_MQTT_device_core::readToken()
{
  return this->token;
}

boolean MAGELLAN_MQTT_device_core::CheckString_isDigit(String valid_payload)
{
  for (byte i = 0; i < valid_payload.length(); i++)
  {
    if (!isDigit(valid_payload.charAt(i)))
      return false;
  }
  return true;
}

boolean MAGELLAN_MQTT_device_core::CheckString_isDouble(String valid_payload)
{
  char *input = (char *)valid_payload.c_str();
  char *end;
  strtod(input, &end);
  if (*input == '\0')
  {
    return false;
  }
  if (end == input || *end != '\0')
  {
    return false;
  }
  return true;
}

// void MAGELLAN_MQTT_device_core::setCallback(void(callback(char*, byte*, unsigned int)))
// {
//   this->client->setCallback(callback);
// }

void MAGELLAN_MQTT_device_core::setCallback_msgHandle()
{
  this->client->setCallback(msgCallback_internalHandler);
}

void MAGELLAN_MQTT_device_core::setMessageListener(void (*callback)(EVENTS, char *))
{
  attr.useAdvanceCallback = true;
  if (callback)
    cb_internal = callback;
}

void MAGELLAN_MQTT_device_core::dead_reconnect_handler()
{
  ulong diff_time_reconnect = millis() - this->fallback_dead_reconnect_time;
  MG_LOG_D_S("# Reconnect Elapsed Time (ms): " + String(String(diff_time_reconnect)));
  if (diff_time_reconnect <= this->threshold_dead_reconnect_time)
  {
    MG_LOG_D_S("# Threshold Dead Flush Reconnect Time (ms): " + String(String(this->threshold_dead_reconnect_time)));
    MG_LOG_D("# Detected Dead Flush Reconnect Elapsed Time <= Threshold Dead Flush Reconnect Time");
    this->cnt_dead_reconnect_time++;
    MG_LOG_D_S("# Dead Flush Reconnect Attempt: " + String(String(this->cnt_dead_reconnect_time)) + "/" + String(String(this->max_cnt_dead_reconnect_time)));

    if (this->cnt_dead_reconnect_time >= this->max_cnt_dead_reconnect_time)
    {
      MG_LOG_E("# Reach Max Dead Flush Reconnect Attempt, Restart Board");
      delay(3000);
      ESP.restart();
    }
  }
  else
  {
    MG_LOG_D("# Dead Flush Reconnect Reset Counter");
    MG_LOG_D_S("# Reconnect Elapsed Time > Threshold Dead Flush Reconnect Time (ms):  " + String(String(diff_time_reconnect)) + " -> " + String(this->threshold_dead_reconnect_time));
    this->cnt_dead_reconnect_time = 0;
  }
  this->fallback_dead_reconnect_time = millis();
}

void MAGELLAN_MQTT_device_core::reconnect()
{
  while (!isConnected())
  {
    MG_LOG_I("Device Disconected from Magellan...");
    this->dead_reconnect_handler();
    checkConnection();
    if (flagToken)
    {
      MG_LOG_I_S("# Remain Subscribes list\n");
      attr.triggerRemainSub = true;
      attr.triggerRemainOTA = true;
      if (!attr.flagAutoOTA)
      {
        sub_InfoOTA();
      }
    }
  }
}

boolean MAGELLAN_MQTT_device_core::acceptEndPoint(String payload)
{
  boolean acceptStatus = false;
  if (payload.length() >= 10)
  {
    const char *buff_payload = payload.c_str();
    JsonObject getCetric = deJson(buff_payload);

    String buf1 = getCetric["ServerDestinationInfo"];
    String buf2 = getCetric["OperationStatus"];
    // Serial.println("buf1 :"+ buf1);
    if (buf2.indexOf("20000") != -1)
    {
      JsonObject getCetric2 = deJson(buf1);
      String buff_ip = getCetric2["ServerIP"];
      String buff_domain = getCetric2["ServerDomain"];
      String buff_port = getCetric2["ServerPort"];
      centric.endPoint_DOMAIN = buff_domain;
      centric.endPoint_IP = buff_ip;
      centric.endPoint_PORT = buff_port;
      acceptStatus = true;
      MG_LOG_I("## NEW ZONE AVAILABLE #######");
      MG_LOG_I_S("# Centric IP >>: " + centric.endPoint_IP);
      MG_LOG_I_S("# Centric Domain >>: " + centric.endPoint_DOMAIN);
      MG_LOG_I_S("# Centric Port >>: " + String(centric.endPoint_PORT));
      cnt_attempt = 0;
    }
    else
    {
      MG_LOG_E("# Fail to Get Endpoint form centric");
      MG_LOG_E("# Please check the thing device is activated");
      MG_LOG_I_S("# response: " + String(payload));
    }
  }
  return acceptStatus;
}

void MAGELLAN_MQTT_device_core::acceptToken(String payload)
{
  if (payload.length() >= 36)
  {
    this->flagToken = true;
    this->token = payload;
    MG_LOG_D_S("# Thingtoken: " + token);
  }
}

void MAGELLAN_MQTT_device_core::acceptToken(EVENTS event)
{
  String _payload = event.Payload;
  if ((event.Topic == "api/v2/thing/" + String(thingIden) + "/" + String(thingSecret) + "/auth/resp/pta") && !flagToken)
  {
    if (_payload.length() >= 36)
    {
      this->flagToken = true;
      this->token = _payload;
      MG_LOG_D_S("# Token >> :" + token);
    }
  }
}

// String MAGELLAN_MQTT_device_core::byteToString(byte* payload, unsigned int length_payload)
// {
//   char buffer_payload[length_payload+1] = {0};
//   memcpy(buffer_payload, (char*)payload, length_payload);
//   return String(buffer_payload);
// }

String MAGELLAN_MQTT_device_core::byteToString(byte *payload, unsigned int length_payload)
{
  return b2str(payload, length_payload);
}

void MAGELLAN_MQTT_device_core::loop()
{
  this->client->loop();
  reconnect();
}

void MAGELLAN_MQTT_device_core::reconnectMagellan()
{
  while (!isConnected())
  {
    const int reconn_sec = 3;
    // Skip futile MQTT connect attempts (and the failure counter/restart cascade below)
    // while the underlying GPRS/cellular link is not up yet, e.g. right after power-on.
    if (this->func_check_network_ready != nullptr && !this->func_check_network_ready())
    {
      MG_LOG_E_S("# Network not ready yet, waiting before MQTT connect attempt (" + String(reconn_sec) + "s)");
      unsigned long waitStart = millis();
      while (millis() - waitStart < reconn_sec * 1000UL)
      {
        if (this->func_on_recon_continue != nullptr)
        {
          this->func_on_recon_continue();
        }
        delay(10);
      }
        if (this->func_on_recon != nullptr)
        {
          this->func_on_recon();
        }
        recon_attempt++;
#ifdef USE_ATTEMPT_LIMIT
        if (recon_attempt >= MAX_ATTEMPT_RECONNECT)
        {
          MG_LOG_I_S(" attempt to connect more than " + String(MAX_ATTEMPT_RECONNECT) + " Restart Board");
          ESP.restart();
        }
#endif
      // }
      // continue;
    }
    srand(time(NULL));
    int randnum = rand() % 10000;   // generate number concat in Client id
    int randnum_2 = rand() % 10000; // generate number concat in Client id
    if (attr.clientNetInterface == useExternalClient && this->prefixClient != "4G_TINY_B_")
    {
      this->prefixClient = "4G_B_EXT_Client_";
    }
    else if (attr.clientNetInterface == useGSMClient)
    {
      this->prefixClient = "4G_B_";
    }
    String client_idBuff = this->prefixClient + this->thingIden + "_" + String(randnum) + "_" + String(randnum_2) + "_" + String(lib_ver);
    client_id = client_idBuff;
    MG_LOG_I("Attempting MQTT connection ...");
    this->client->setServer(this->host.c_str(), this->port);
    this->client->setCallback(msgCallback_internalHandler);
    MG_LOG_I_S("Connecting Magellan on: " + String(this->host) + ", Port: " + String(this->port));
    // Open the socket ourselves with a short bounded timeout first; PubSubClient::connect()
    // skips its own (75s default) connect() call once the underlying client reports connected,
    // so this keeps a stuck/bad link from blocking each retry for a very long time.
    boolean tcpPreConnectFailed = this->func_tcp_pre_connect != nullptr && !this->func_tcp_pre_connect(this->host, this->port, 15);
    if (!tcpPreConnectFailed && this->client->connect(client_idBuff.c_str(), this->thingIden.c_str(), this->thingSecret.c_str()))
    {
      client_id = client_idBuff;
      MG_LOG_I_S("Client id: " + client_idBuff + " is connected");
      recon_attempt = 0;
    }
    else
    {
      if (tcpPreConnectFailed)
      {
        MG_LOG_E_S("failed, TCP socket open timed out, try again in " + String(reconn_sec) + " seconds");
      }
      else
      {
        MG_LOG_E_S("failed, reconnect =" + String(this->client->state()) + " try again in " + String(reconn_sec) + " seconds");
      }
      if (this->func_on_recon != nullptr)
      {
        this->func_on_recon();
      }
      if (!flagToken)
      {
        MG_LOG_E("# Please check the thing device is activated ");
      }
      unsigned long startTime = millis();
      while (millis() - startTime < reconn_sec * 1000UL)
      {
        
        if (this->func_on_recon_continue != nullptr)
        {
          this->func_on_recon_continue();
        }
        delay(10);
      }
      // delay(reconn_sec * 1000);
      recon_attempt++;
      MG_LOG_D_S("# attempt connect on :" + String(String(recon_attempt) + " times"));
#ifdef USE_ATTEMPT_LIMIT
      if (recon_attempt >= MAX_ATTEMPT_RECONNECT)
      {
        MG_LOG_I_S(" attempt to connect more than " + String(MAX_ATTEMPT_RECONNECT) + " Restart Board");
        ESP.restart();
      }
#else
      MG_LOG_I_S(F("USE_MAX_ATTEMPT_RECONNECT is disabled(0), try to connect until success no restart board"));
#endif
    }
  }
  thingRegister();
}

void MAGELLAN_MQTT_device_core::checkConnection()
{
  if (!isConnected())
  {
    reconnectMagellan();
  }
}

void MAGELLAN_MQTT_device_core::magellanCentric(const char *_host, int _port)
{
  if (!isConnected())
  {
    while (!isConnected())
    {
      const int reconn_sec = 3;
      srand(time(NULL));
      int randnum = rand() % 10000;   // generate number concat in Client id
      int randnum_2 = rand() % 10000; // generate number concat in Client id
      String client_idBuff = "Centric_" + this->thingIden + "_" + String(randnum) + "_" + String(randnum_2) + "_" + String(lib_ver);
      MG_LOG_I("#Attempting connection ...");
      this->client->setServer(_host, _port);
      this->client->setCallback(msgCallback_internalHandler);
      MG_LOG_I_S("Connecting Centric Magellan on: " + String(_host) + ", Port: " + String(_port));
      String thisIdenCentric = "Centric." + thingIden;
      this->client->setBufferSize(this->_default_bufferSize);
      if (this->client->connect(client_idBuff.c_str(), thisIdenCentric.c_str(), this->thingSecret.c_str()))
      {
        MG_LOG_I_S("Client id : " + client_idBuff + " is connected");
        recon_attempt = 0;
      }
      else
      {
        MG_LOG_E_S("failed, reconnect =" + String(this->client->state()) + " try again in " + String(reconn_sec) + " seconds");
        MG_LOG_D_S("Count Attemp Reconnect: ");
        recon_attempt++;
        MG_LOG_I_S(recon_attempt);

        delay(reconn_sec * 1000);
        // #ifdef USE_ATTEMPT_LIMIT
        if (recon_attempt >= MAX_ATTEMPT_RECONNECT)
        {
          MG_LOG_I_S(" attempt to connect more than: " + String(MAX_ATTEMPT_RECONNECT) + " Restart Board");
          ESP.restart();
        }
        // #endif
      }
    }
    getEndPoint();
  }
}

void MAGELLAN_MQTT_device_core::getEndPoint()
{
  MG_LOG_I("# REQUEST ENDPOINT");
  while (!flagRegisterEndPoint)
  {
    String topic = "api/v2/things/" + this->thingIden + "/" + this->thingSecret + "/server/destination/response";
    this->flagRegisterEndPoint = this->client->subscribe(topic.c_str());
    MG_LOG_I_S("# Register destination server: " + String(flagRegisterEndPoint ? "Success" : "Fail"));
  }
  while (!flagGetEndPoint)
  {
    this->client->loop();
    // Serial.println("Pub");
    this->requestEndpoint();
    if (attr.ext_EndPoint.length() >= 10)
    {
      // Serial.println(attr.ext_EndPoint);
      flagGetEndPoint = acceptEndPoint(attr.ext_EndPoint);
    }
    // delay(5000);
    for (int i = 0; i < 5000; i++)
    {
      this->client->loop();
      delay(1);
    }
  }
  if (flagGetEndPoint)
  {
    MG_LOG_I("# Disconnect from Centric");
    this->client->disconnect();
    MG_LOG_I("# Connect to new zone");
    srand(time(NULL));
    int randnum = rand() % 10000;   // generate number concat in Client id
    int randnum_2 = rand() % 10000; // generate number concat in Client id
    if (attr.clientNetInterface == useExternalClient && this->prefixClient != "4G_TINY_B_")
    {
      this->prefixClient = "4G_B_EXT_Client_";
    }
    else if (attr.clientNetInterface == useGSMClient)
    {
      this->prefixClient = "4G_B_";
    }
    String client_idBuff = this->prefixClient + this->thingIden + "_" + String(randnum) + "_" + String(randnum_2) + "_" + String(lib_ver);
    client_id = client_idBuff;
    // this->beginCustom(client_idBuff, true, centric.endPoint_IP, (centric.endPoint_PORT).toInt(), this->_default_bufferSize);
    this->beginCustom(client_idBuff, true, centric.endPoint_DOMAIN, (centric.endPoint_PORT).toInt(), this->_default_bufferSize);
  }
}

bool MAGELLAN_MQTT_device_core::isConnected()
{
  return this->client->connected();
}

void MAGELLAN_MQTT_device_core::beginCustom(String _client_id, boolean builtInSensor, String _host, int _port, uint16_t bufferSize)
{
  MG_LOG_I_S("=================== Begin MAGELLAN Library [AIS 4G Board] " + String(lib_version) + " ===================");
  if (attr.clientNetInterface == useGSMClient)
  {
    MG_LOG_I("# AIS 4G Board");
    while (!GSM.begin())
    {
      MG_LOG_E("GSM setup fail");
      delay(2000);
    }
    if ((this->thingIden == NULL) && (this->thingSecret == NULL))
    {
      initialBoard();
    }
  }
  else if (attr.clientNetInterface == useExternalClient && this->prefixClient == "4G_TINY_B_")
  {
    MG_LOG_I("# AIS 4G Board [TinyGSM]");
  }
  else
  {
    MG_LOG_I("# External Client");
  }

  if (builtInSensor && attr.clientNetInterface == useGSMClient)
  {
    MG_LOG_I("# Using Builtin SENSOR");
    attr.useBuiltInSensor = builtInSensor;
    mySensor.begin();
  }
  delay(3000);
  // getRadio();
  this->host = _host;
  this->port = _port;
  this->client_id = _client_id;
  if (bufferSize > _default_OverBufferSize)
  {
    MG_LOG_I_S("# Buffer size from you set over than 8192 set buffer to: " + String(_default_OverBufferSize));
    this->setBufferSize(_default_OverBufferSize);
    attr.calculate_chunkSize = _default_OverBufferSize / 2;
  }
  else
  {
    this->setBufferSize(bufferSize);
    attr.calculate_chunkSize = bufferSize / 2;
  }
  checkConnection();
}

void MAGELLAN_MQTT_device_core::beginCentric()
{
  MG_LOG_I_S("=================== Begin MAGELLAN Library [AIS 4G Board] " + String(lib_version) + " ===================");
  if (attr.clientNetInterface == useGSMClient)
  {
    MG_LOG_I("# AIS 4G Board");
    while (!GSM.begin())
    {
      MG_LOG_E("GSM setup fail");
      delay(2000);
    }
    if ((this->thingIden == NULL) && (this->thingSecret == NULL))
    {
      initialBoard();
      // getRadio();
    }
  }
  else if (attr.clientNetInterface == useExternalClient)
  {
    MG_LOG_I("# External Client");
  }
  MG_LOG_I("# Connect to Centric");
  delay(5000);
  //  getRadio();
  this->host = hostCentric;
  this->port = mgPort;
  this->client_id = this->getICCID(); // auto_assigned Client ID with ThingIdent
  setBufferSize(_default_bufferSize);
  magellanCentric();
}

void MAGELLAN_MQTT_device_core::begin(boolean builtInSensor)
{
  MG_LOG_I_S("=================== Begin MAGELLAN Library [AIS 4G Board] " + String(lib_version) + " ===================");
  if (attr.clientNetInterface == useGSMClient)
  {
    MG_LOG_I("# AIS 4G Board");
    while (!GSM.begin())
    {
      MG_LOG_E("GSM setup fail");
      delay(2000);
    }
    if ((this->thingIden == NULL) && (this->thingSecret == NULL))
    {
      initialBoard();
      // getRadio();
    }
  }
  else if (attr.clientNetInterface == useExternalClient)
  {
    MG_LOG_I("# External Client");
  }
  if (builtInSensor)
  {
    MG_LOG_I("# Using Builtin SENSOR");
    attr.useBuiltInSensor = builtInSensor;
    mySensor.begin();
  }
  // getRadio();
  // delay(5000);
  for (int i = 0; i < 5000; i++)
  {
    this->client->loop();
    delay(1);
  }
  String _host = _host_production;

  this->host = _host;
  this->port = mgPort;
  this->client_id = getICCID(); // auto_assigned Client ID with ICCID
  setBufferSize(_default_bufferSize);
  checkConnection();
}

void MAGELLAN_MQTT_device_core::begin(String _thingIden, String _thingSencret, String _imei, uint16_t bufferSize, boolean builtInSensor)
{
  setAuthMagellan(_thingIden, _thingSencret, _imei);
  MG_LOG_I_S("ThingIdentify: " + String(_thingIden));
  MG_LOG_D_S("ThingSecret: " + String(_thingSencret));
  MG_LOG_D_S("IMEI: " + String(_imei));
  begin(_thingIden, builtInSensor, bufferSize);
}

void MAGELLAN_MQTT_device_core::begin(String _client_id, boolean builtInSensor, uint16_t bufferSize)
{
  MG_LOG_I_S("=================== Begin MAGELLAN Library [AIS 4G Board] " + String(lib_version) + " ===================");
  if (attr.clientNetInterface == useGSMClient)
  {
    MG_LOG_I("# AIS 4G Board");
    while (!GSM.begin())
    {
      MG_LOG_E("GSM setup fail");
      delay(2000);
    }
    if ((this->thingIden == NULL) && (this->thingSecret == NULL))
    {
      initialBoard();
      // getRadio();
    }
  }

  else if (attr.clientNetInterface == useExternalClient)
  {
    MG_LOG_I("# External Client");
  }

  if (builtInSensor)
  {
    MG_LOG_I("# Using Builtin SENSOR");
    attr.useBuiltInSensor = builtInSensor;
    mySensor.begin();
  }
  // getRadio();
  // delay(5000);
  for (int i = 0; i < 5000; i++)
  {
    this->client->loop();
    delay(1);
  }

  this->host = _host_production;
  this->port = mgPort;
  this->client_id = _client_id;
  if (bufferSize > _default_OverBufferSize)
  {
    MG_LOG_E_S("# Buffer size from you set over than 8192 set buffer to: " + String(_default_OverBufferSize));
    this->setBufferSize(_default_OverBufferSize);
    attr.calculate_chunkSize = _default_bufferSize / 2;
  }
  else
  {
    this->setBufferSize(bufferSize);
    attr.calculate_chunkSize = bufferSize / 2;
  }
  checkConnection();
}

boolean MAGELLAN_MQTT_device_core::registerToken()
{
  String topic = "api/v2/thing/" + this->thingIden + "/" + this->thingSecret + "/auth/resp/pta";
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  MG_LOG_I("# Register Token to magellan");
  MG_LOG_I_S("# Register Token Status: " + _debug);
  return Sub_status;
}

boolean MAGELLAN_MQTT_device_core::report(String payload)
{

  String topic = "api/v2/thing/" + token + "/report/persist";
  boolean Pub_status = this->client->publish(topic.c_str(), payload.c_str());
  _debug = (Pub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Report JSON: " + _debug);
  MG_LOG_D_S("# [Sensors]: " + payload);
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::report(String key, String value)
{
  String topic = "api/v2/thing/" + token + "/report/persist/pta/?sensor=" + key;
  boolean Pub_status = client->publish(topic.c_str(), value.c_str());
  _debug = (Pub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Report Plaintext: " + _debug);
  MG_LOG_I_S("# [key]: " + key);
  MG_LOG_I_S("# [value]: " + value);
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::reportSensor()
{
  String bufferPlayload = buildSensorJSON(*attr.docSensor);
  boolean Pub_status = false;
  if (bufferPlayload.indexOf("null") == -1)
  {
    Pub_status = report(bufferPlayload);
    clearSensorBuffer(*attr.docSensor);
  }
  else
  {
    MG_LOG_I("# Can't reportSensor Because Not set function \"void addSensor(key,value)\" before reportSensor");
  }
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::ACKControl(String key, String value)
{
  String topic = "api/v2/thing/" + token + "/report/persist/pta/?sensor=" + key;
  boolean Pub_status = this->client->publish(topic.c_str(), value.c_str());
  _debug = (Pub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# ACKNOWNLEDGE Control Plaintext: " + _debug);
  MG_LOG_I_S("# [key]: " + key);
  MG_LOG_I_S("# [value]: " + value);
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::ACKControl(String payload)
{
  String topic = "api/v2/thing/" + token + "/report/persist";
  boolean Pub_status = this->client->publish(topic.c_str(), payload.c_str());
  _debug = (Pub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# ACKNOWNLEDGE Control JSON: " + _debug);
  MG_LOG_D_S("# [Sensors]: " + payload);
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::reportTimestamp(String timestamp, String JSONpayload)
{
  boolean Pub_status = false;
  String topic_ = "api/v2/thing/" + token + "/report/timestamp/persist";
  if (timestamp != NULL && JSONpayload != NULL)
  {
    String payload_ = "[{\"UNIXTS\":" + timestamp + ",\"Sensor\":" + JSONpayload + "}]";
    Pub_status = this->client->publish(topic_.c_str(), payload_.c_str());
    _debug = (Pub_status == true) ? "Success" : "Failure";
    MG_LOG_I_S("# Report with timestamp: " + _debug);
    MG_LOG_D_S("# [Sensors]: " + payload_);
    return Pub_status;
  }
  else
  {
    MG_LOG_E("# Report with timestamp: Failure");
    MG_LOG_E("# Error Empty timestamp or Sensor payload");
  }
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::reportClientConfig(String payload)
{
  String topic = "api/v2/thing/" + token + "/config/persist";
  boolean Pub_status = this->client->publish(topic.c_str(), payload.c_str());
  _debug = (Pub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Save ClientConfig: " + _debug);
  MG_LOG_I_S("# [ClientConfigs]: " + payload);
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::requestEndpoint()
{
  boolean Pub_status = false;
  if (!flagGetEndPoint)
  {
    if (cnt_attempt >= limit_attempt)
    {
      MG_LOG_I_S("Device Attempt to request ENDPOINT more than " + String(limit_attempt) + " time. restart board");
      delay(1000);
      ESP.restart();
    }
    if (millis() - previouseMillis > 10000)
    {

      previouseMillis = millis();
      String topic = "api/v2/things/" + thingIden + "/" + thingSecret + "/server/destination/request";
      Pub_status = this->client->publish(topic.c_str(), " ");
      _debug = (Pub_status == true) ? "Success" : "Failure";
      MG_LOG_I_S("# Request Endpoint: " + _debug);
      if (cnt_attempt > 0)
      {
        MG_LOG_I_S(" Attempt >> " + String(cnt_attempt) + " time");
      }
      this->client->loop();
      cnt_attempt++;
    }
  }
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::requestToken()
{
  boolean Pub_status = false;
  if (!flagToken)
  {
    if (cnt_attempt >= limit_attempt)
    {
      MG_LOG_I_S("Device Attempt to request token more than " + String(limit_attempt) + " time. restart board");
      delay(1000);
      ESP.restart();
    }
    if (millis() - previouseMillis > 10000)
    {
      previouseMillis = millis();
      String topic = "api/v2/thing/" + thingIden + "/" + thingSecret + "/auth/req";
      Pub_status = this->client->publish(topic.c_str(), " ");
      _debug = (Pub_status == true) ? "Success" : "Failure";
      // Serial.println("topic :" + topic);
      MG_LOG_D_S("# Request Token: " + _debug);
      if (cnt_attempt > 0)
      {
        MG_LOG_I_S(" Attempt >> " + String(cnt_attempt - 1) + " time");
      }
      cnt_attempt++;
    }
  }
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::setBufferSize(uint16_t size)
{
  MG_LOG_I_S("# set BufferSize: " + String(size));
  return this->client->setBufferSize(size);
}

boolean MAGELLAN_MQTT_device_core::heartbeat()
{
  String topic = "api/v2/thing/" + token + "/heartbeat";
  boolean Pub_status = this->client->publish(topic.c_str(), " ");
  _debug = (Pub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Heartbeat Trigger: " + _debug);
  return Pub_status;
}

boolean firstHBdoing = true;
void MAGELLAN_MQTT_device_core::heartbeat(unsigned int triger_ms)
{
  HB_threshold_ms = triger_ms;
  HB_now_time = millis();
  unsigned long different_ms = HB_now_time - HB_prev_time;
  if (different_ms >= HB_threshold_ms || firstHBdoing)
  {
    firstHBdoing = false;
    heartbeat();
    HB_prev_time = HB_now_time;
  }
}

void MAGELLAN_MQTT_device_core::setManualToken(String _token)
{
  MG_LOG_I("# SET MANUAL TOKEN =====");
  MG_LOG_D_S("#Token: " + _token);
  if (_token.length() >= 36)
  {
    this->flagToken = true;
    this->token = _token;
    attr.ext_Token = this->token;
    acceptToken(token);
  }
}

boolean MAGELLAN_MQTT_device_core::reqControlJSON()
{

  String topic = "api/v2/thing/" + token + "/delta/req";
  boolean Pub_status = this->client->publish(topic.c_str(), " ");
  _debug = (Pub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Request Control [JSON]: " + _debug);
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::reqControl(String key)
{
  String topic = "api/v2/thing/" + token + "/delta/req/?sensor=" + key;
  boolean Pub_status = this->client->publish(topic.c_str(), " ");
  _debug = (Pub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Request Control Plaintext by [Key]: \"" + key + "\": " + _debug);
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::reqConfigJSON()
{
  String topic = "api/v2/thing/" + token + "/config/req";
  boolean Pub_status = this->client->publish(topic.c_str(), " ");
  _debug = (Pub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Request Config [JSON]: " + _debug);
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::reqConfig(String key)
{
  String topic = "api/v2/thing/" + token + "/config/req/?config=" + key; // fact C c
  boolean Pub_status = this->client->publish(topic.c_str(), " ");
  _debug = (Pub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Request Config Plaintext [Key]: \"" + key + "\": " + _debug);
  return Pub_status;
}

void MAGELLAN_MQTT_device_core::getControl(String key, ctrl_handleCallback ctrl_callback)
{
  attr.ctrl_regis_key = true;
  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONTROL_PLAINTEXT;
  newRegis->registerKey = key;
  newRegis->ctrl_key_callback = ctrl_callback;
  newRegis->next = NULL;

  if (_startRegis != NULL)
  {
    regisAPI *focusRegis = _startRegis;
    while (focusRegis->next != NULL)
    {
      focusRegis = (regisAPI *)focusRegis->next;
    }
    focusRegis->next = newRegis;
  }
  else
  {
    _startRegis = newRegis;
  }
}

void MAGELLAN_MQTT_device_core::getControl(ctrl_PTAhandleCallback ctrl_pta_callback)
{
  attr.ctrl_regis_pta = true;

  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONTROL_PLAINTEXT;
  newRegis->ctrl_pta_callback = ctrl_pta_callback;
  newRegis->next = NULL;

  if (_startRegisPTA != NULL)
  {
    regisAPI *focusRegis = _startRegisPTA;
    while (focusRegis->next != NULL)
    {
      focusRegis = (regisAPI *)focusRegis->next;
    }
    focusRegis->next = newRegis;
  }
  else
  {
    _startRegisPTA = newRegis;
  }
}

void MAGELLAN_MQTT_device_core::getControlJSON(ctrl_Json_handleCallback ctrl_json_callback)
{
  attr.ctrl_regis_json = true;

  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONTROL_JSON;
  newRegis->ctrl_Json_callback = ctrl_json_callback;
  newRegis->next = NULL;

  if (_startRegisJSON != NULL)
  {
    regisAPI *focusRegis = _startRegisJSON;
    while (focusRegis->next != NULL)
    {
      focusRegis = (regisAPI *)focusRegis->next;
    }
    focusRegis->next = newRegis;
  }
  else
  {
    _startRegisJSON = newRegis;
  }
}

void MAGELLAN_MQTT_device_core::getControlJSON(ctrl_JsonOBJ_handleCallback jsonOBJ_cb)
{
  attr.ctrl_jsonOBJ = true;
  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONTROL_JSON;
  newRegis->ctrl_obj_callback = jsonOBJ_cb;
  newRegis->next = NULL;

  if (_startOBJ_CTRL != NULL)
  {
    regisAPI *focusRegis = _startOBJ_CTRL;
    while (focusRegis->next != NULL)
    {
      focusRegis = (regisAPI *)focusRegis->next;
    }
    focusRegis->next = newRegis;
  }
  else
  {
    _startOBJ_CTRL = newRegis;
  }
}

void MAGELLAN_MQTT_device_core::getConfig(String key, conf_handleCallback _conf_callback)
{
  attr.conf_regis_key = true;

  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONFIG_PLAINTEXT;
  newRegis->registerKey = key;
  newRegis->conf_key_callback = _conf_callback;
  newRegis->next = NULL;

  if (_startRegisConf != NULL)
  {
    regisAPI *focusRegis = _startRegisConf;
    while (focusRegis->next != NULL)
    {
      focusRegis = (regisAPI *)focusRegis->next;
    }
    focusRegis->next = newRegis;
  }
  else
  {
    _startRegisConf = newRegis;
  }
}

void MAGELLAN_MQTT_device_core::getConfig(conf_PTAhandleCallback conf_pta_callback)
{
  attr.conf_regis_pta = true;
  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONFIG_PLAINTEXT;
  newRegis->conf_pta_callback = conf_pta_callback;
  newRegis->next = NULL;

  if (_startRegisPTAConf != NULL)
  {
    regisAPI *focusRegis = _startRegisPTAConf;
    while (focusRegis->next != NULL)
    {
      focusRegis = (regisAPI *)focusRegis->next;
    }
    focusRegis->next = newRegis;
  }
  else
  {
    _startRegisPTAConf = newRegis;
  }
}

void MAGELLAN_MQTT_device_core::getConfigJSON(conf_Json_handleCallback conf_json_callback)
{
  attr.conf_regis_json = true;
  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONFIG_JSON;
  newRegis->conf_json_callback = conf_json_callback;
  newRegis->next = NULL;

  if (_startRegisJSONConf != NULL)
  {
    regisAPI *focusRegis = _startRegisJSONConf;
    while (focusRegis->next != NULL)
    {
      focusRegis = (regisAPI *)focusRegis->next;
    }
    focusRegis->next = newRegis;
  }
  else
  {
    _startRegisJSONConf = newRegis;
  }
}

void MAGELLAN_MQTT_device_core::getConfigJSON(conf_JsonOBJ_handleCallback jsonOBJ_cb)
{
  attr.conf_jsonOBJ = true;
  regisAPI *newRegis = new regisAPI;
  newRegis->Event = CONFIG_JSON;
  newRegis->conf_obj_callback = jsonOBJ_cb;
  newRegis->next = NULL;

  if (_startOBJ_CONF != NULL)
  {
    regisAPI *focusRegis = _startOBJ_CONF;
    while (focusRegis->next != NULL)
    {
      focusRegis = (regisAPI *)focusRegis->next;
    }
    focusRegis->next = newRegis;
  }
  else
  {
    _startOBJ_CONF = newRegis;
  }
}

void MAGELLAN_MQTT_device_core::getRESP(unsigned int resp_event, resp_callback resp_cb)
{
  attr.resp_regis = true;
  regisAPI *newRegis = new regisAPI;
  newRegis->Event = resp_event;
  newRegis->resp_h_callback = resp_cb;
  newRegis->next = NULL;

  if (_startRESP != NULL)
  {
    regisAPI *focusRegis = _startRESP;
    while (focusRegis->next != NULL)
    {
      focusRegis = (regisAPI *)focusRegis->next;
    }
    focusRegis->next = newRegis;
  }
  else
  {
    _startRESP = newRegis;
  }
}

boolean MAGELLAN_MQTT_device_core::registerResponseReport(int format)
{
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/" + token + "/report/resp/pta/+";
    break;
  case 1:
    topic = "api/v2/thing/" + token + "/report/resp";
    break;
  default:
    MG_LOG_I("out of length resp args format support [\"0\" or PLAINTEXT] is Plaint text(default) and [\"1\" or JSON]");
    topic = "api/v2/thing/" + token + "/report/resp";
    break;
  }
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  String respType = (format == 0) ? "Plaintext" : "JSON";
  // Serial.println("# RegisterRESP Report: "+ _debug);
  MG_LOG_I_S("# Subscribe Response Report: " + _debug);
  MG_LOG_I_S("# Response type: " + respType);
  return Sub_status;
}

boolean MAGELLAN_MQTT_device_core::registerResponseReportTimestamp()
{
  String topic = "api/v2/thing/" + token + "/report/timestamp/resp";
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  // Serial.println("# RegisterRESP ReportTimestamp: "+ _debug);
  MG_LOG_I_S("# Subscribe Response ReportTimestamp: " + _debug);
  return Sub_status;
}

boolean MAGELLAN_MQTT_device_core::registerResponseHeartbeat(int format)
{
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/" + token + "/heartbeat/resp/pta";
    break;
  case 1:
    topic = "api/v2/thing/" + token + "/heartbeat/resp";
    break;
  default:
    MG_LOG_I("out of length resp args format support [\"0\" or PLAINTEXT] is Plaint text(default) and [\"1\" or JSON]");
    topic = "api/v2/thing/" + token + "/heartbeat/resp";
    break;
  }
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  String respType = (format == 0) ? "Plaintext" : "JSON";
  // Serial.println("# RegisterRESP Heartbeat: "+ _debug);
  MG_LOG_I_S("# Subscribe Response Heartbeat: " + _debug);
  MG_LOG_I_S("# Response type: " + respType);
  return Sub_status;
}

boolean MAGELLAN_MQTT_device_core::registerConfig(String key)
{
  String topic = "api/v2/thing/" + token + "/config/resp/pta/?config=" + key; // fact C c
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  // Serial.println("# Register Server Config [Key]: \""+key+"\" Register: "+ _debug);
  MG_LOG_I_S("# Subscribe ServerConfig [Key]: \"" + key + "\" Subscribe: " + _debug);
  return Sub_status;
}

boolean MAGELLAN_MQTT_device_core::registerConfig(int format)
{
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/" + token + "/config/resp/pta/+";
    break;
  case 1:
    topic = "api/v2/thing/" + token + "/config/resp";
    break;
  default:
    MG_LOG_I("out of length resp args format support [\"0\" or PLAINTEXT] is Plaint text(default) and [\"1\" or JSON]");
    topic = "api/v2/thing/" + token + "/config/resp";
    break;
  }
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  String respType = (format == 0) ? "Plaintext" : "JSON";
  // Serial.println("# Register Server Config: "+ _debug);
  MG_LOG_I_S("# Subscribe ServerConfig: " + _debug);
  MG_LOG_I_S("# Response type: " + respType);
  return Sub_status;
}

boolean MAGELLAN_MQTT_device_core::getTimestamp()
{
  String topic = "api/v2/server/dateTime/req";
  boolean Pub_status = this->client->publish(topic.c_str(), " ");
  _debug = (Pub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Get ServerTime Request: " + _debug);
  return Pub_status;
}

boolean MAGELLAN_MQTT_device_core::registerTimestamp(int format)
{
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/server/dateTime/resp/pta";
    break;
  case 1:
    topic = "api/v2/server/dateTime/resp";
    break;
  default:
    MG_LOG_I("out of length resp args format support [\"0\" or PLAINTEXT] is Plaint text(default) and [\"1\" or JSON]");
    topic = "api/v2/server/dateTime/resp";
    break;
  }
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  String respType = (format == 0) ? "Plaintext" : "JSON";
  MG_LOG_I("# Subscribe Timestamp magellan");
  // Serial.println(F("# RegisterTimestamp magellan"));
  MG_LOG_I_S("# Subscribe ServerTime: " + _debug);
  // Serial.println("# RegisterTimestamp: "+ _debug);
  MG_LOG_I_S("# Response type: " + respType);
  return Sub_status;
}

boolean MAGELLAN_MQTT_device_core::registerControl(int format)
{
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/" + token + "/delta/resp/pta/+";
    break;
  case 1:
    topic = "api/v2/thing/" + token + "/delta/resp";
    break;
  default:
    MG_LOG_I("out of length resp args format support [\"0\" or PLAINTEXT] is Plaint text(default) and [\"1\" or JSON]");
    topic = "api/v2/thing/" + token + "/delta/resp";
    break;
  }
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  String respType = (format == 0) ? "Plaintext" : "JSON";
  // Serial.println("# RegisterControl: "+ _debug);
  MG_LOG_I_S("# Subscribe Control: " + _debug);
  MG_LOG_I_S("# Response type: " + respType);
  return Sub_status;
}

boolean MAGELLAN_MQTT_device_core::registerControl(String key)
{
  String topic = "api/v2/thing/" + token + "/delta/resp/pta/?sensor=" + key; // fact S s
  boolean Sub_status = this->client->subscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  // Serial.println(topic);
  // Serial.println("# RegisterControl [Key]: \""+key+"\" Register: "+ _debug);
  MG_LOG_I_S("# Subscribe Control [Key]: \"" + key + "\" Subscribe: " + _debug);
  return Sub_status;
}

void MAGELLAN_MQTT_device_core::thingRegister()
{
  while (!flagRegisterToken)
  {
    this->flagRegisterToken = registerToken();
  }
  while (!flagToken)
  {
    loop();
    requestToken();
    if (attr.ext_Token.length() >= 30)
    {
      token = attr.ext_Token;
      // Serial.println("DEBUG");
      // Serial.println(token);
      acceptToken(token);
    }
  }
}

void MAGELLAN_MQTT_device_core::registerList(func_callback_registerList cb_regisList)
{
  if (attr.triggerRemainSub)
  {
    if (attr.inProcessOTA)
    {
      MG_LOG_I("# Subscribes List is terminated when Inprocess OTA");
    }
    else
    {
      MG_LOG_I("# Subscribes List");
      cb_regisList();
      if (!attr.flagAutoOTA)
      {
        sub_InfoOTA();
      }
    }
    attr.triggerRemainSub = false;
  }
}

boolean firstTimedoing = true;
void MAGELLAN_MQTT_device_core::interval_ms(unsigned long ms, func_callback_ms cb_ms)
{
  threshold_ms = ms;
  now_time = millis();
  unsigned long different_ms = now_time - prev_time;
  if (different_ms >= threshold_ms || firstTimedoing)
  {
    firstTimedoing = false;
    prev_time = millis();
    cb_ms();
  }
}

JsonObject MAGELLAN_MQTT_device_core::deserialJson(String jsonContent)
{
  JsonObject buffer;
  if (jsonContent != NULL && jsonContent != "clear")
  {
    DeserializationError error = deserializeJson(docJson, jsonContent);
    buffer = docJson.as<JsonObject>();
    if (error)
      MG_LOG_E("# Error to DeserializeJson Control");
  }
  return buffer;
}

String MAGELLAN_MQTT_device_core::deserialControlJSON(String jsonContent)
{
  String content = "40300";
  JsonObject buffdoc = deserialJson(jsonContent);
  String statusCode = buffdoc["Code"];
  String buffDelta;
  if (statusCode == "20000")
  {
    if (jsonContent.indexOf("Delta") != -1)
    {
      buffDelta = buffdoc["Delta"].as<String>();
      content = buffDelta;
    }
    else if (jsonContent.indexOf("Sensor") != -1)
    {
      buffDelta = buffdoc["Sensor"].as<String>();
      content = buffDelta;
    }
  }
  return content;
}

void MAGELLAN_MQTT_device_core::updateSensor(String key, String value, JsonDocument &ref_docs)
{
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::updateSensor(String key, const char *value, JsonDocument &ref_docs)
{
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::updateSensor(String key, int value, JsonDocument &ref_docs)
{
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::updateSensor(String key, float value, JsonDocument &ref_docs)
{
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::updateSensor(String key, boolean value, JsonDocument &ref_docs)
{
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::addSensor(String key, String value, JsonDocument &ref_docs)
{
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::addSensor(String key, const char *value, JsonDocument &ref_docs)
{
  // Serial.println("[Key]: "+key+" [Value]: "+value);
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::addSensor(String key, int value, JsonDocument &ref_docs)
{
  // Serial.println("[Key]: "+key+" [Value]: "+String(value));
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::addSensor(String key, float value, JsonDocument &ref_docs)
{
  // Serial.println("[Key]: "+key+" [Value]: "+String(value));
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::addSensor(String key, boolean value, JsonDocument &ref_docs)
{
  // Serial.println("[Key]: "+key+" [Value]: "+String(value));
  ref_docs[key] = value;
}

void MAGELLAN_MQTT_device_core::remove(String key, JsonDocument &ref_docs)
{
  MG_LOG_I_S("Remove [Key]: " + key);
  ref_docs.remove(key);
}

boolean MAGELLAN_MQTT_device_core::findKey(String key, JsonDocument &ref_docs)
{
#if ARDUINOJSON_VERSION_MAJOR >= 7
  // Code สำหรับ Version 7
  return ref_docs[key.c_str()].isNull() ? false : true;
#else
  return ref_docs.containsKey(key);
#endif
}

String MAGELLAN_MQTT_device_core::buildSensorJSON(JsonDocument &ref_docs)
{
  String bufferJsonStr;
// Serial.println("# [Build JSON Key is]: "+ String(ref_docs.size()) +" key");
#if !MAGELLAN_USE_ARDUINOJSON7
  size_t mmr_usage = ref_docs.memoryUsage();
  size_t max_size = ref_docs.memoryPool().capacity();

  size_t safety_size = max_size * (0.97);
  if (mmr_usage >= safety_size)
  {
    bufferJsonStr = "null";
    MG_LOG_I_S("# [Overload memory toJSONString] *Maximum Safety Memory size to use is: " + String(safety_size));
  }
  else
  {
    serializeJson(ref_docs, bufferJsonStr);
    MG_LOG_I_S("# [to JSON String Key is]: " + String(ref_docs.size()) + " key");
  }

  MG_LOG_D_S("# MemoryUsage: " + String(mmr_usage) + "/" + String(safety_size) + " from(" + String(max_size) + ")");
#else
  // v7: JsonDocument is dynamic; measureJson() gives the actual serialized byte count.
  serializeJson(ref_docs, bufferJsonStr);
  MG_LOG_I_S("# [to JSON[V7] String Key is]: " + String(ref_docs.size()) + " key");
#endif
  return bufferJsonStr;
}

void MAGELLAN_MQTT_device_core::adjustBufferSensor(size_t sizeJSONbuffer)
{
#if !MAGELLAN_USE_ARDUINOJSON7
  if (attr.docSensor != NULL)
  {
    delete attr.docSensor;
    attr.docSensor = NULL;
  }
  attr.docSensor = new DynamicJsonDocument(sizeJSONbuffer);
#else
  (void)sizeJSONbuffer;
  if (attr.docSensor == NULL)
  {
    attr.docSensor = new JsonDocument();
  }
  else
  {
    attr.docSensor->clear();
  }
#endif
  // attr.docSensor = new DynamicJsonDocument(sizeJSONbuffer);
}

int MAGELLAN_MQTT_device_core::readBufferSensor(JsonDocument &ref_docs)
{
#if !MAGELLAN_USE_ARDUINOJSON7
  return ref_docs.memoryPool().capacity();
#else
  return 8192;
#endif
}

void MAGELLAN_MQTT_device_core::clearSensorBuffer(JsonDocument &ref_docs)
{
  MG_LOG_I("# [Clear JSON buffer]");
  ref_docs.clear();
}

boolean MAGELLAN_MQTT_device_core::registerInfoOTA()
{
  return sub_InfoOTA();
}

boolean MAGELLAN_MQTT_device_core::unregisterInfoOTA()
{
  return unsub_InfoOTA();
}

boolean MAGELLAN_MQTT_device_core::requestFW_Info()
{
  return pub_Info();
}

boolean MAGELLAN_MQTT_device_core::registerDownloadOTA()
{
  return sub_DownloadOTA();
}

boolean MAGELLAN_MQTT_device_core::unregisterDownloadOTA()
{
  return unsub_DownloadOTA();
}

boolean MAGELLAN_MQTT_device_core::requestFW_Download(unsigned int fw_chunk, size_t chunk_size)
{
  return pub_Download(fw_chunk, chunk_size);
}

boolean MAGELLAN_MQTT_device_core::updateProgressOTA(String OTA_state, String description)
{
  return pub_UpdateProgress(OTA_state.c_str(), description.c_str());
}

void MAGELLAN_MQTT_device_core::activeOTA(size_t chunk_size, boolean useChecksum)
{
  MG_LOG_I("# Activated OTA");
  Update.begin(UPDATE_SIZE_UNKNOWN);
  attr.using_Checksum = useChecksum;
  String isC_sum = (attr.using_Checksum == true) ? "ENABLE" : "DISABLE";
  MG_LOG_I_S(String(isC_sum) + " Checksum FirmwareOTA");
  // Serial.println(*attr.chunk_size);
  if (chunk_size > 4096)
  {
    MG_LOG_E_S("# [Warning] Chunk Size Maximun is 4096 (use Default \"" + String(attr.default_chunk_size) + "\")");
    setChunkSize(attr.default_chunk_size);
  }
  else
  {
    setChunkSize(chunk_size);
  }
  configOTAFile.beginFileSystem(true);
  if (!configOTAFile.checkFileOTA())
  {
    configOTAFile.createConfigFileOTA();
  }

  if (!configOTAFile.checkLastedOTA())
  {
    configOTAFile.createLastedOTA();
    if (configOTAFile.checkFileOTA())
    {
      String fw_infoInFIleSys = configOTAFile.readLastedOTA();
    }
  }
  else
  {
    String fw_infoInFIleSys;
    JsonObject fw_last = configOTAFile.readObjectLastedOTA();
    int bufferFW_size = fw_last["sizefirmware"];

    fw_last.remove("namefirmware");
    fw_last.remove("sizefirmware");
    fw_last.remove("checksumAlgorithm");
    String bufferFW_v = fw_last["versionfirmware"];
    fw_last["firmwareVersion"] = bufferFW_v;
    fw_last.remove("versionfirmware");
    serializeJson(fw_last, fw_infoInFIleSys);
    if (fw_infoInFIleSys.indexOf("null") == -1)
    {
      this->reportClientConfig(fw_infoInFIleSys);
    }
    else if ((bufferFW_v.length() > 4 || bufferFW_v.indexOf("null") == -1) && (fw_infoInFIleSys.indexOf("null") != -1)) // handle if fw version !null but some key value found null is still pub client config
    {
      this->reportClientConfig(fw_infoInFIleSys);
    }
    else if ((bufferFW_v.indexOf("null") != -1) && (bufferFW_size > 0))
    {
      this->reportClientConfig(fw_infoInFIleSys);
    }
  }
}
void MAGELLAN_MQTT_device_core::setChecksum(String md5Checksum)
{
  MG_LOG_I_S("# Set Checksum md5: " + md5Checksum + " Status:" + (Update.setMD5(md5Checksum.c_str()) == true ? " Success" : " Fail"));
}

void MAGELLAN_MQTT_device_core::setChunkSize(size_t chunkSize)
{
  attr.chunk_size = chunkSize;
  MG_LOG_D_S("# Set Chunk size: " + String(attr.chunk_size));
}

void checkTimeoutReq_fw_download()
{
  if (attr.checkTimeout_request_download_fw)
  {
    unsigned long differentTime = millis() - attr.prv_cb_timeout_millis;
    if (differentTime > 60000 && !attemp_download_1)
    {
      MG_LOG_I("#Attemp resume download 1 after checktimeout 1 minute");
      pub_Download(attr.fw_count_chunk, attr.chunk_size);
      attemp_download_1 = true;
    }
    if (differentTime > 120000 && !attemp_download_2)
    {
      MG_LOG_I("#Attemp resume download 2 after checktimeout 2 minute");
      pub_Download(attr.fw_count_chunk, attr.chunk_size);
      attemp_download_2 = true;
    }
    if (differentTime > attr.timeout_req_download_fw)
    {
      // pub_UpdateProgress("FAILED","{\"errordescription\":\"Timeout from request firmware download (version. "+ MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion+")\"}");
      char _to_pb[160];
      snprintf(_to_pb, sizeof(_to_pb), "{\"errordescription\":\"Downloading firmware %s is timeout on chunk (%u/%u)\"}",
               MAGELLAN_MQTT_device_core::OTA_info.firmwareVersion.c_str(), attr.current_chunk, attr.totalChunk);
      pub_UpdateProgress("FAILED", _to_pb);
      configOTAFile.saveSuccessOrFail("fail");

      MG_LOG_E_S("#device must restart timeout from request firmware dowload " + String(attr.timeout_req_download_fw / 60000) + " minute");
      // delay(5000);
      for (int i = 0; i < 5000; i++)
      {
        attr.mqtt_client->loop();
        delay(1);
      }
      ESP.restart();
    }
    // Serial.println("Counting Timeout: "+String(diferentTime/1000));
  }
  else
  {
    attr.prv_cb_timeout_millis = millis();
  }
}

unsigned long prv_mills_usingCheckOTA = 0;
void checkTimeoutCheckUpdate()
{
  if (attr.usingCheckUpdate)
  {
    unsigned long differentTime = millis() - prv_mills_usingCheckOTA;
    if (differentTime > 15000)
    {
      attr.usingCheckUpdate = false;
    }
  }
  else if (!attr.usingCheckUpdate)
  {
    prv_mills_usingCheckOTA = millis();
  }
}

boolean remind_unsub_when_inProcessOTA = false;
unsigned long prv_mills_pubinfo = 0;
void MAGELLAN_MQTT_device_core::handleOTA(boolean OTA_after_getInfo)
{
  checkTimeoutReq_fw_download();
  checkTimeoutCheckUpdate();
  if (attr.triggerRemainOTA)
  {

    MG_LOG_I("# Active handleOTA");
    registerInfoOTA();
    if (attr.flagAutoOTA)
      registerDownloadOTA();

    if (attr.inProcessOTA) // if get fw in hook fw will auto count and request dowload fw
    {
      attr.flag_remain_ota = true;
      attr.remain_ota_fw_info_match = false;

      if (!attr.remain_ota_fw_info_match)
      {
        if (millis() - prv_mills_pubinfo > 5000)
        {
          pub_Info();
          prv_mills_pubinfo = millis();
        }
      }
      else if (attr.remain_ota_fw_info_match) // remain get firmware
      {
        pub_Download(attr.fw_count_chunk, attr.chunk_size);
      }
    }
    if (attr.startReqDownloadOTA && !attr.inProcessOTA) // if get part 0 fail but start ota still work when reconnect
    {
      pub_Download(attr.fw_count_chunk, attr.chunk_size);
    }
    attr.triggerRemainOTA = false;
  }
  if ((attr.fw_total_size > 0) && !(attr.remind_Event_GET_FW_infoOTA))
  {
    // Serial.println(F("======================="));
    // Serial.println(F("# Firmware OTA Information Available #"));
    // Serial.println("  ->Firmware Name: "+MAGELLAN_MQTT_device_core::OTA_info.fw_name);
    // Serial.println("  ->Firmware total size: "+String(MAGELLAN_MQTT_device_core::OTA_info.fw_totalSize));
    // Serial.println("  ->Firmware version: "+MAGELLAN_MQTT_device_core::OTA_info.fw_version);
    // Serial.println("  ->Firmware checksum Algorithm: "+MAGELLAN_MQTT_device_core::OTA_info.algorithm_check_sum);
    // Serial.println("  ->Firmware checksum: "+MAGELLAN_MQTT_device_core::OTA_info.check_sum);
    // Serial.println(F("======================="));
    if (attr.using_Checksum && !attr.isFirmwareUptodate)
    {
      if (MAGELLAN_MQTT_device_core::OTA_info.checksumAlgorithm == "md5" || MAGELLAN_MQTT_device_core::OTA_info.checksumAlgorithm == "MD5")
        setChecksum(MAGELLAN_MQTT_device_core::OTA_info.checksum);
      else
      {
        MG_LOG_E("#[Warning] Can't set checksum because algorithm checksum is not \"md5\"");
        MG_LOG_E("#[Warning] But OTA Process still working without checksum ");
        attr.using_Checksum = false;
      }
    }
    if (OTA_after_getInfo && !attr.isFirmwareUptodate) // auto request OTA after get fw information
    {
      pub_Download(0, attr.chunk_size);
    }
    attr.remind_Event_GET_FW_infoOTA = true;
  }
  if (attr.inProcessOTA && !remind_unsub_when_inProcessOTA)
  {
    MG_LOG_I("# Inprocess OTA terminate other incoming message");
    MG_LOG_I("# Unsubscribe unuse function");
    if (attr.ctrl_regis_key || attr.ctrl_regis_pta)
    {
      unregisterControl(PLAINTEXT);
    }
    if (attr.ctrl_jsonOBJ || attr.ctrl_regis_json)
    {
      unregisterControl(JSON);
    }
    if (attr.conf_regis_key || attr.conf_regis_pta)
    {
      unregisterConfig(PLAINTEXT);
    }
    if (attr.conf_jsonOBJ || attr.conf_regis_json)
    {
      unregisterConfig(JSON);
    }
    if (attr.resp_regis)
    {
      unregisterResponseHeartbeat(PLAINTEXT);
      unregisterResponseHeartbeat(JSON);
      unregisterResponseReport(PLAINTEXT);
      unregisterResponseReport(JSON);
      unregisterResponseReportTimestamp();
      unregisterTimestamp(PLAINTEXT);
      unregisterTimestamp(JSON);
    }
    remind_unsub_when_inProcessOTA = true;
  }
}
////////////////// Unsub ///////////
boolean MAGELLAN_MQTT_device_core::unregisterControl(int format)
{
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/" + token + "/delta/resp/pta/+";
    break;
  case 1:
    topic = "api/v2/thing/" + token + "/delta/resp";
    break;
  default:
    topic = "api/v2/thing/" + token + "/delta/resp";
    break;
  }
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  String respType = (format == 0) ? "Plaintext" : "JSON";
  // Serial.println("# RegisterControl: "+ _debug);
  MG_LOG_I_S("# Unsubscribe Control: " + _debug);
  MG_LOG_I_S("# Response type: " + respType);
  return Sub_status;
} //
boolean MAGELLAN_MQTT_device_core::unregisterControl(String key)
{
  String topic = "api/v2/thing/" + token + "/delta/resp/pta/?sensor=" + key; // fact S s
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S(topic);
  MG_LOG_I_S("# Unsubscribe Control [Key]: \"" + key + "\" Unsubscribe: " + _debug);
  return Sub_status;
} //
boolean MAGELLAN_MQTT_device_core::unregisterConfig(int format)
{
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/" + token + "/config/resp/pta/+";
    break;
  case 1:
    topic = "api/v2/thing/" + token + "/config/resp";
    break;
  default:
    topic = "api/v2/thing/" + token + "/config/resp";
    break;
  }
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  String respType = (format == 0) ? "Plaintext" : "JSON";
  MG_LOG_I_S("# Unsubscribe ServerConfig: " + _debug);
  MG_LOG_I_S("# Response type: " + respType);
  return Sub_status;
} //
boolean MAGELLAN_MQTT_device_core::unregisterConfig(String key)
{
  String topic = "api/v2/thing/" + token + "/config/resp/pta/?config=" + key; // fact C c
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Unsubscribe ServerConfig [Key]: \"" + key + "\" Unsubscribe: " + _debug);
  return Sub_status;
} //
boolean MAGELLAN_MQTT_device_core::unregisterTimestamp(int format)
{
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/server/dateTime/resp/pta";
    break;
  case 1:
    topic = "api/v2/server/dateTime/resp";
    break;
  default:
    topic = "api/v2/server/dateTime/resp";
    break;
  }
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  String respType = (format == 0) ? "Plaintext" : "JSON";
  MG_LOG_I_S("# Unsubscribe ServerTime: " + _debug);
  MG_LOG_I_S("# Response type: " + respType);
  return Sub_status;
} //
boolean MAGELLAN_MQTT_device_core::unregisterResponseReport(int format)
{
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/" + token + "/report/resp/pta/+";
    break;
  case 1:
    topic = "api/v2/thing/" + token + "/report/resp";
    break;
  default:
    topic = "api/v2/thing/" + token + "/report/resp";
    break;
  }
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  String respType = (format == 0) ? "Plaintext" : "JSON";
  MG_LOG_I_S("# Unsubscribe Response Report: " + _debug);
  MG_LOG_I_S("# Response type: " + respType);
  return Sub_status;
}
boolean MAGELLAN_MQTT_device_core::unregisterResponseReportTimestamp()
{
  String topic = "api/v2/thing/" + token + "/report/timestamp/resp";
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  MG_LOG_I_S("# Unsubscribe Response ReportTimestamp: " + _debug);
  return Sub_status;
}
boolean MAGELLAN_MQTT_device_core::unregisterResponseHeartbeat(int format)
{
  String topic;
  switch (format)
  {
  case 0:
    topic = "api/v2/thing/" + token + "/heartbeat/resp/pta";
    break;
  case 1:
    topic = "api/v2/thing/" + token + "/heartbeat/resp";
    break;
  default:
    topic = "api/v2/thing/" + token + "/heartbeat/resp";
    break;
  }
  boolean Sub_status = this->client->unsubscribe(topic.c_str());
  _debug = (Sub_status == true) ? "Success" : "Failure";
  String respType = (format == 0) ? "Plaintext" : "JSON";
  MG_LOG_I_S("# Unsubscribe Response Heartbeat: " + _debug);
  MG_LOG_I_S("# Response type: " + respType);
  return Sub_status;
}
////////////////// Unsub //////////

void MAGELLAN_MQTT_device_core::disconnect()
{
  if (this->client->connected())
  {
    this->client->disconnect();
  }
}