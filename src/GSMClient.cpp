#include "GSMClient.h"
#include "GSMNetwok.h"
#include "GSM_LOG.h"
#include "GSMSocket.h"

EventGroupHandle_t _gsm_client_flags = NULL;

#define GSM_CLIENT_CONNECTED_FLAG          (1 << 0)
#define GSM_CLIENT_CONNECT_FAIL_FLAG       (1 << 1)
#define GSM_CLIENT_DISCONNAECTED_FLAG      (1 << 2)
#define GSM_CLIENT_DISCONNAECT_FAIL_FLAG   (1 << 3)
#define GSM_CLIENT_SEND_DATA_TO_MODULE_SUCCESS_FLAG (1 << 4)
#define GSM_CLIENT_SEND_DATA_TO_MODULE_FAIL_FLAG    (1 << 5)
#define GSM_CLIENT_SEND_FAIL_FLAG          (1 << 6)
#define GSM_CLIENT_SEND_SUCCESS_FLAG       (1 << 7)
#define GSM_CLIENT_RECEIVE_DATA_SIZE_SUCCESS_FLAG   (1 << 8)
#define GSM_CLIENT_RECEIVE_DATA_SIZE_FAIL_FLAG      (1 << 9)
#define GSM_CLIENT_RECEIVE_DATA_SUCCESS_FLAG   (1 << 10)
#define GSM_CLIENT_RECEIVE_DATA_FAIL_FLAG   (1 << 11)

static bool setupURC = false;
static int check_socket_id = 0;

GSMClient::GSMClient() {
    if (!_gsm_client_flags) {
        _gsm_client_flags = xEventGroupCreate();
        if (!_gsm_client_flags) {
            GSM_LOG_E("Evant flag of GSM Client create fail");
        }
    }

    if (!setupURC) {
        _SIM_Base.URCRegister("+CIPCLOSE", [](String urcText) {
            int socket_id = -1, error = -1;
            if (sscanf(urcText.c_str(), "+CIPCLOSE: %d,%d", &socket_id, &error) != 2) {
                GSM_LOG_E("Close status format fail");
                xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_DISCONNAECT_FAIL_FLAG);
                return;
            }

            if ((socket_id < 0) || (socket_id > 9)) {
                GSM_LOG_E("Socket %d is out of range", socket_id);
                xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_DISCONNAECT_FAIL_FLAG);
                return;
            }

            if (error != 0) {
                GSM_LOG_I("Socket %d close error code: %d", socket_id, error);
            }

            GSM_LOG_I("Socket %d is close", socket_id);
            ClientSocketInfo[socket_id].connected = false;
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_DISCONNAECTED_FLAG);
        });

        _SIM_Base.URCRegister("+IPCLOSE", [](String urcText) {
            int socket_id = -1, close_because = -1;
            if (sscanf(urcText.c_str(), "+IPCLOSE: %d,%d", &socket_id, &close_because) != 2) {
                GSM_LOG_E("Close status format fail");
                xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_DISCONNAECT_FAIL_FLAG);
                return;
            }

            if ((socket_id < 0) || (socket_id > 9)) {
                GSM_LOG_E("Socket %d is out of range", socket_id);
                xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_DISCONNAECT_FAIL_FLAG);
                return;
            }

            GSM_LOG_I("Socket %d is close bacause %d", socket_id, close_because);
            ClientSocketInfo[socket_id].connected = false;
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_DISCONNAECTED_FLAG);
        });

        setupURC = true;
    }

    setup_Socket();
}

int GSMClient::connect(IPAddress ip, uint16_t port, int32_t timeout) {
    return this->connect(ip.toString().c_str(), port, timeout);
}

