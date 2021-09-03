#include "GSMUdp.h"
#include "GSMSocket.h"

GSMUdp::GSMUdp() {
    
}

uint8_t GSMUdp::begin(uint16_t) {
    return 0;
}

uint8_t GSMUdp::beginMulticast(IPAddress ip, uint16_t port) {
    return 0;
}

int GSMUdp::beginPacket(const char *host, uint16_t port) {
    if (this->write_buff) {
        free(this->write_buff);
        this->write_buff = NULL;
    }

    this->write_buff = (uint8_t*)malloc(UDP_WRITE_BUFFER);

    size_t host_len = strlen(host);
    this->write_host = (char*)malloc(host_len + 1);
    memcpy(this->write_host, host, host_len);
    this->write_host[host_len] = 0; // END of String

    this->write_port = port;

    return 0;
}

int GSMUdp::endPacket() {
    // AT+CIPSEND=<link_num>,<length>,<serverIP>,<serverPort>

    free(this->write_buff);
    this->write_buff = NULL;

    return 0;
}

size_t GSMUdp::write(uint8_t c) {
    return this->write(&c, 1);
}

size_t GSMUdp::write(const uint8_t *buffer, size_t size) {
    uint16_t copy_len = min(this->write_len, size);
    memcpy(&this->write_buff[write_len], buffer, copy_len);
    this->write_len += copy_len;

    return copy_len;
}

int GSMUdp::parsePacket() {
    return -1;
}

int GSMUdp::available() {
    return -1;
}

int GSMUdp::read() {
    return -1;
}

int GSMUdp::read(unsigned char* buffer, size_t len) {
    return -1;
}

int GSMUdp::read(char* buffer, size_t len) {
    return -1;
}

int GSMUdp::peek() {
    return -1;
}

void GSMUdp::flush() {

}

IPAddress GSMUdp::remoteIP() {
    return IPAddress((uint32_t) 0x0);
}

uint16_t GSMUdp::remotePort() {
    return 0;
}
