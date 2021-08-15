#include "GSMClient.h"
#include "GSMNetwok.h"
#include "GSM_LOG.h"

struct {
    bool itUsing = false;
} ClientSocketInfo[10];

GSMClient::GSMClient() {
    
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
            break;
        }
    }

    if (this->sock_id == -1) {
        GSM_LOG_E("Socket not available");
        return -5;
    }

    if (!rxQueue) {
        rxQueue = xQueueCreate(
            GSM_TCP_BUFFER, 
            sizeof(uint8_t)
        );
        if (!rxQueue) {
            GSM_LOG_E("Create RX queue fail, RAM not available");
            return -5;
        }
    }
    xQueueReset(rxQueue);

    if (!_SIM_Base.sendCommandCheckRespond("AT+CIPTIMEOUT=" + String(timeout) + "," + String(timeout) + "," + String(timeout))) {
        GSM_LOG_E("Set timeout error");
    }

    if (!Network.networkOpen(timeout)) {
        GSM_LOG_E("Network open fail");
        return -2;
    }

    String connectStatusBuffer = "";
    if (!_SIM_Base.sendCommandGetRespondOneLine("AT+CIPOPEN=" + String(this->sock_id) + ",\"TCP\",\"" + String(host) + "\"," + String(port), &connectStatusBuffer, timeout)) {
        GSM_LOG_E("Send connect TCP/IP error");
        return -1; // Timeout
    }

    int res_socket_id = -1, res_status = -1;
    if (sscanf(connectStatusBuffer.c_str(), "+CIPOPEN: %d,%d", &res_socket_id, &res_status) != 2) {
        GSM_LOG_E("Respont format error");
        return -4; // INVALID_RESPONSE
    }

    if (res_socket_id != this->sock_id) {
        GSM_LOG_E("Respont socket id wrong");
        return -4; // INVALID_RESPONSE
    }

    if (res_status != 0) {
        GSM_LOG_E("Connect fail, error code: %d", res_status);
        return -3; // TRUNCATED
    }

    if (!_SIM_Base.sendCommandCheckRespond("AT+CIPRXGET=1")) {
        GSM_LOG_E("Set get the network data manual fail");
    }

    GSM_LOG_I("Connected !");
    this->_connected = true;

    return 1;
}

size_t GSMClient::write(uint8_t c) {
    return this->write((const uint8_t *)&c, 1);
}

size_t GSMClient::write(const uint8_t *buf, size_t size) {
    if (!this->_connected) {
        return 0;
    }

    if (!_SIM_Base.sendCommand("AT+CIPSEND=" + String(this->sock_id) + "," + String(size))) {
        GSM_LOG_E("Send req send data TCP/IP error");
        return 0; // Timeout
    }

    if (!_SIM_Base.wait("\r\n>")) {
        GSM_LOG_E("Wait > timeout");
        return 0; // Timeout
    }

    _SIM_Base.write(buf, size);
    _SIM_Base.flush();

    uint8_t *buffOut = (uint8_t*)malloc(size);
    _SIM_Base.setTimeout(1000);
    size_t res_size = _SIM_Base.readBytes(buffOut, size);
    if (res_size != size) {
        GSM_LOG_E("GSM reply data size wrong, Send : %d, Rev: %d", size, res_size);
        free(buffOut);
        return 0; // Timeout
    }

    if (memcmp(buf, buffOut, size) != 0) {
        GSM_LOG_E("GSM reply data wrong");
        free(buffOut);
        return 0; // Timeout
    }
    free(buffOut);

    if (!_SIM_Base.wait("\r\nOK\r\n")) {
        GSM_LOG_E("Wait > timeout");
        return 0; // Timeout
    }

    if (!_SIM_Base.wait("\r\n")) {
        GSM_LOG_E("Wait > timeout");
        return 0; // Timeout
    }

    String sendStatusBuffer = "";
    if (!_SIM_Base.readEndsWith(&sendStatusBuffer, 50, "\r\n", 300)) {
        GSM_LOG_I("Wait data reply timeout (2)");
        return 0;
    }

    sendStatusBuffer = sendStatusBuffer.substring(0, sendStatusBuffer.length() - 2);
    GSM_LOG_I("Send command got : %s", sendStatusBuffer.c_str());

    int res_socket = -1, res_send_length = 0, res_real_send_length = 0;
    if (sscanf(sendStatusBuffer.c_str(), "+CIPSEND: %d,%d,%d", &res_socket, &res_send_length, &res_real_send_length) != 3) {
        GSM_LOG_E("Get TCP/IP send data format error");
        return 0;
    }

    if (res_real_send_length == -1) {
        GSM_LOG_E("TCP/IP is disconnect so can't send data");
        this->_connected = false;
        return 0;
    }

    if (res_send_length != size) {
        GSM_LOG_E("Req send data size back error, needs %d but got %d", size, res_send_length);
        return 0;
    }

    if (res_real_send_length != size) {
        GSM_LOG_E("Real send data size back error, needs %d but got %d (Go disconnect)", size, res_real_send_length);
        this->stop();
        return 0;
    }

    return size;
}