int GSMClient::connect(const char *host, uint16_t port, int32_t timeout) {
    if (this->sock_id != -1) {
        this->stop();
    }

    // Find socket
    for (uint8_t i=0;i<10;i++) {
        if (!ClientSocketInfo[i].itUsing) {
            ClientSocketInfo[i].itUsing = true;
            this->sock_id = i;
            GSM_LOG_I("Socket %d free !", i);
            break;
        } else {
            GSM_LOG_I("Socket %d using", i);
        }
    }

    if (this->sock_id == -1) {
        GSM_LOG_E("Socket not available in connect");
        return -5;
    }

    ClientSocketInfo[this->sock_id].rxQueue = xQueueCreate(
        GSM_TCP_BUFFER, 
        sizeof(uint8_t)
    );
    if (!ClientSocketInfo[this->sock_id].rxQueue) {
        GSM_LOG_E("Create RX queue fail, RAM not available");
        return -5;
    }

    if (!_SIM_Base.sendCommandFindOK("AT+CIPTIMEOUT=" + String(timeout) + "," + String(timeout) + "," + String(timeout))) {
        GSM_LOG_I("Set timeout error");
    }

    if (!Network.networkOpen(timeout)) {
        GSM_LOG_E("Network open fail");
        return -2;
    }

    xEventGroupClearBits(_gsm_client_flags, GSM_CLIENT_CONNECT_FAIL_FLAG | GSM_CLIENT_CONNECT_FAIL_FLAG);
    _SIM_Base.URCRegister("+CIPOPEN", [](String urcText) {
        _SIM_Base.URCDeregister("+CIPOPEN");

        int res_socket_id = -1, res_status = -1;
        if (sscanf(urcText.c_str(), "+CIPOPEN: %d,%d", &res_socket_id, &res_status) != 2) {
            GSM_LOG_E("Respont format error");
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_CONNECT_FAIL_FLAG);
            return;
        }
            
        if (res_socket_id != check_socket_id) {
            GSM_LOG_E("Respont socket id wrong");
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_CONNECT_FAIL_FLAG);
            return;
        }

        if (res_status != 0) {
            GSM_LOG_E("Connect fail, error code: %d", res_status);
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_CONNECT_FAIL_FLAG);
            return;
        }
            
        xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_CONNECTED_FLAG);
    });

    check_socket_id = this->sock_id;
    if (!_SIM_Base.sendCommandFindOK("AT+CIPOPEN=" + String(this->sock_id) + ",\"TCP\",\"" + String(host) + "\"," + String(port), timeout)) {
        GSM_LOG_E("Send connect TCP/IP error");
        return -1; // Timeout
    }

    EventBits_t flags = xEventGroupWaitBits(_gsm_client_flags, GSM_CLIENT_CONNECTED_FLAG | GSM_CLIENT_CONNECT_FAIL_FLAG, pdTRUE, pdFALSE, (timeout + 3000) / portTICK_PERIOD_MS);
    if (flags & GSM_CLIENT_CONNECTED_FLAG) {
        GSM_LOG_I("Socket %d connected", this->sock_id);
    } else if (flags & GSM_CLIENT_CONNECT_FAIL_FLAG) {
        GSM_LOG_I("Socket %d connect fail", this->sock_id);
        return -3; // TRUNCATED
    } else {
        GSM_LOG_E("Socket %d wait respont timeout", this->sock_id);
        return -1; // Timeout
    }

    if (!_SIM_Base.sendCommandFindOK("AT+CIPRXGET=1")) {
        GSM_LOG_E("Socket %d set get the network data manual fail", this->sock_id);
    }

    GSM_LOG_I("Connected !");
    ClientSocketInfo[this->sock_id].connected = true;

    return 1;
}

size_t GSMClient::write(uint8_t c) {
    return this->write((const uint8_t *)&c, 1);
}

static int check_send_length = 0;
static uint8_t *_data_send_buf = NULL;
static uint16_t _data_send_size = 0;

