#include "SIMBase.h"
#include "GSM_LOG.h"

EventGroupHandle_t _urc_flags = NULL;
TaskHandle_t URCServiceTaskHandle = NULL;
SemaphoreHandle_t _serial_mutex = NULL;

void URCServiceTask(void*) ;

#define URC_OK_FLAG                     (1 << 0)
#define URC_ERROR_FLAG                  (1 << 1)
#define URC_COMMAND_RECHECK_FLAG        (1 << 2)
#define URC_WAITING_FOUND_FLAG          (1 << 3)
#define URC_COMMAND_READ_ONE_LINE_FLAG  (1 << 4)

#define TAKE_USE_SERIAL xSemaphoreTake(_serial_mutex, portMAX_DELAY)
#define GIVE_USE_SERIAL xSemaphoreGive(_serial_mutex)

String line_after_recheck = "";

bool SIMBase::readStringWithTimeout(String *out, uint32_t size, uint32_t timeout) {
    uint32_t beforeTimeout = this->getTimeout();
    this->setTimeout(timeout);

    char *buff = (char*)malloc(size + 1);
    memset(buff, 0, size + 1);
    uint32_t len = this->readBytes(buff, size);
    *out = String(buff);
    free(buff);

    this->setTimeout(beforeTimeout);

    return len == size;
}

bool SIMBase::readEndsWith(String *out, uint16_t max_size, String endswith, uint32_t timeout) {
    bool foundEndOfString = false;
    uint32_t startTime = millis();
    while(((millis() - startTime) < timeout) && foundEndOfString == false) {
        delay(1);
        while(this->available() && foundEndOfString == false) {
            out->concat((char)this->read());
            if (out->endsWith(endswith)) {
                foundEndOfString = true;
                break;
            }
            startTime = millis();
        }
    }

    return foundEndOfString;
}

bool SIMBase::wait(String str, uint32_t timeout) {
    uint32_t beforeTimeout = this->getTimeout();
    this->setTimeout(timeout);

    bool isFound = this->find((const char *)str.c_str());

    this->setTimeout(beforeTimeout);

    return isFound;
}

bool SIMBase::send(String str) {
    return this->send((uint8_t*)str.c_str(), str.length());
}

bool SIMBase::send(uint8_t* data, uint16_t len) {
    TAKE_USE_SERIAL;
    this->write(data, len);
    this->flush();
    GIVE_USE_SERIAL;

    return true;
}

String lastCommand;

bool SIMBase::sendCommand(String cmd, uint32_t timeout) {
    GSM_LOG_I("Send : %s", cmd.c_str());
    lastCommand = cmd;

    xEventGroupClearBits(_urc_flags, URC_COMMAND_RECHECK_FLAG);

    this->send(cmd + "\r");

    // return this->wait(finalCommand, timeout);
    EventBits_t flags = xEventGroupWaitBits(_urc_flags, URC_COMMAND_RECHECK_FLAG, pdTRUE, pdFALSE, timeout);
    if (flags & URC_COMMAND_RECHECK_FLAG) {
        return true;
    }

    if (cmd != "AT") {
        GSM_LOG_E("Wait command recheck %s timeout in %d ms", cmd.c_str(), timeout);
    }
    return false;
}


bool SIMBase::sendCommandFindOK(String cmd, uint32_t timeout) {
    if (!this->sendCommand(cmd)) {
        return false;
    }

    // return this->wait("\r\nOK\r\n", timeout);
    return _SIM_Base.waitOKorERROR(timeout) == 1;
}

bool SIMBase::sendCommandGetRespondOneLine(String cmd, String* respond, uint32_t timeout) {
    respond->clear();

    if (!this->sendCommand(cmd)) {
        GSM_LOG_E("Send command \"%s\" error timeout (1)", cmd.c_str());
        return false;
    }

    EventBits_t flags = xEventGroupWaitBits(_urc_flags, URC_COMMAND_READ_ONE_LINE_FLAG, pdTRUE, pdFALSE, timeout);
    if ((flags & URC_COMMAND_READ_ONE_LINE_FLAG) == 0) {
        GSM_LOG_E("Send command \"%s\" read one line timeout (1)", cmd.c_str());
        return false;
    }

    respond->concat(line_after_recheck);

    return this->waitOKorERROR();
}

bool SIMBase::sendCommandCheckRespond(String cmd, uint32_t timeout) {
    if (!this->sendCommand(cmd)) {
        GSM_LOG_I("Send command \"%s\" error timeout (1)", cmd.c_str());
        return false;
    }

    if (!this->wait("\r\n", timeout)) {
        GSM_LOG_I("Send command \"%s\" error timeout (2) [%d]", cmd.c_str(), timeout);
        return false;
    }

    String status = "";
    if (!this->readEndsWith(&status, 255, "\r\n", 300)) {
        GSM_LOG_I("Send command \"%s\" error timeout (3)", cmd.c_str());
        return false;
    }

    status = status.substring(0, status.length() - 2);
    GSM_LOG_I("Send command \"%s\" got : %s", cmd.c_str(), status.c_str());
    if (status == "OK") {
        return true;
    } else if (status == "ERROR") {
        return false;
    }

    return false;
}

