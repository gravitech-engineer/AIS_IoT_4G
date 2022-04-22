#include "Storage.h"
#include "GSM_LOG.h"
#include "SIMBase.h"

EventGroupHandle_t _gsm_storage_flags = NULL;

#define GSM_FILE_WRITE_FAIL_FLAG          (1 << 0)
#define GSM_FILE_WRITE_SUCCESS_FLAG       (1 << 1)
#define GSM_FILE_READ_FAIL_FLAG           (1 << 2)
#define GSM_FILE_READ_SUCCESS_FLAG        (1 << 3)
#define GSM_FILE_GET_LIST_SUCCESS_FLAG    (1 << 4)
#define GSM_FILE_GET_SIZE_FAIL_FLAG       (1 << 5)
#define GSM_FILE_GET_SIZE_SUCCESS_FLAG    (1 << 6)

#define OFFSET_CALCULATE_FILE_SIZE 50

GSMStorage::GSMStorage() {
    if (!_gsm_storage_flags) {
        _gsm_storage_flags = xEventGroupCreate();
        if (!_gsm_storage_flags) {
            GSM_LOG_E("Evant flag of GSM Storage create fail");
        }
    }
}

bool GSMStorage::selectCurrentDirectory(String path) {
    if (!_SIM_Base.sendCommandFindOK("AT+FSCD=" + path)) {
        GSM_LOG_E("Select current directory fail");
        return false;
    }
    
    _current_drive = path.charAt(0);
    return true;
}

size_t GSMStorage::getFileSize(String path) {
    int startIndexOfFile = path.lastIndexOf('/');
    String fileName = path.substring(startIndexOfFile);

    if (path.charAt(0) != _current_drive) {
        if (!this->selectCurrentDirectory(path.substring(0, startIndexOfFile))) {
            return 0;
        }
    }

    xEventGroupClearBits(_gsm_storage_flags,  GSM_FILE_GET_SIZE_FAIL_FLAG | GSM_FILE_GET_SIZE_SUCCESS_FLAG);
    int sizeOfFile = 0;
    _SIM_Base.URCRegister("+FSATTRI:", [&sizeOfFile](String urcText) {
        _SIM_Base.URCDeregister("+FSATTRI:");

        int size = 0;
        char dateOfFile[30];
        if (sscanf(urcText.c_str(), "+FSATTRI: %d,%s", &size, dateOfFile) != 2) {
            GSM_LOG_E("+CIPRXGET: 2: Respont format error");
            xEventGroupSetBits(_gsm_storage_flags, GSM_FILE_GET_SIZE_FAIL_FLAG);
            return;
        }

        sizeOfFile = size;
        xEventGroupSetBits(_gsm_storage_flags, GSM_FILE_GET_SIZE_SUCCESS_FLAG);
    });

    if (!_SIM_Base.sendCommand("AT+FSATTRI=" + fileName)) {
        GSM_LOG_E("Send req get attributes error timeout");
        return 0;
    }

    EventBits_t flags;
    flags = xEventGroupWaitBits(_gsm_storage_flags, GSM_FILE_GET_SIZE_SUCCESS_FLAG, pdTRUE, pdFALSE, pdMS_TO_TICKS(500));

    if (flags & GSM_FILE_GET_SIZE_SUCCESS_FLAG) {
        GSM_LOG_I("Get list successfully");
    } else {
        GSM_LOG_E("Get list Fail");
        return 0;
    }

    if (!_SIM_Base.waitOKorERROR(500)) {
        GSM_LOG_E("Wait OK timeout");
        return 0; // Timeout
    }

    return sizeOfFile;
}

bool GSMStorage::isFileExist(String path) {
    const int index = path.lastIndexOf("/");
    String basePath = path.substring(0, index + 1);
    String fileName = path.substring(index + 1);

    ListFileString listFile = this->getListOfFiles(basePath);
    for (String s : listFile) {
        if (s.equals(fileName)) return true;
    }
    return false;
}

ListFileString GSMStorage::getListOfDirectories(String path) {
    ListFileString list;
    String fileList = this->getListOf(DIR_TYPE, path);
    int lastIndex = 0;
    for (int i = 0; i < fileList.length(); i++) {
        if (fileList.charAt(i) == '\r') {
            String str = fileList.substring(lastIndex, i);
            list.push_back(str);
            lastIndex = i+2;
        }
    }
    return list;
}