size_t GSMClient::write(const uint8_t *buf, size_t size) {
    if (this->sock_id == -1) {
        GSM_LOG_E("Socket not available in write");
        return 0;
    }

    if (!ClientSocketInfo[this->sock_id].connected) {
        return 0;
    }

    check_socket_id = this->sock_id;
    check_send_length = size;

    xEventGroupClearBits(_gsm_client_flags, GSM_CLIENT_SEND_SUCCESS_FLAG | GSM_CLIENT_SEND_FAIL_FLAG);
    _SIM_Base.URCRegister("+CIPSEND", [](String urcText) {
        _SIM_Base.URCDeregister("+CIPSEND");
        
        int socket_id = -1, send_length = -1;
        if (sscanf(urcText.c_str(), "+CIPSEND: %d,%*d,%d", &socket_id, &send_length) != 2) {
            GSM_LOG_E("Send respont format error");
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_SEND_FAIL_FLAG);
            return;
        }
            
        if (socket_id != check_socket_id) {
            GSM_LOG_E("Send respont socket id wrong");
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_SEND_FAIL_FLAG);
            return;
        }

        if (send_length == -1) {
            GSM_LOG_E("Socket %d is disconnect so can't send data", socket_id);
            ClientSocketInfo[socket_id].connected = false;
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_SEND_FAIL_FLAG);
            return;
        }

        if (send_length != check_send_length) {
            GSM_LOG_E("Socket %d send respont size wrong, Send %d but real send %d", socket_id, check_send_length, send_length);
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_SEND_FAIL_FLAG);
            return;
        }
            
        xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_SEND_SUCCESS_FLAG);
    });

    xEventGroupClearBits(_gsm_client_flags, GSM_CLIENT_SEND_DATA_TO_MODULE_SUCCESS_FLAG | GSM_CLIENT_SEND_DATA_TO_MODULE_FAIL_FLAG);
    _SIM_Base.URCRegister(">", [](String urcText) {
        _SIM_Base.URCDeregister(">");

        uint8_t *buffOut = (uint8_t*)malloc(_data_send_size);

        delay(50);
        _SIM_Base.send(_data_send_buf, _data_send_size);


        uint32_t beforeTimeout = _SIM_Base.getTimeout();
        _SIM_Base.setTimeout(500);

        uint16_t res_size = 0;
        while(1) { // Try to fix bug
            uint16_t n = _SIM_Base.readBytes(&buffOut[0], 1);
            if (n != 1) {
                GSM_LOG_E("GSM reply timeout");
                free(buffOut);
                xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_SEND_DATA_TO_MODULE_FAIL_FLAG);
                return;
            }
            if (buffOut[0] == _data_send_buf[0]) {
                res_size += 1;
                break;
            } else if (buffOut[0] == '\r') { // I think i found URC return
                if (_SIM_Base.find('\n')) {
                    String urc = _SIM_Base.readStringUntil('\r');
                    if (_SIM_Base.find('\n')) {
                        urc = urc.substring(0, urc.length() - 1);
                        GSM_LOG_E("Try to process URC: %s, ", urc.c_str());
                        extern void URCProcess(String data);
                        URCProcess(urc);
                    } else {
                        GSM_LOG_E("Try to read URC but fail (2)");
                        free(buffOut);
                        xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_SEND_DATA_TO_MODULE_FAIL_FLAG);
                        return;
                    }
                } else {
                    GSM_LOG_E("Try to read URC but fail (1)");
                    free(buffOut);
                    xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_SEND_DATA_TO_MODULE_FAIL_FLAG);
                    return;
                }
            }
        }

        _SIM_Base.setTimeout(3000);
        res_size += _SIM_Base.readBytes(&buffOut[1], _data_send_size - 1);
        
        // uint16_t res_size = _SIM_Base.readBytes(buffOut, _data_send_size);

        if (res_size != _data_send_size) {
            GSM_LOG_E("GSM reply data size wrong, Send : %d, Rev: %d", _data_send_size, res_size);
            free(buffOut);
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_SEND_DATA_TO_MODULE_FAIL_FLAG);
            return;
        }

        if (memcmp(_data_send_buf, buffOut, _data_send_size) != 0) {
            GSM_LOG_E("GSM reply data wrong");
            free(buffOut);
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_SEND_DATA_TO_MODULE_FAIL_FLAG);
            return;
        }

        _SIM_Base.setTimeout(beforeTimeout);

        free(buffOut);

        xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_SEND_DATA_TO_MODULE_SUCCESS_FLAG);
    });
    
    _SIM_Base.URCRegister("+CIPERROR:", [](String urcText) {
        _SIM_Base.URCDeregister("+CIPERROR:");

        int error = -1;
        if (sscanf(urcText.c_str(), "+CIPERROR: %d", &error) != 1) {
            GSM_LOG_E("+CIPERROR: response format error");
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_SEND_DATA_TO_MODULE_FAIL_FLAG);
            return;
        }
        
        if (error == 4) {
            GSM_LOG_E("Socket %d is disconnect so can't send data", check_socket_id);
            ClientSocketInfo[check_socket_id].connected = false;
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_SEND_DATA_TO_MODULE_FAIL_FLAG);
        }
    });

    _data_send_buf = (uint8_t*)buf;
    _data_send_size = size;

    if (!_SIM_Base.sendCommand("AT+CIPSEND=" + String(this->sock_id) + "," + String(size))) {
        GSM_LOG_E("Send req send data TCP/IP error timeout");
        return 0; // Timeout
    }

    EventBits_t flags;
    flags = xEventGroupWaitBits(_gsm_client_flags, GSM_CLIENT_SEND_DATA_TO_MODULE_SUCCESS_FLAG | GSM_CLIENT_SEND_DATA_TO_MODULE_FAIL_FLAG, pdTRUE, pdFALSE, 3000 / portTICK_PERIOD_MS);
    _SIM_Base.URCDeregister("+CIPERROR:");
    if (flags & GSM_CLIENT_SEND_DATA_TO_MODULE_SUCCESS_FLAG) {
        GSM_LOG_I("Socket %d send data to module", this->sock_id);
    } else if (flags & GSM_CLIENT_SEND_DATA_TO_MODULE_FAIL_FLAG) {
        GSM_LOG_I("Socket %d send data to module fail", this->sock_id);
        return 0;
    } else {
        GSM_LOG_E("Socket %d send data to module wait respont timeout", this->sock_id);
        return 0;
    }

    if (!_SIM_Base.waitOKorERROR(300)) {
        GSM_LOG_E("Wait OK timeout");
        return 0; // Timeout
    }

    flags = xEventGroupWaitBits(_gsm_client_flags, GSM_CLIENT_SEND_SUCCESS_FLAG | GSM_CLIENT_SEND_FAIL_FLAG, pdTRUE, pdFALSE, 3000 / portTICK_PERIOD_MS);
    if (flags & GSM_CLIENT_SEND_SUCCESS_FLAG) {
        GSM_LOG_I("Socket %d send data success", this->sock_id);
    } else if (flags & GSM_CLIENT_SEND_FAIL_FLAG) {
        GSM_LOG_I("Socket %d send data fail", this->sock_id);
        return 0;
    } else {
        GSM_LOG_E("Socket %d send data wait respont timeout", this->sock_id);
        return 0;
    }

    return size;
}

