#include "SIM76xx.h"
#include "GSM_LOG.h"

SIM76XX::SIM76XX(int rx_pin, int tx_pin, int pwr_pin) {
    this->rx_pin = rx_pin;
    this->tx_pin = tx_pin;
    this->pwr_pin = pwr_pin;
}

bool SIM76XX::begin() {
    pinMode(this->pwr_pin, OUTPUT);
    digitalWrite(this->pwr_pin, LOW);

    static bool init_serial = false;
    if (!init_serial) {
        _SIM_Base.setTimeout(100);
        _SIM_Base.begin(115200, SERIAL_8N1, rx_pin, tx_pin);
        init_serial = true;
        delay(50);
    }

    bool sim_is_ready = false;
    for (int i=0;i<5;i++) {
        GSM_LOG_I("Begin check with AT command (%d)...", i + 1);
        if (this->AT()) {
            GSM_LOG_I("OK, SIM76xx work!");
            sim_is_ready = true;
            break;
        }
        GSM_LOG_I("ERROR, Try again");
        delay(100);
    }

    if (!sim_is_ready) {
        // Turn ON EC-21
        digitalWrite(this->pwr_pin, LOW);
        delay(100);
        digitalWrite(this->pwr_pin, HIGH);
        delay(800);
        digitalWrite(this->pwr_pin, LOW);
        delay(1000); // Wait Boot

        while (_SIM_Base.available()) (void)_SIM_Base.read();

        GSM_LOG_I("Wait Ready... (Max 30s)");
        if (!_SIM_Base.wait("\r\nRDY\r\n", 30 * 1000)) { // Max wait 30s
            GSM_LOG_I("Timeout");
            return false;
        }

        GSM_LOG_I("Ready !");

        GSM_LOG_I("Wait CPIN Ready... (Max 30s)");
        if (!_SIM_Base.wait("\r\n+CPIN: READY\r\n", 5 * 1000)) { // Max wait 5s
            GSM_LOG_I("Timeout");
            return false;
        }
        GSM_LOG_I("CPIN Ready !");

        GSM_LOG_I("Wait SMS Ready... (Max 5s)");
        if (!_SIM_Base.wait("\r\nSMS DONE\r\n", 5 * 1000)) { // Max wait 5s
            GSM_LOG_I("Timeout");
            return false;
        }

        GSM_LOG_I("SMS Ready !");

        GSM_LOG_I("Wait PB Ready... (Max 30s)");
        if (!_SIM_Base.wait("\r\nPB DONE\r\n", 5 * 1000)) { // Max wait 5s
            GSM_LOG_I("Timeout");
            return false;
        }
        GSM_LOG_I("PB Ready !");

        // Test via AT
        GSM_LOG_I("Test AT command (2)...");
        if (!this->AT()) {
            GSM_LOG_I("FAIL, SIM76xx not start");
            return false;
        }
        GSM_LOG_I("OK !");

        sim_is_ready = true;
    }

    if (!sim_is_ready) {
        return false;
    }

    _SIM_Base.URCServiceStart();

    // Configs
    GSM_LOG_I("Network close... ");
    if (!Network.networkClose()) {
        GSM_LOG_I("FAIL");
    } else {
        GSM_LOG_I("OK");
    }

    return true;
}

bool SIM76XX::shutdown() {
    GSM_LOG_I("Send power down...");
    if (!_SIM_Base.sendCommandFindOK("AT+CPOF")) {
        GSM_LOG_I("FAIL, wait try...");
        return false;
    }
    GSM_LOG_I("OK !");

    return true;
}

bool SIM76XX::AT() {
    return _SIM_Base.sendCommandFindOK("AT", 300);
}

String SIM76XX::getIMEI() {
    GSM_LOG_I("Get IMEI...");
    if (!_SIM_Base.sendCommand("AT+CGSN")) {
        GSM_LOG_I("FAIL (1)");
        return "";
    }

    String imeiReadBuffer = "";
    String endOfFind = "\r\n\r\nOK\r\n";
    if (!_SIM_Base.readEndsWith(&imeiReadBuffer, 30, endOfFind, 300)) {
        GSM_LOG_I("FAIL (2), \"%s\"", imeiReadBuffer.c_str());
        return "";
    }

    String imei = imeiReadBuffer.substring(2, imeiReadBuffer.length() - endOfFind.length());
    GSM_LOG_I("OK !, %s", imei.c_str());

    return imei;
}

SIM76XX GSM(16, 13, 21); // Rx, Tx, PWR