ListFileString GSMStorage::getListOfFiles(String path) {
    ListFileString list;
    String fileList = this->getListOf(FILE_TYPE, path);
    int lastIndex = 0;
    for (int i = 0; i < fileList.length(); i++) {
        if (fileList.charAt(i) == '\r') {
            String str = fileList.substring(lastIndex, i);
            list.push_back(str);
            lastIndex = i+2;
        }
    }
    return list;
}

String *_list_buff = NULL;
String GSMStorage::getListOf(ListType listType, String path) {
    if (path.charAt(0) != _current_drive) {
        if (!this->selectCurrentDirectory(path)) {
            return String();
        }
    }

    xEventGroupClearBits(_gsm_storage_flags, GSM_FILE_GET_LIST_SUCCESS_FLAG);
    int type = static_cast<int>(listType);

    String responseCheck = "+FSLS: ";
    responseCheck.concat(listType == FILE_TYPE ? "FILES:" : "SUBDIRECTORIES:");

    _SIM_Base.URCRegister(responseCheck, [responseCheck](String urcText) {
        _SIM_Base.URCDeregister(responseCheck);
        uint8_t buff[512];
        uint16_t buffSize = _SIM_Base.getDataAfterIt(buff, 512);
        
        String strBuff = "";
        for (int i = 0; i < buffSize; i++) {
            strBuff.concat(static_cast<char>(buff[i]));
        }

        int index = strBuff.indexOf("\r\nOK");
        _list_buff->clear();
        _list_buff->concat(strBuff.substring(0, index));
        xEventGroupSetBits(_gsm_storage_flags, GSM_FILE_GET_LIST_SUCCESS_FLAG);
    });
    
    String readData = "";
    _list_buff = &readData;
    if (!_SIM_Base.sendCommand("AT+FSLS=" + String(type))) {
        GSM_LOG_E("Send req get list error timeout");
        return String();
    }

    EventBits_t flags;
    flags = xEventGroupWaitBits(_gsm_storage_flags, GSM_FILE_GET_LIST_SUCCESS_FLAG, pdTRUE, pdFALSE, pdMS_TO_TICKS(1000));

    if (flags & GSM_FILE_GET_LIST_SUCCESS_FLAG) {
        GSM_LOG_I("Get list successfully");
    } else {
        GSM_LOG_E("Get list Fail");
        return String();
    }

    if (!_SIM_Base.waitOKorERROR(500)) {
        GSM_LOG_E("Wait OK timeout");
        return String(); // Timeout
    }

    return readData;
}

String *_content_to_write = NULL;
bool GSMStorage::fileWrite(String path, String content) {
    xEventGroupClearBits(_gsm_storage_flags, GSM_FILE_WRITE_SUCCESS_FLAG | GSM_FILE_WRITE_FAIL_FLAG);
    _SIM_Base.URCRegister(">", [](String urcText) {
        _SIM_Base.URCDeregister(">");
        
        uint8_t *buffOut = (uint8_t*)malloc(_content_to_write->length());

        _SIM_Base.send(*_content_to_write);

        uint32_t beforeTimeout = _SIM_Base.getTimeout();
        _SIM_Base.setTimeout(3000);

        uint16_t res_size = _SIM_Base.readBytes(buffOut, _content_to_write->length());

        _SIM_Base.setTimeout(beforeTimeout);

        if (res_size != _content_to_write->length()) {
            GSM_LOG_E("GSM file write reply data size wrong, Send : %d, Rev: %d", _content_to_write->length(), res_size);
            free(buffOut);
            xEventGroupSetBits(_gsm_storage_flags, GSM_FILE_WRITE_FAIL_FLAG);
            return;
        }

        if (memcmp(_content_to_write->c_str(), buffOut, _content_to_write->length()) != 0) {
            GSM_LOG_E("GSM file write reply data wrong");
            free(buffOut);
            xEventGroupSetBits(_gsm_storage_flags, GSM_FILE_WRITE_FAIL_FLAG);
            return;
        }

        free(buffOut);

        xEventGroupSetBits(_gsm_storage_flags, GSM_FILE_WRITE_SUCCESS_FLAG);
    });

    _content_to_write = &content;

    if (!_SIM_Base.sendCommand("AT+CFTRANRX=\"" + path + "\"," + String(content.length()))) {
        GSM_LOG_E("Send req file write error timeout");
        return false; // Timeout
    }

    EventBits_t flags;
    flags = xEventGroupWaitBits(_gsm_storage_flags, GSM_FILE_WRITE_SUCCESS_FLAG | GSM_FILE_WRITE_FAIL_FLAG, pdTRUE, pdFALSE, 3000 / portTICK_PERIOD_MS);
    if (flags & GSM_FILE_WRITE_SUCCESS_FLAG) {
        GSM_LOG_I("File %s send content OK", path.c_str());
    } else if (flags & GSM_FILE_WRITE_FAIL_FLAG) {
        GSM_LOG_I("File %s send content fail", path.c_str());
        return false;
    } else {
        GSM_LOG_E("File %s send content wait respont timeout", path.c_str());
        return false;
    }

    if (!_SIM_Base.waitOKorERROR(300)) {
        GSM_LOG_E("Wait OK timeout");
        return 0; // Timeout
    }

    return true;
}