static int data_in_buffer_length = 0;

int GSMClient::available() {
    if (this->sock_id == -1) {
        GSM_LOG_E("Socket not available in available");
        return 0;
    }

    if (!ClientSocketInfo[this->sock_id].rxQueue) {
        GSM_LOG_E("No queue !");
        return 0;
    }

    size_t dataWaitRead = uxQueueMessagesWaiting(ClientSocketInfo[this->sock_id].rxQueue);
    if (uxQueueSpacesAvailable(ClientSocketInfo[this->sock_id].rxQueue) <= 0) {
        GSM_LOG_I("No spaces !");
        return dataWaitRead;
    }

    if (!ClientSocketInfo[this->sock_id].read_request_flag) {
        // GSM_LOG_I("No flage set !");
        return dataWaitRead;
    }

    check_socket_id = this->sock_id;

    xEventGroupClearBits(_gsm_client_flags, GSM_CLIENT_RECEIVE_DATA_SIZE_SUCCESS_FLAG | GSM_CLIENT_RECEIVE_DATA_SIZE_FAIL_FLAG);
    _SIM_Base.URCRegister("+CIPRXGET: 4", [](String urcText) {
        _SIM_Base.URCDeregister("+CIPRXGET: 4");
        
        int socket_id = -1;
        if (sscanf(urcText.c_str(), "+CIPRXGET: 4,%d,%d", &socket_id, &data_in_buffer_length) != 2) {
            GSM_LOG_E("+CIPRXGET: 4: Respont format error");
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_RECEIVE_DATA_SIZE_FAIL_FLAG);
            return;
        }
            
        if (socket_id != check_socket_id) {
            GSM_LOG_E("+CIPRXGET: 4: Send respont socket id wrong");
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_RECEIVE_DATA_SIZE_FAIL_FLAG);
            return;
        }
            
        xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_RECEIVE_DATA_SIZE_SUCCESS_FLAG);
    });

    if (!_SIM_Base.sendCommand("AT+CIPRXGET=4," + String(this->sock_id))) {
        GSM_LOG_E("Send req recv data size error (1)");
        return 0; // Timeout
    }

    EventBits_t flags;
    flags = xEventGroupWaitBits(_gsm_client_flags, GSM_CLIENT_RECEIVE_DATA_SIZE_SUCCESS_FLAG | GSM_CLIENT_RECEIVE_DATA_SIZE_FAIL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
    if (flags & GSM_CLIENT_RECEIVE_DATA_SIZE_SUCCESS_FLAG) {
        GSM_LOG_I("Socket %d recv data size in buffer is %d", this->sock_id, data_in_buffer_length);
    } else if (flags & GSM_CLIENT_RECEIVE_DATA_SIZE_FAIL_FLAG) {
        GSM_LOG_I("Socket %d recv data size in buffer fail", this->sock_id);
        return 0; // Timeout
    } else {
        GSM_LOG_E("Socket %d recv data size in buffer timeout", this->sock_id);
        return 0; // Timeout
    }

    if (data_in_buffer_length > 0) {
        check_socket_id = this->sock_id;
        
        xEventGroupClearBits(_gsm_client_flags, GSM_CLIENT_RECEIVE_DATA_SUCCESS_FLAG | GSM_CLIENT_RECEIVE_DATA_FAIL_FLAG);
        _SIM_Base.URCRegister("+CIPRXGET: 2", [](String urcText) {
            _SIM_Base.URCDeregister("+CIPRXGET: 2");
            
            int socket_id = -1, real_data_can_read = 0;
            if (sscanf(urcText.c_str(), "+CIPRXGET: 2,%d,%d,%*d", &socket_id, &real_data_can_read) != 2) {
                GSM_LOG_E("+CIPRXGET: 2: Respont format error");
                xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_RECEIVE_DATA_FAIL_FLAG);
                return;
            }
                
            if (socket_id != check_socket_id) {
                GSM_LOG_E("+CIPRXGET: 2: Send respont socket id wrong");
                xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_RECEIVE_DATA_FAIL_FLAG);
                return;
            }

            uint8_t* buff = (uint8_t*)malloc(real_data_can_read + 2); // Add \r\n

            uint16_t res_size = _SIM_Base.getDataAfterIt(buff, real_data_can_read + 2); // Add \r\n

            for (int i=0;i<res_size - 2;i++) { // remove \r\n
                if (xQueueSend(ClientSocketInfo[socket_id].rxQueue, &buff[i], 0) != pdTRUE) {
                    GSM_LOG_E("Queue is full ?");
                    break;
                }
            }
            free(buff);
                
            xEventGroupSetBits(_gsm_client_flags, GSM_CLIENT_RECEIVE_DATA_SUCCESS_FLAG);
        });

        int bufferFree = uxQueueSpacesAvailable(ClientSocketInfo[this->sock_id].rxQueue);
        uint16_t read_size = min(min(data_in_buffer_length, bufferFree), 1500);
        GSM_LOG_I("Data in GSM %d , Buffer free: %d, Read size %d", data_in_buffer_length, bufferFree, read_size);
        if (!_SIM_Base.sendCommand("AT+CIPRXGET=2," + String(this->sock_id) + "," + String(read_size), 300)) {
            GSM_LOG_E("Send req recv data error");
            return 0; // Timeout
        }

        EventBits_t flags;
        flags = xEventGroupWaitBits(_gsm_client_flags, GSM_CLIENT_RECEIVE_DATA_SUCCESS_FLAG | GSM_CLIENT_RECEIVE_DATA_FAIL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
        if (flags & GSM_CLIENT_RECEIVE_DATA_SUCCESS_FLAG) {
            GSM_LOG_I("Socket %d recv data in buffer", this->sock_id);
        } else if (flags & GSM_CLIENT_RECEIVE_DATA_FAIL_FLAG) {
            GSM_LOG_I("Socket %d recv data in buffer fail", this->sock_id);
            return 0;
        } else {
            GSM_LOG_E("Socket %d recv data in buffer timeout", this->sock_id);
            return 0;
        }

        if (!_SIM_Base.waitOKorERROR(300)) {
            GSM_LOG_E("Socket %d recv data wait OK timeout", this->sock_id);
        }

        if ((data_in_buffer_length - read_size) > 0) {
            ClientSocketInfo[this->sock_id].read_request_flag = true;
        } else {
            ClientSocketInfo[this->sock_id].read_request_flag = false;
        }
    } else {
        ClientSocketInfo[this->sock_id].read_request_flag = false;
    }

    return uxQueueMessagesWaiting(ClientSocketInfo[this->sock_id].rxQueue);
}

