#include "GSMUdp.h"
#include "GSMNetwok.h"
#include "GSMSocket.h"
#include "SIMBase.h"
#include "GSM_LOG.h"

EventGroupHandle_t _gsm_udp_flags = NULL;

#define GSM_UDP_OPEN_SUCCESS_FLAG                (1 << 0)
#define GSM_UDP_OPEN_FAIL_FLAG                   (1 << 1)
#define GSM_UDP_SEND_DATA_TO_MODULE_SUCCESS_FLAG (1 << 2)
#define GSM_UDP_SEND_DATA_TO_MODULE_FAIL_FLAG    (1 << 3)
#define GSM_UDP_SEND_SUCCESS_FLAG                (1 << 4)
#define GSM_UDP_SEND_FAIL_FLAG                   (1 << 5)
#define GSM_UDP_RECEIVE_DATA_SIZE_SUCCESS_FLAG   (1 << 6)
#define GSM_UDP_RECEIVE_DATA_SIZE_FAIL_FLAG      (1 << 7)
#define GSM_UDP_RECEIVE_DATA_SUCCESS_FLAG        (1 << 8)
#define GSM_UDP_RECEIVE_DATA_FAIL_FLAG           (1 << 9)

GSMUdp::GSMUdp() {
    if (!_gsm_udp_flags) {
        _gsm_udp_flags = xEventGroupCreate();
        if (!_gsm_udp_flags) {
            GSM_LOG_E("Evant flag of GSM Udp create fail");
        }
    }
    
    setup_Socket();
}

static int check_socket_id = 0;
uint8_t GSMUdp::begin(uint16_t local_port) {
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
        UDP_READ_BUFFER, 
        sizeof(uint8_t)
    );
    if (!ClientSocketInfo[this->sock_id].rxQueue) {
        GSM_LOG_E("Create RX queue fail, RAM not available");
        return -5;
    }

    if (!Network.networkOpen(30 * 1000)) {
        GSM_LOG_E("Network open fail");
        return -2;
    }

    xEventGroupClearBits(_gsm_udp_flags, GSM_UDP_OPEN_SUCCESS_FLAG | GSM_UDP_OPEN_FAIL_FLAG);
    _SIM_Base.URCRegister("+CIPOPEN", [](String urcText) {
        _SIM_Base.URCDeregister("+CIPOPEN");

        int res_socket_id = -1, res_status = -1;
        if (sscanf(urcText.c_str(), "+CIPOPEN: %d,%d", &res_socket_id, &res_status) != 2) {
            GSM_LOG_E("Respont format error");
            xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_OPEN_FAIL_FLAG);
            return;
        }
            
        if (res_socket_id != check_socket_id) {
            GSM_LOG_E("Respont socket id wrong");
            xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_OPEN_FAIL_FLAG);
            return;
        }

        if (res_status != 0) {
            GSM_LOG_E("Connect fail, error code: %d", res_status);
            xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_OPEN_FAIL_FLAG);
            return;
        }
            
        xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_OPEN_SUCCESS_FLAG);
    });

    check_socket_id = this->sock_id;
    if (!_SIM_Base.sendCommandFindOK("AT+CIPOPEN=" + String(this->sock_id) + ",\"UDP\",,," + String(local_port), 300)) {
        GSM_LOG_E("Send connect TCP/IP error");
        return -1; // Timeout
    }

    EventBits_t flags = xEventGroupWaitBits(_gsm_udp_flags, GSM_UDP_OPEN_SUCCESS_FLAG | GSM_UDP_OPEN_FAIL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
    if (flags & GSM_UDP_OPEN_SUCCESS_FLAG) {
        GSM_LOG_I("Socket %d open OK", this->sock_id);
    } else if (flags & GSM_UDP_OPEN_FAIL_FLAG) {
        GSM_LOG_I("Socket %d open fail", this->sock_id);
        return -3; // TRUNCATED
    } else {
        GSM_LOG_E("Socket %d wait respont timeout", this->sock_id);
        return -1; // Timeout
    }

    if (!_SIM_Base.sendCommandFindOK("AT+CIPRXGET=1")) {
        GSM_LOG_E("Socket %d set get the network data manual fail", this->sock_id);
    }

    GSM_LOG_I("Opened !");
    ClientSocketInfo[this->sock_id].connected = true;
    
    return 1;
}

uint8_t GSMUdp::beginMulticast(IPAddress ip, uint16_t port) {
    return 0; // Not support
}

