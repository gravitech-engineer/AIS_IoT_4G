#include "SHT40.h"

SHT40Class::SHT40Class(TwoWire *wire) {
    this->wire = wire;
}

void SHT40Class::begin(uint8_t address) {
    this->dev_addr = address;
}

bool SHT40Class::read() {
    this->wire->beginTransmission(this->dev_addr);
    this->wire->write(0xFD);
    this->wire->endTransmission();

    delay(10);

    int n = this->wire->requestFrom((int)this->dev_addr, (int)6);
    if (n != 6) {
        return false;
    }

    uint8_t rx_bytes[6];
    this->wire->readBytes(rx_bytes, 6);

    uint16_t t_ticks = rx_bytes[0] * 256 + rx_bytes[1];
    (void)rx_bytes[2];
    uint16_t rh_ticks = rx_bytes[3] * 256 + rx_bytes[4];
    (void)rx_bytes[5];
    this->t_degC = -45.0f + 175.0f * t_ticks / 65535.0f;
    this->rh_pRH = -6.0f + 125.0f * rh_ticks / 65535.0f;

    this->rh_pRH = max(min(rh_pRH, 100.0f), 0.0f);

    return true;
}

float SHT40Class::readTemperature(int units) {
    if (!this->read()) {
        return -99;
    }

    return units == CELSIUS ? this->t_degC : (this->t_degC * 9.0 / 5.0 + 32.0);
}

float SHT40Class::readHumidity() {
    if (!this->read()) {
        return -99;
    }

    return this->rh_pRH;
}

void SHT40Class::end() {
    this->wire = NULL;
    this->dev_addr = 0;
}

SHT40Class SHT40(&Wire);