int GSMClient::read() {
    char c;
    if (this->read((uint8_t*)&c, 1) >= 0) {
        return c;
    }
    
    return -1;
}

int GSMClient::read(uint8_t *buf, size_t size) {
    if (this->sock_id == -1) {
        return -1;
    }

    if (!ClientSocketInfo[this->sock_id].rxQueue) {
        return -1;
    }

    size_t dataWaitRead = uxQueueMessagesWaiting(ClientSocketInfo[this->sock_id].rxQueue);
    if ((dataWaitRead == 0) || ClientSocketInfo[this->sock_id].read_request_flag) {
        dataWaitRead = this->available();
    }

    if (dataWaitRead == 0) {
        return -1;
    }

    uint8_t realRead = 0;
	for (int i=0;i<min(dataWaitRead, size);i++) {
		if (xQueueReceive(ClientSocketInfo[this->sock_id].rxQueue, &buf[i], 0) == pdTRUE) {
            realRead++;
        } else {
            break;
        }
	}

    return realRead;
}

int GSMClient::peek() {
    if (this->sock_id == -1) {
        return -1;
    }

    if (!ClientSocketInfo[this->sock_id].rxQueue) {
        return -1;
    }

    size_t dataWaitRead = uxQueueMessagesWaiting(ClientSocketInfo[this->sock_id].rxQueue);
    if (dataWaitRead == 0) {
        dataWaitRead = this->available();
    }

    if (dataWaitRead == 0) {
        return -1;
    }

    uint8_t c;
    if (xQueuePeek(ClientSocketInfo[this->sock_id].rxQueue, &c, 0) != pdTRUE) {
        return -1;
    }

    return c;
}