int GSMUdp::beginPacket(const char *host, uint16_t port) {
    if (this->write_buff) {
        free(this->write_buff);
        this->write_buff = NULL;
        this->write_len = 0;
    }

    this->write_buff = (uint8_t*)malloc(UDP_WRITE_BUFFER);

    this->write_host = String(host);
    this->write_port = port;

    return 1;
}

static int check_send_length = 0;
static uint8_t *_data_send_buf = NULL;
static uint16_t _data_send_size = 0;

int GSMUdp::endPacket() {
    if (this->sock_id == -1) {
        if (this->begin(random(0x0001, 0xFFFF)) <= 0) {
            return 0;
        }
    }

    if (!ClientSocketInfo[this->sock_id].connected) {
        return 0;
    }

    check_socket_id = this->sock_id;
    check_send_length = write_len;

    xEventGroupClearBits(_gsm_udp_flags, GSM_UDP_SEND_SUCCESS_FLAG | GSM_UDP_SEND_FAIL_FLAG);
    _SIM_Base.URCRegister("+CIPSEND", [](String urcText) {
        _SIM_Base.URCDeregister("+CIPSEND");
        
        int socket_id = -1, send_length = -1;
        if (sscanf(urcText.c_str(), "+CIPSEND: %d,%*d,%d", &socket_id, &send_length) != 2) {
            GSM_LOG_E("Send respont format error");
            xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_SEND_FAIL_FLAG);
            return;
        }
            
        if (socket_id != check_socket_id) {
            GSM_LOG_E("Send respont socket id wrong");
            xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_SEND_FAIL_FLAG);
            return;
        }

        if (send_length == -1) {
            GSM_LOG_E("Socket %d is disconnect so can't send data", socket_id);
            ClientSocketInfo[socket_id].connected = false;
            xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_SEND_FAIL_FLAG);
            return;
        }

        if (send_length != check_send_length) {
            GSM_LOG_E("Socket %d send respont size wrong, Send %d but real send %d", socket_id, check_send_length, send_length);
            xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_SEND_FAIL_FLAG);
            return;
        }
            
        xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_SEND_SUCCESS_FLAG);
    });

    xEventGroupClearBits(_gsm_udp_flags, GSM_UDP_SEND_DATA_TO_MODULE_SUCCESS_FLAG | GSM_UDP_SEND_DATA_TO_MODULE_FAIL_FLAG);
    _SIM_Base.URCRegister(">", [](String urcText) {
        _SIM_Base.URCDeregister(">");
        
        uint8_t *buffOut = (uint8_t*)malloc(_data_send_size);

        _SIM_Base.send(_data_send_buf, _data_send_size);

        uint32_t beforeTimeout = _SIM_Base.getTimeout();
        _SIM_Base.setTimeout(3000);

        uint16_t res_size = _SIM_Base.readBytes(buffOut, _data_send_size);

        _SIM_Base.setTimeout(beforeTimeout);

        if (res_size != _data_send_size) {
            // GSM_LOG_E("GSM reply data size wrong, Send : %d, Rev: %d", _data_send_size, res_size);
            free(buffOut);
            xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_SEND_DATA_TO_MODULE_FAIL_FLAG);
            return;
        }

        if (memcmp(_data_send_buf, buffOut, _data_send_size) != 0) {
            GSM_LOG_E("GSM reply data wrong");
            free(buffOut);
            xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_SEND_DATA_TO_MODULE_FAIL_FLAG);
            return;
        }

        free(buffOut);

        xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_SEND_DATA_TO_MODULE_SUCCESS_FLAG);
    });

    _data_send_buf = write_buff;
    _data_send_size = write_len;

    // AT+CIPSEND=<link_num>,<length>,<serverIP>,<serverPort>
    if (!_SIM_Base.sendCommand("AT+CIPSEND=" + String(this->sock_id) + "," + String(write_len) + ",\"" + write_host + "\"," + String(write_port))) {
        GSM_LOG_E("Send req send data TCP/IP error timeout");
        return 0; // Timeout
    }

    EventBits_t flags;
    flags = xEventGroupWaitBits(_gsm_udp_flags, GSM_UDP_SEND_DATA_TO_MODULE_SUCCESS_FLAG | GSM_UDP_SEND_DATA_TO_MODULE_FAIL_FLAG, pdTRUE, pdFALSE, 3000 / portTICK_PERIOD_MS);
    if (flags & GSM_UDP_SEND_DATA_TO_MODULE_SUCCESS_FLAG) {
        GSM_LOG_I("Socket %d send data to module", this->sock_id);
    } else if (flags & GSM_UDP_SEND_DATA_TO_MODULE_FAIL_FLAG) {
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

    flags = xEventGroupWaitBits(_gsm_udp_flags, GSM_UDP_SEND_SUCCESS_FLAG | GSM_UDP_SEND_FAIL_FLAG, pdTRUE, pdFALSE, 3000 / portTICK_PERIOD_MS);
    if (flags & GSM_UDP_SEND_SUCCESS_FLAG) {
        GSM_LOG_I("Socket %d send data success", this->sock_id);
    } else if (flags & GSM_UDP_SEND_FAIL_FLAG) {
        GSM_LOG_I("Socket %d send data fail", this->sock_id);
        return 0;
    } else {
        GSM_LOG_E("Socket %d send data wait respont timeout", this->sock_id);
        return 0;
    }

    int final_write = write_len;

    free(this->write_buff);
    this->write_buff = NULL;
    this->write_len = 0;

    return final_write;
}

