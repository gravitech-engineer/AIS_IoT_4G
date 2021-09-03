#include "RS485..h"

RS485Class::RS485Class(int n) : HardwareSerial(n) {
    // Not use
}

void RS485Class::begin(unsigned long baudrate, uint32_t config, int rx_pin, int tx_pin, int dir_pin) {
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

RS485Class RS485(3); // Serial3,
