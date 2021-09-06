#include "Storage.h"
#include "GSM_LOG.h"
#include "SIMBase.h"

EventGroupHandle_t _gsm_storage_flags = NULL;

#define GSM_FILE_WRITE_FAIL_FLAG          (1 << 0)
#define GSM_FILE_WRITE_SUCCESS_FLAG       (1 << 1)
#define GSM_FILE_READ_FAIL_FLAG           (1 << 2)
#define GSM_FILE_READ_SUCCESS_FLAG        (1 << 3)

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