bool SIMBase::URCServiceStart() {
    if (!_urc_flags) {
        _urc_flags = xEventGroupCreate();
        if (!_urc_flags) {
            GSM_LOG_E("Evant flag of URC create fail");
            return false;
        }
    }

    if (!_serial_mutex) {
        _serial_mutex = xSemaphoreCreateMutex();
        if (!_serial_mutex) {
            GSM_LOG_E("Mutex of URC create fail");
            return false;
        }
    }

    if (!URCServiceTaskHandle) {
        BaseType_t xReturned = xTaskCreatePinnedToCore(
            URCServiceTask,       /* Function that implements the task. */
            "URCService",         /* Text name for the task. */
            (16 * 1024) / 4,      /* Stack size in words, not bytes. */
            NULL,                 /* Parameter passed into the task. */
            10,                   /* Priority at which the task is created. */
            &URCServiceTaskHandle,/* Used to pass out the created task's handle. */
            1                     // Core 1
        );

        if (xReturned != pdPASS) {
            GSM_LOG_E("URC Service task create fail");
            return false;
        }
    }

    return true;
}

typedef struct {
    String start;
    URCHandlerFunction callback = NULL;
    void *next = NULL;
} URCRegister_node;

URCRegister_node *_urc_first_node = NULL;

URCRegister_node *getURCRegisterLastNode() {
    URCRegister_node *last_node = _urc_first_node;
    if (last_node == NULL) {
        return NULL;
    }

    while(last_node != NULL) {
        if (last_node->next) {
            last_node = (URCRegister_node*)last_node->next;
        } else {
            break;
        }
    }

    return last_node;
}

URCRegister_node *getURCRegisterNodeByStart(String urc_text) {
    URCRegister_node *last_node = _urc_first_node;
    if (last_node == NULL) {
        return NULL;
    }

    while(last_node != NULL) {
        // GSM_LOG_I("Find node : %s", last_node->start.c_str());
        if (urc_text.startsWith(last_node->start)) {
            // GSM_LOG_I("Found node : %s", last_node->start.c_str());
            break;
        } else {
            last_node = (URCRegister_node*)last_node->next;
        }
    }

    return last_node;
}

bool SIMBase::URCRegister(String start, URCHandlerFunction callback) {
    URCRegister_node *node = new URCRegister_node;
    node->start = start;
    node->callback = callback;
    node->next = NULL;

    URCRegister_node *last_node = getURCRegisterLastNode();
    if (last_node) {
        last_node->next = node;
    } else {
        _urc_first_node = node;
    }

    return true;
}

bool SIMBase::URCDeregister(String start) {
    if (_urc_first_node == NULL) {
        return NULL;
    }

    if (_urc_first_node->start == start) {
        URCRegister_node *next_node = (URCRegister_node*)_urc_first_node->next;
        free(_urc_first_node);
        _urc_first_node = next_node;
    } else {
        URCRegister_node *node = _urc_first_node;
        while (1) {
            if (node == NULL) {
                break;
            }
            URCRegister_node *next_node = (URCRegister_node*)node->next;
            if (next_node == NULL) {
                break;
            }

            if (next_node->start == start) {
                node->next = next_node->next;
                free(next_node);
                break;
            } else {
                node = next_node;
            }
        }
    }

    return true;
}

String _urc_waiting;
void SIMBase::setWaitURC(String start) {
    xEventGroupClearBits(_urc_flags, URC_WAITING_FOUND_FLAG);

    _urc_waiting = start;
}

bool SIMBase::waitURC(uint32_t timeout) {
    EventBits_t flags = xEventGroupWaitBits(_urc_flags, URC_WAITING_FOUND_FLAG, pdTRUE, pdFALSE, timeout / portTICK_PERIOD_MS);

    if (flags & URC_WAITING_FOUND_FLAG) {
        return true;
    }

    return false; // Timeout
}

char *_urc_service_data_buffer = NULL;
uint16_t _urc_service_data_buffer_index = 0;
uint16_t _urc_service_data_buffer_size = 0;

