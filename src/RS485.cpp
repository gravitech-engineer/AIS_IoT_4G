#include "RS485..h"

RS485Class::RS485Class(int n) : HardwareSerial(n) {
    // Not use
}

void RS485Class::begin(unsigned long baudrate, uint32_t config, int rx_pin, int tx_pin, int dir_pin) {
    HardwareSerial::setTimeout(1000); // Max time of Respond is 1000 mS
    HardwareSerial::begin(baudrate, config, rx_pin, tx_pin);

    pinMode(dir_pin, OUTPUT);
    this->dir_pin = dir_pin;
}

void RS485Class::beginTransmission() {
    this->noReceive();
}

void RS485Class::endTransmission() {
    HardwareSerial::flush(); // Wait send all data
    this->receive();
}

void RS485Class::receive() {
    digitalWrite(dir_pin, LOW); // Active ^RE pin
}

void RS485Class::noReceive() {
    digitalWrite(dir_pin, HIGH); // Active DE pin
}

int RS485Class::coilRead(int address) {
    return this->coilRead(0, address);
}

int RS485Class::coilRead(int id, int address) {
    uint8_t buff_send[8] = {
        (uint8_t)(id & 0xFF),           // Devices Address
        0x01,                           // Function code
        (uint8_t)(address >> 8),        // Start Address HIGH
        (uint8_t)(address & 0xFF),      // Start Address LOW
        0x00,                           // Quantity of coils HIGH
        0x01,                           // Quantity of coils LOW
        0,                              // CRC LOW
        0                               // CRC HIGH
    };

    uint16_t crc = CRC16(&buff_send[0], 6);
    buff_send[6] = crc & 0xFF;
    buff_send[7] = (crc >> 8) & 0xFF;

    this->beginTransmission();
    HardwareSerial::write(buff_send, 8);
    this->endTransmission();

    delay(10); // Wait reply

    uint8_t buff_reply[6];
    if (HardwareSerial::readBytes(&buff_reply[0], 2) != 2) { // Read first byte
        return -1; // timeout
    }

    if ((id != 0) && (buff_reply[0] != id)) {
        return -1; // reply device id invalid
    }

    if (buff_reply[1] != 0x01) { // Recheck function code
        return -1; // Function code is wrong
    }

    if (HardwareSerial::readBytes(&buff_reply[2], 4) != 4) { // Read other byte
        return -1; // timeout
    }

    if (buff_reply[2] != 1) { // Byte Count => only 1
        return -1; // Byte Count invalid
    }

    uint8_t coil_value = buff_reply[3] & 0x80;

    uint16_t crc_recheck = ((uint16_t)buff_reply[5] << 8) | buff_reply[4];
    if (crc_recheck != CRC16(buff_reply, 4)) { // Check CRC
        return -1; // CRC invalid
    }

    return coil_value;
}

int RS485Class::discreteInputRead(int address) {
    return this->discreteInputRead(0, address);
}

int RS485Class::discreteInputRead(int id, int address) {
    uint8_t buff_send[8] = {
        (uint8_t)(id & 0xFF),           // Devices Address
        0x02,                           // Function code
        (uint8_t)(address >> 8),        // Start Address HIGH
        (uint8_t)(address & 0xFF),      // Start Address LOW
        0x00,                           // Quantity of coils HIGH
        0x01,                           // Quantity of coils LOW
        0,                              // CRC LOW
        0                               // CRC HIGH
    };

    uint16_t crc = CRC16(&buff_send[0], 6);
    buff_send[6] = crc & 0xFF;
    buff_send[7] = (crc >> 8) & 0xFF;

    this->beginTransmission();
    HardwareSerial::write(buff_send, 8);
    this->endTransmission();

    delay(10); // Wait reply

    uint8_t buff_reply[6];
    if (HardwareSerial::readBytes(&buff_reply[0], 2) != 2) { // Read first byte
        return -1; // timeout
    }

    if ((id != 0) && (buff_reply[0] != id)) {
        return -1; // reply device id invalid
    }

    if (buff_reply[1] != 0x02) { // Recheck function code
        return -1; // Function code is wrong
    }

    if (HardwareSerial::readBytes(&buff_reply[2], 4) != 4) { // Read other byte
        return -1; // timeout
    }

    if (buff_reply[2] != 1) { // Byte Count => only 1
        return -1; // Byte Count invalid
    }

    uint8_t input_status = buff_reply[3] & 0x80;

    uint16_t crc_recheck = ((uint16_t)buff_reply[5] << 8) | buff_reply[4];
    if (crc_recheck != CRC16(buff_reply, 4)) { // Check CRC
        return -1; // CRC invalid
    }

    return input_status;
}