int GSMClient::available() {
    if (this->sock_id != -1) {
        return 0;
    }

    String dataAvailableBuffer = "";
    if (!_SIM_Base.sendCommandGetRespondOneLine("AT+CIPRXGET=4," + String(this->sock_id), &dataAvailableBuffer, 300)) {
        GSM_LOG_E("Send get Data Available TCP/IP error");
        return 0; // Timeout
    }

    int res_socket_id = -1, res_length = -1;
    if (sscanf(dataAvailableBuffer.c_str(), "+CIPRXGET: 4,%d,%d", &res_socket_id, &res_length) != 2) {
        GSM_LOG_E("Get Data Available TCP/IP response format error");
        return 0;
    }

    if (res_length > 0) {
        // Read from SIM7600 to queue
        size_t dataLenGet = min(res_length, (int)uxQueueSpacesAvailable(rxQueue));
        uint8_t* buff = (uint8_t*)malloc(dataLenGet);
        int realReadSize = this->_gsm_read(buff, dataLenGet);
        if (realReadSize != dataLenGet) {
            GSM_LOG_E("TCP/IP Read from GSM error size, Read : %d but got %d", dataLenGet, realReadSize);
        }
        for (int i=0;i<realReadSize;i++) {
            if (xQueueSend(rxQueue, &buff[i], 0) != pdTRUE) {
                GSM_LOG_E("Queue is full ?");
                break;
            }
        }
        free(buff);
    }

    return uxQueueMessagesWaiting(rxQueue);
}

int GSMClient::_gsm_read(uint8_t *buf, size_t size) {
    if (this->sock_id != -1) {
        return -1;
    }

    String dataBuffer = "";
    if (!_SIM_Base.sendCommand("AT+CIPRXGET=2," + String(this->sock_id) + "," + String(size), 300)) {
        GSM_LOG_E("Send get Data TCP/IP error");
        return -1; // Timeout
    }

    if (!_SIM_Base.wait("\r\n", 500)) {
        GSM_LOG_E("Wait data reply timeout (1)");
        return -1; // Timeout
    }

    String dataStatusBuffer = "";
    if (!_SIM_Base.readEndsWith(&dataStatusBuffer, 50, "\r\n", 300)) {
        GSM_LOG_I("Wait data reply timeout (2)");
        return -1;
    }

    dataStatusBuffer = dataStatusBuffer.substring(0, dataStatusBuffer.length() - 2);
    GSM_LOG_I("Send command got : %s", dataStatusBuffer.c_str());

    int res_type = 0, res_socket = -1, res_length = 0, res_in_buffer = 0;
    if (sscanf(dataStatusBuffer.c_str(), "+CIPRXGET: %d,%d,%d,%d", &res_type, &res_socket, &res_length, &res_in_buffer) != 4) {
        GSM_LOG_E("Get TCP/IP data format error");
        return -1;
    }

    if (res_length != size) {
        GSM_LOG_E("Data size back error, needs %d but got %d", size, res_length);
        // return -1;
    }

    uint32_t beforeTimeout = this->getTimeout();
    _SIM_Base.setTimeout(3000);

    size_t real_read_length = _SIM_Base.readBytes(buf, res_length);

    _SIM_Base.setTimeout(beforeTimeout);

    if (real_read_length != res_length) {
        GSM_LOG_I("Wait data reply timeout (4)");
        return -1;
    }

    if (!_SIM_Base.wait("\r\n\r\nOK\r\n", 300)) {
        GSM_LOG_I("Wait data reply timeout (5)");
    }

    return res_length;
}

int GSMClient::read() {
    char c;
    if (this->read((uint8_t*)&c, 1) >= 0) {
        return c;
    } else {
        return -1;
    }
}

int GSMClient::read(uint8_t *buf, size_t size) {
    if (this->sock_id != -1) {
        return -1;
    }

    size_t dataWaitRead = uxQueueMessagesWaiting(rxQueue);
    if (dataWaitRead == 0) {
        dataWaitRead = this->available();
    }

    if (dataWaitRead == 0) {
        return -1;
    }

    uint8_t realRead = 0;
	for (int i=0;i<min(dataWaitRead, size);i++) {
		if (xQueueReceive(rxQueue, &buf[i], 0) == pdTRUE) {
            realRead++;
        } else {
            break;
        }
	}

    return realRead;
}

int GSMClient::peek() {
    if (this->sock_id != -1) {
        return -1;
    }

    size_t dataWaitRead = uxQueueMessagesWaiting(rxQueue);
    if (dataWaitRead == 0) {
        dataWaitRead = this->available();
    }

    if (dataWaitRead == 0) {
        return -1;
    }

    uint8_t c;
    if (xQueuePeek(rxQueue, &c, 0) != pdTRUE) {
        return -1;
    }

    return c;
}

void GSMClient::flush() { // Not support
    
}

uint8_t GSMClient::connected() {
    return this->_connected;
}

GSMClient::operator bool() {
    return this->_connected;
}

void GSMClient::stop() {
    if (this->sock_id == -1) {
        return;
    }

    vQueueDelete(rxQueue);

    String disonnectStatusBuffer = "";
    if (!_SIM_Base.sendCommandGetRespondOneLine("AT+CIPCLOSE=" + String(this->sock_id), &disonnectStatusBuffer, 500)) {
        GSM_LOG_E("Send disconnect TCP/IP error");
        return; // Timeout
    }

    int res_socket_id = -1, res_error = -1;
    if (sscanf(disonnectStatusBuffer.c_str(), "+CIPCLOSE: %d,%d", &res_socket_id, &res_error) != 2) {
        GSM_LOG_E("TCP/IP disconnect response format error");
        return;
    }

    if (res_error == 0) {
        GSM_LOG_I("Disconnect TCP/IP socket : %d", this->sock_id);
        this->_connected = false;
        ClientSocketInfo[this->sock_id].itUsing = false;
        this->sock_id = -1;
    } else {
        GSM_LOG_I("Disconnect TCP/IP socket %d error code: %d", this->sock_id, res_error);
    }
}

GSMClient::~GSMClient() {
    this->stop();
}