String *_read_buffer = NULL;
String GSMStorage::fileRead(String path) {
    if (path.charAt(0) != _current_drive) {
        if (!this->selectCurrentDirectory(path.substring(0, 2))) {
            return String();
        }
    }

    xEventGroupClearBits(_gsm_storage_flags, GSM_FILE_READ_SUCCESS_FLAG | GSM_FILE_READ_FAIL_FLAG);
    _SIM_Base.URCRegister("+CFTRANTX: DATA", [](String urcText) {
        _SIM_Base.URCDeregister("+CFTRANTX: DATA");
            
        int real_data_can_read = 0;
        if (sscanf(urcText.c_str(), "+CFTRANTX: DATA,%d", &real_data_can_read) != 1) {
            GSM_LOG_E("+CIPRXGET: 2: Respont format error");
            xEventGroupSetBits(_gsm_storage_flags, GSM_FILE_READ_FAIL_FLAG);
            return;
        }
        
        uint8_t* buff = (uint8_t*)malloc(real_data_can_read + 2); // Add \r\n
        uint16_t res_size = _SIM_Base.getDataAfterIt(buff, real_data_can_read + 2); // Add \r\n
        
        _read_buffer->clear();
        for (int i=0;i<res_size - 2;i++) { // remove \r\n
            _read_buffer->concat((char)buff[i]);
        }
        free(buff);
                
        // xEventGroupSetBits(_gsm_storage_flags, GSM_FILE_READ_SUCCESS_FLAG);
    });

    _SIM_Base.URCRegister("+CFTRANTX: 0", [](String urcText) {
        _SIM_Base.URCDeregister("+CFTRANTX: 0");

        xEventGroupSetBits(_gsm_storage_flags, GSM_FILE_READ_SUCCESS_FLAG);
    });

    String read_data = "";
    _read_buffer = &read_data;
    if (!_SIM_Base.sendCommand("AT+CFTRANTX=\"" + path + "\"", 300)) {
        GSM_LOG_E("Send req read file error");
        return String(); // Timeout
    }

    EventBits_t flags;
    flags = xEventGroupWaitBits(_gsm_storage_flags, GSM_FILE_READ_SUCCESS_FLAG | GSM_FILE_READ_FAIL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
    if (flags & GSM_FILE_READ_SUCCESS_FLAG) {
        GSM_LOG_I("File %s read in buffer OK", path.c_str());
    } else if (flags & GSM_FILE_READ_FAIL_FLAG) {
        GSM_LOG_I("File %s read in buffer fail", path.c_str());
        return String();
    } else {
        GSM_LOG_E("File %s recv data in buffer timeout", path.c_str());
        return String();
    }

    if (!_SIM_Base.waitOKorERROR(300)) {
        GSM_LOG_E("File %s read data wait OK timeout", path.c_str());
    }

    return read_data;    
}

String getFileData(String data) {
    int indexCount = data.indexOf('\r');
    std::vector<String> strList;
    String capture = "";
    String tmp = "";
    
    strList.push_back(data.substring(0, indexCount));

    for (int i = indexCount; i < data.length(); i++) {
        if (data.charAt(i) == '+') {
            capture = data.substring(i, data.indexOf('\r', i));

            int sizeToRead = 0;
            if (sscanf(capture.c_str(), "+CFTRANTX: DATA,%d", &sizeToRead) == 1) {
                i+= capture.length() + 2;
                tmp = data.substring(i, i + sizeToRead);
                strList.push_back(tmp);
                i += sizeToRead;
            }
        }
    }

    tmp.clear();
    for (String str: strList) {
        tmp += str;
    }
    return tmp;
} 

String GSMStorage::readBigFile(String path) {
    if (path.charAt(0) != _current_drive) {
        if (!this->selectCurrentDirectory(path.substring(0, 2))) {
            return String();
        }
    }

    xEventGroupClearBits(_gsm_storage_flags, GSM_FILE_READ_SUCCESS_FLAG | GSM_FILE_READ_FAIL_FLAG);

    size_t sizeOfFile = this->getFileSize(path);
    if (sizeOfFile == 0) {
        GSM_LOG_I("Can't get file size");
        return String();
    }

    _SIM_Base.URCRegister("+CFTRANTX: DATA", [sizeOfFile](String urcText) {
        _SIM_Base.URCDeregister("+CFTRANTX: DATA");
            
        int real_data_can_read = 0;
        if (sscanf(urcText.c_str(), "+CFTRANTX: DATA,%d", &real_data_can_read) != 1) {
            GSM_LOG_I("+CIPRXGET: 2: Respont format error");
            xEventGroupSetBits(_gsm_storage_flags, GSM_FILE_READ_FAIL_FLAG);
            return;
        }

        int calSize = sizeOfFile + ((sizeOfFile / 512) * OFFSET_CALCULATE_FILE_SIZE);
        uint8_t* buff = (uint8_t*)malloc(calSize + 2); // Add \r\n
        uint16_t res_size = _SIM_Base.getDataAfterIt(buff, calSize + 2); // Add \r\n
        
        _read_buffer->clear();
        for (int i=0;i<res_size - 2;i++) { // remove \r\n
            _read_buffer->concat((char)buff[i]);
        }
        *_read_buffer = getFileData(*_read_buffer);

        free(buff);
                
        if (_read_buffer->length() > 0) {
            xEventGroupSetBits(_gsm_storage_flags, GSM_FILE_READ_SUCCESS_FLAG);
        }
    });

    String read_data = "";
    _read_buffer = &read_data;
    if (!_SIM_Base.sendCommand("AT+CFTRANTX=\"" + path + "\"", 300)) {
        GSM_LOG_E("Send req read file error");
        return String(); // Timeout
    }

    EventBits_t flags;
    flags = xEventGroupWaitBits(_gsm_storage_flags, GSM_FILE_READ_SUCCESS_FLAG | GSM_FILE_READ_FAIL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
    if (flags & GSM_FILE_READ_SUCCESS_FLAG) {
        GSM_LOG_I("File %s read in buffer OK", path.c_str());
    } else if (flags & GSM_FILE_READ_FAIL_FLAG) {
        GSM_LOG_E("File %s read in buffer fail", path.c_str());
        return String();
    } else {
        GSM_LOG_E("File %s recv data in buffer timeout", path.c_str());
        return String();
    }

    if (!_SIM_Base.waitOKorERROR(300)) {
        GSM_LOG_E("File %s read data wait OK timeout", path.c_str());
    }

    return read_data; 
}

bool GSMStorage::mkdir(String path) {
    if (path.endsWith("/")) {
        path = path.substring(0, path.length() - 1);
    }

    if (!this->selectCurrentDirectory(path.substring(0, path.lastIndexOf("/")))) {
        return false;
    }

    if (!_SIM_Base.sendCommandFindOK("AT+FSMKDIR=" + path.substring(path.lastIndexOf("/") + 1))) {
        GSM_LOG_E("Make new directory in current directory fail");
        return false;
    }

    return true;
}

bool GSMStorage::rmdir(String path) {
    if (path.endsWith("/")) {
        path = path.substring(0, path.length() - 1);
    }

    if (!this->selectCurrentDirectory(path.substring(0, path.lastIndexOf("/")))) {
        return false;
    }

    if (!_SIM_Base.sendCommandFindOK("AT+FSRMDIR=" + path.substring(path.lastIndexOf("/") + 1))) {
        GSM_LOG_E("Delete directory in current directory fail");
        return false;
    }

    return true;
}

bool GSMStorage::remove(String path) {
    if (!this->selectCurrentDirectory(path.substring(0, path.lastIndexOf("/")))) {
        return false;
    }

    if (!_SIM_Base.sendCommandFindOK("AT+FSDEL=" + path.substring(path.lastIndexOf("/") + 1))) {
        GSM_LOG_E("Delete directory in current directory fail");
        return false;
    }

    return true;
}

GSMStorage Storage;