long RS485Class::holdingRegisterRead(int address) {
    return this->holdingRegisterRead(0, address);
}

long RS485Class::holdingRegisterRead(int id, int address) {
    uint8_t buff_send[8] = {
        (uint8_t)(id & 0xFF),           // Devices Address
        0x03,                           // Function code
        (uint8_t)(address >> 8),        // Start Address HIGH
        (uint8_t)(address & 0xFF),      // Start Address LOW
        0x00,                           // Quantity of coils HIGH
        0x01,                           // Quantity of coils LOW
        0,                              // CRC LOW
        0                               // CRC HIGH
    };

    uint16_t crc = CRC16(&buff_send[0], 6);
    buff_send[6] = crc & 0xFF;
    buff_send[7] = (crc >> 8) & 0xFF;

    this->beginTransmission();
    HardwareSerial::write(buff_send, 8);
    this->endTransmission();

    delay(10); // Wait reply

    uint8_t buff_reply[7];
    if (HardwareSerial::readBytes(&buff_reply[0], 2) != 2) { // Read first byte
        return -1; // timeout
    }

    if ((id != 0) && (buff_reply[0] != id)) {
        return -1; // reply device id invalid
    }

    if (buff_reply[1] != 0x03) { // Recheck function code
        return -1; // Function code is wrong
    }

    if (HardwareSerial::readBytes(&buff_reply[2], 5) != 5) { // Read other byte
        return -1; // timeout
    }

    if (buff_reply[2] != 2) { // Byte Count => only 2
        return -1; // Byte Count invalid
    }

    uint16_t register_value = ((uint16_t)buff_reply[3] << 8) | buff_reply[4];

    uint16_t crc_recheck = ((uint16_t)buff_reply[6] << 8) | buff_reply[5];
    if (crc_recheck != CRC16(buff_reply, 5)) { // Check CRC
        return -1; // CRC invalid
    }

    return register_value;
}

long RS485Class::inputRegisterRead(int address) {
    return this->inputRegisterRead(0, address);
}

long RS485Class::inputRegisterRead(int id, int address) {
    uint8_t buff_send[8] = {
        (uint8_t)(id & 0xFF),           // Devices Address
        0x04,                           // Function code
        (uint8_t)(address >> 8),        // Start Address HIGH
        (uint8_t)(address & 0xFF),      // Start Address LOW
        0x00,                           // Quantity of coils HIGH
        0x01,                           // Quantity of coils LOW
        0,                              // CRC LOW
        0                               // CRC HIGH
    };

    uint16_t crc = CRC16(&buff_send[0], 6);
    buff_send[6] = crc & 0xFF;
    buff_send[7] = (crc >> 8) & 0xFF;

    this->beginTransmission();
    HardwareSerial::write(buff_send, 8);
    this->endTransmission();

    delay(10); // Wait reply

    uint8_t buff_reply[7];
    if (HardwareSerial::readBytes(&buff_reply[0], 2) != 2) { // Read first byte
        return -1; // timeout
    }

    if ((id != 0) && (buff_reply[0] != id)) {
        return -1; // reply device id invalid
    }

    if (buff_reply[1] != 0x04) { // Recheck function code
        return -1; // Function code is wrong
    }

    if (HardwareSerial::readBytes(&buff_reply[2], 5) != 5) { // Read other byte
        return -1; // timeout
    }

    if (buff_reply[2] != 2) { // Byte Count => only 2
        return -1; // Byte Count invalid
    }

    uint16_t register_value = ((uint16_t)buff_reply[3] << 8) | buff_reply[4];

    uint16_t crc_recheck = ((uint16_t)buff_reply[6] << 8) | buff_reply[5];
    if (crc_recheck != CRC16(buff_reply, 5)) { // Check CRC
        return -1; // CRC invalid
    }

    return register_value;
}