void GSMClient::flush() { // Not support
    
}

uint8_t GSMClient::connected() {
    if (this->sock_id == -1) {
        GSM_LOG_E("Socket not available in connected");
        return 0;
    }

    return ClientSocketInfo[this->sock_id].connected || (this->available() > 0);
}

GSMClient::operator bool() {
    return this->connected();
}

void GSMClient::stop() {
    if ((this->sock_id < 0) || (this->sock_id > 9)) {
        return;
    }

    ClientSocketInfo[this->sock_id].itUsing = false;
    if (ClientSocketInfo[this->sock_id].rxQueue) {
        vQueueDelete(ClientSocketInfo[this->sock_id].rxQueue);
        ClientSocketInfo[this->sock_id].rxQueue = NULL;
    }

    if (ClientSocketInfo[this->sock_id].connected) {
        xEventGroupClearBits(_gsm_client_flags, GSM_CLIENT_DISCONNAECTED_FLAG | GSM_CLIENT_DISCONNAECT_FAIL_FLAG);

        if (!_SIM_Base.sendCommandFindOK("AT+CIPCLOSE=" + String(this->sock_id), 500)) {
            GSM_LOG_E("Send disconnect TCP/IP error");
            return; // Timeout
        }

        EventBits_t flags = xEventGroupWaitBits(_gsm_client_flags, GSM_CLIENT_DISCONNAECTED_FLAG | GSM_CLIENT_DISCONNAECT_FAIL_FLAG, pdTRUE, pdFALSE, 500 / portTICK_PERIOD_MS);
        if (flags & GSM_CLIENT_DISCONNAECTED_FLAG) {
            GSM_LOG_I("Socket %d disconnected", this->sock_id);
        } else if (flags & GSM_CLIENT_DISCONNAECT_FAIL_FLAG) {
            GSM_LOG_I("Socket %d disconnect fail", this->sock_id);
        } else {
            GSM_LOG_E("Socket %d wait respont timeout", this->sock_id);
        }
    }

    this->sock_id = -1;
}

GSMClient::~GSMClient() {
    this->stop();
}