size_t GSMUdp::write(uint8_t c) {
    return this->write(&c, 1);
}

size_t GSMUdp::write(const uint8_t *buffer, size_t size) {
    uint16_t copy_len = min(UDP_WRITE_BUFFER - this->write_len, size);
    memcpy(&this->write_buff[write_len], buffer, copy_len);
    this->write_len += copy_len;

    return copy_len;
}

static int data_in_buffer_length = 0;

int GSMUdp::parsePacket() {
    if (this->sock_id == -1) {
        GSM_LOG_E("Socket not available in available");
        return 0;
    }

    if (!ClientSocketInfo[this->sock_id].rxQueue) {
        GSM_LOG_E("No queue !");
        return 0;
    }

    if (uxQueueSpacesAvailable(ClientSocketInfo[this->sock_id].rxQueue) <= 0) {
        GSM_LOG_I("No spaces !");
        return 0;
    }

    if (!ClientSocketInfo[this->sock_id].read_request_flag) {
        // GSM_LOG_I("No flage set !");
        return 0;
    }

    check_socket_id = this->sock_id;

    xEventGroupClearBits(_gsm_udp_flags, GSM_UDP_RECEIVE_DATA_SIZE_SUCCESS_FLAG | GSM_UDP_RECEIVE_DATA_SIZE_FAIL_FLAG);
    _SIM_Base.URCRegister("+CIPRXGET: 4", [](String urcText) {
        _SIM_Base.URCDeregister("+CIPRXGET: 4");
        
        int socket_id = -1;
        if (sscanf(urcText.c_str(), "+CIPRXGET: 4,%d,%d", &socket_id, &data_in_buffer_length) != 2) {
            GSM_LOG_E("+CIPRXGET: 4: Respont format error");
            xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_RECEIVE_DATA_SIZE_FAIL_FLAG);
            return;
        }
            
        if (socket_id != check_socket_id) {
            GSM_LOG_E("+CIPRXGET: 4: Send respont socket id wrong");
            xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_RECEIVE_DATA_SIZE_FAIL_FLAG);
            return;
        }
            
        xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_RECEIVE_DATA_SIZE_SUCCESS_FLAG);
    });

    if (!_SIM_Base.sendCommand("AT+CIPRXGET=4," + String(this->sock_id))) {
        GSM_LOG_E("Send req recv data size error (1)");
        return 0; // Timeout
    }

    EventBits_t flags;
    flags = xEventGroupWaitBits(_gsm_udp_flags, GSM_UDP_RECEIVE_DATA_SIZE_SUCCESS_FLAG | GSM_UDP_RECEIVE_DATA_SIZE_FAIL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
    if (flags & GSM_UDP_RECEIVE_DATA_SIZE_SUCCESS_FLAG) {
        GSM_LOG_I("Socket %d recv data size in buffer is %d", this->sock_id, data_in_buffer_length);
    } else if (flags & GSM_UDP_RECEIVE_DATA_SIZE_FAIL_FLAG) {
        GSM_LOG_I("Socket %d recv data size in buffer fail", this->sock_id);
        return 0; // Timeout
    } else {
        GSM_LOG_E("Socket %d recv data size in buffer timeout", this->sock_id);
        return 0; // Timeout
    }

    if (data_in_buffer_length > 0) {
        check_socket_id = this->sock_id;
        
        xEventGroupClearBits(_gsm_udp_flags, GSM_UDP_RECEIVE_DATA_SUCCESS_FLAG | GSM_UDP_RECEIVE_DATA_FAIL_FLAG);
        _SIM_Base.URCRegister("+CIPRXGET: 2", [](String urcText) {
            _SIM_Base.URCDeregister("+CIPRXGET: 2");
            
            int socket_id = -1, real_data_can_read = 0;
            if (sscanf(urcText.c_str(), "+CIPRXGET: 2,%d,%d,%*d", &socket_id, &real_data_can_read) != 2) {
                GSM_LOG_E("+CIPRXGET: 2: Respont format error");
                xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_RECEIVE_DATA_FAIL_FLAG);
                return;
            }
                
            if (socket_id != check_socket_id) {
                GSM_LOG_E("+CIPRXGET: 2: Send respont socket id wrong");
                xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_RECEIVE_DATA_FAIL_FLAG);
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
                
            xEventGroupSetBits(_gsm_udp_flags, GSM_UDP_RECEIVE_DATA_SUCCESS_FLAG);
        });

        int bufferFree = uxQueueSpacesAvailable(ClientSocketInfo[this->sock_id].rxQueue);
        uint16_t read_size = min(min(data_in_buffer_length, bufferFree), 1500);
        GSM_LOG_I("Data in GSM %d , Buffer free: %d, Read size %d", data_in_buffer_length, bufferFree, read_size);
        if (!_SIM_Base.sendCommand("AT+CIPRXGET=2," + String(this->sock_id) + "," + String(read_size), 300)) {
            GSM_LOG_E("Send req recv data error");
            return 0; // Timeout
        }

        EventBits_t flags;
        flags = xEventGroupWaitBits(_gsm_udp_flags, GSM_UDP_RECEIVE_DATA_SUCCESS_FLAG | GSM_UDP_RECEIVE_DATA_FAIL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
        if (flags & GSM_UDP_RECEIVE_DATA_SUCCESS_FLAG) {
            GSM_LOG_I("Socket %d recv data in buffer", this->sock_id);
        } else if (flags & GSM_UDP_RECEIVE_DATA_FAIL_FLAG) {
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

    return data_in_buffer_length;
}

int GSMUdp::available() {
    if (this->sock_id == -1) {
        return -1;
    }

    if (!ClientSocketInfo[this->sock_id].rxQueue) {
        return -1;
    }

    return uxQueueMessagesWaiting(ClientSocketInfo[this->sock_id].rxQueue);
}

int GSMUdp::read() {
    char c;
    if (this->read((uint8_t*)&c, 1) >= 0) {
        return c;
    }
    
    return -1;
}

int GSMUdp::read(unsigned char* buffer, size_t len) {
    return this->read((char*)buffer, len);
}

int GSMUdp::read(char* buffer, size_t len) {
    if (this->sock_id == -1) {
        return -1;
    }

    if (!ClientSocketInfo[this->sock_id].rxQueue) {
        return -1;
    }

    size_t dataWaitRead = uxQueueMessagesWaiting(ClientSocketInfo[this->sock_id].rxQueue);
    if (dataWaitRead == 0) {
        return -1;
    }

    uint8_t realRead = 0;
	for (int i=0;i<min(dataWaitRead, len);i++) {
		if (xQueueReceive(ClientSocketInfo[this->sock_id].rxQueue, &buffer[i], 0) == pdTRUE) {
            realRead++;
        } else {
            break;
        }
	}

    return realRead;
}

int GSMUdp::peek() {
    if (this->sock_id == -1) {
        return -1;
    }

    if (!ClientSocketInfo[this->sock_id].rxQueue) {
        return -1;
    }

    size_t dataWaitRead = uxQueueMessagesWaiting(ClientSocketInfo[this->sock_id].rxQueue);
    if (dataWaitRead == 0) {
        return -1;
    }

    uint8_t c;
    if (xQueuePeek(ClientSocketInfo[this->sock_id].rxQueue, &c, 0) != pdTRUE) {
        return -1;
    }

    return c;
}

void GSMUdp::flush() {
    // Not support
}

IPAddress GSMUdp::remoteIP() {
    return this->remote_ip;
}

uint16_t GSMUdp::remotePort() {
    return this->remote_port;
}

void GSMUdp::stop() {
    if ((this->sock_id < 0) || (this->sock_id > 9)) {
        return;
    }

    ClientSocketInfo[this->sock_id].itUsing = false;
    if (ClientSocketInfo[this->sock_id].rxQueue) {
        vQueueDelete(ClientSocketInfo[this->sock_id].rxQueue);
        ClientSocketInfo[this->sock_id].rxQueue = NULL;
    }

    if (ClientSocketInfo[this->sock_id].connected) {
        if (!_SIM_Base.sendCommandFindOK("AT+CIPCLOSE=" + String(this->sock_id), 500)) {
            GSM_LOG_E("Send disconnect UDP error");
            return; // Timeout
        }
    }

    this->sock_id = -1;
}

GSMUdp::~GSMUdp() {
    this->stop();
}