int RS485Class::coilWrite(int address, uint8_t value) {
    return this->coilWrite(0, address, value);
}

int RS485Class::coilWrite(int id, int address, uint8_t value) {
    uint8_t buff_send[8] = {
        (uint8_t)id,                    // Devices Address
        0x05,                           // Function code
        (uint8_t)(address >> 8),        // Start Address HIGH
        (uint8_t)(address & 0xFF),      // Start Address LOW
        value ? (uint8_t)0xFF : (uint8_t)0x00, // Data 1 HIGH
        0x00,                           // Data 1 LOW
        0,                              // CRC LOW
        0                               // CRC HIGH
    };

    uint16_t crc = CRC16(&buff_send[0], 6);
    buff_send[6] = crc & 0xFF;
    buff_send[7] = (crc >> 8) & 0xFF;

    this->beginTransmission();
    HardwareSerial::write(buff_send, 8);
    this->endTransmission();

    delay(10); // Wait reply

    uint8_t buff_reply[8];
    if (HardwareSerial::readBytes(&buff_reply[0], 2) != 2) { // Read first byte
        return 0; // timeout
    }

    if ((id != 0) && (buff_reply[0] != id)) {
        return 0; // reply device id invalid
    }

    if (buff_reply[1] != 0x05) { // Recheck function code
        return 0; // Function code is wrong
    }

    if (HardwareSerial::readBytes(&buff_reply[2], 6) != 6) { // Read other byte
        return 0; // timeout
    }

    if (memcmp(&buff_send[2], &buff_reply[2], 6) != 0) { // Check Send != Reply
        return 0; // Reply invalid
    }

    return 1;
}

int RS485Class::holdingRegisterWrite(int address, uint16_t value) {
    return this->holdingRegisterWrite(0, address, value);
}

int RS485Class::holdingRegisterWrite(int id, int address, uint16_t value) {
    uint8_t buff_send[8] = {
        (uint8_t)id,                    // Devices Address
        0x06,                           // Function code
        (uint8_t)(address >> 8),        // Start Address HIGH
        (uint8_t)(address & 0xFF),      // Start Address LOW
        (uint8_t)(value >> 8),          // Data 1 HIGH
        (uint8_t)(value & 0xFF),        // Data 1 LOW
        0,                              // CRC LOW
        0                               // CRC HIGH
    };

    uint16_t crc = CRC16(&buff_send[0], 6);
    buff_send[6] = crc & 0xFF;
    buff_send[7] = (crc >> 8) & 0xFF;

    this->beginTransmission();
    HardwareSerial::write(buff_send, 8);
    this->endTransmission();

    delay(10); // Wait reply

    uint8_t buff_reply[8];
    if (HardwareSerial::readBytes(&buff_reply[0], 2) != 2) { // Read first byte
        return 0; // timeout
    }

    if ((id != 0) && (buff_reply[0] != id)) {
        return 0; // reply device id invalid
    }

    if (buff_reply[1] != 0x06) { // Recheck function code
        return 0; // Function code is wrong
    }

    if (HardwareSerial::readBytes(&buff_reply[2], 6) != 6) { // Read other byte
        return 0; // timeout
    }

    if (memcmp(&buff_send[2], &buff_reply[2], 6) != 0) { // Check Send != Reply
        return 0; // Reply invalid
    }

    return 1;
}

uint16_t RS485Class::CRC16(uint8_t *buf, int len) {  
  uint16_t crc = 0xFFFF;
  for (uint16_t pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)buf[pos];        // XOR byte into least sig. byte of crc
    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      } else {                        // Else LSB is not set
        crc >>= 1;                    // Just shift right
      }
    }
  }

  return crc;
}

RS485Class RS485(3); // Serial3,