uint16_t SIMBase::getDataAfterIt(uint8_t *buff, uint16_t len, uint32_t timeout) {
    uint16_t buff_index = 0;

    if (_urc_service_data_buffer) {
        while(1) {
            if (buff_index == len) {
                break;
            }
            if (_urc_service_data_buffer_index < _urc_service_data_buffer_size) {
                buff[buff_index++] = _urc_service_data_buffer[_urc_service_data_buffer_index++];
            } else {
                break;
            }
        }
    }

    if (buff_index < len) {
        uint16_t wait_read = len - buff_index;

        uint32_t beforeTimeout = _SIM_Base.getTimeout();
        _SIM_Base.setTimeout(timeout);

        buff_index += _SIM_Base.readBytes(&buff[buff_index], wait_read);

        _SIM_Base.setTimeout(beforeTimeout);
    }

    return buff_index;
}

int8_t SIMBase::waitOKorERROR(uint32_t timeout) {
    if (!_urc_flags) {
        return -2;
    }

    EventBits_t flags = xEventGroupWaitBits(_urc_flags, URC_OK_FLAG | URC_ERROR_FLAG, pdTRUE, pdFALSE, timeout / portTICK_PERIOD_MS);

    if (flags & URC_OK_FLAG) {
        return 1; // OK found
    } else if (flags & URC_ERROR_FLAG) {
        return 0; // ERROR found
    }
    
    return -1; // Timeout
}

void URCProcess(String data) {
    GSM_LOG_I("URC Process data: %s , Wait: %s", data.c_str(), _urc_waiting.c_str());
    // return;

    if (data == "OK") {
        xEventGroupSetBits(_urc_flags, URC_OK_FLAG);
    } else if (data == "ERROR") {
        xEventGroupSetBits(_urc_flags, URC_ERROR_FLAG);
    } else if (data == _urc_waiting) {
        xEventGroupSetBits(_urc_flags, URC_WAITING_FOUND_FLAG);
        _urc_waiting = "";
    } else { // Find URC Register
        URCRegister_node *node = getURCRegisterNodeByStart(data);
        if (node) {
            if (node->callback) {
                node->callback(data);
            }
        } else {
            line_after_recheck = data;
            xEventGroupSetBits(_urc_flags, URC_COMMAND_READ_ONE_LINE_FLAG);

            GSM_LOG_W("Not found URC %s register", data.c_str());
        }
    }
}

void URCServiceTask(void*) {
    uint8_t state = 0;
    String commandRecheckBuff = "";
    String responseBuff = "";
    while(1) {
        TAKE_USE_SERIAL;
        _urc_service_data_buffer_size = _SIM_Base.available();
        GIVE_USE_SERIAL;
        if (_urc_service_data_buffer_size == 0) {
            delay(10);
            continue;
        }
        _urc_service_data_buffer = (char*)malloc(_urc_service_data_buffer_size);
        TAKE_USE_SERIAL;
        _urc_service_data_buffer_size = _SIM_Base.readBytes(_urc_service_data_buffer, _urc_service_data_buffer_size);
        GIVE_USE_SERIAL;
        _urc_service_data_buffer_index = 0;

        while (_urc_service_data_buffer_index < _urc_service_data_buffer_size) {
            char c = _urc_service_data_buffer[_urc_service_data_buffer_index++];
            
            // GSM_LOG_I("Rev: %c", c);
            if (state == 0) {
                if (c == '\r') {
                    if (commandRecheckBuff.length() > 0) {
                        GSM_LOG_I("Rev command recheck: %s", commandRecheckBuff.c_str());
                        if (commandRecheckBuff.indexOf(lastCommand) >= 0) {
                            xEventGroupSetBits(_urc_flags, URC_COMMAND_RECHECK_FLAG);
                        } else {
                            line_after_recheck = commandRecheckBuff;
                            xEventGroupSetBits(_urc_flags, URC_COMMAND_READ_ONE_LINE_FLAG);
                        }
                        commandRecheckBuff.clear();
                        state = 0;
                    } else {
                        state = 1;
                    }
                } else {
                    commandRecheckBuff += c;
                    state = 0;
                }
            } else if (state == 1) {
                if (c == '\n') {
                    responseBuff.clear();

                    state = 2;
                } else if (c == '\r') { // Found \r\r
                    state = 1;
                } else {
                    state = 0;
                }
            } else if (state == 2) {
                if (c != '\r') {
                    if (responseBuff.length() == 0 && c == '>') {
                        URCProcess(">");
                        state = 0;
                    } else {
                        responseBuff += (char)c;
                    }
                } else {
                    state = 3;
                }
            } else if (state == 3) {
                if (c == '\n') {
                    URCProcess(responseBuff);
                    responseBuff.clear();
                    state = 0;
                } else if (c == '\r') { // Found \r\r
                    state = 3;
                } else {
                    state = 0;
                }
            }
        }
        free(_urc_service_data_buffer);
    }

    vTaskDelete(NULL);
}


SIMBase _SIM_Base(1); // Serial 1

