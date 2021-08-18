#include "SIM76xx.h"
#include "GSM_LOG.h"

EventGroupHandle_t _sim_general_flags = NULL;

#define SIM_UPDATE_TIME_FROM_NTP_FAIL_FLAG       (1 << 0)
#define SIM_UPDATE_TIME_FROM_NTP_SUCCESS_FLAG    (1 << 1)
#define SIM_GET_TIME_FROM_RTC_FAIL_FLAG          (1 << 2)
#define SIM_GET_TIME_FROM_RTC_SUCCESS_FLAG       (1 << 3)

SIM76XX::SIM76XX(int rx_pin, int tx_pin, int pwr_pin) {
    this->rx_pin = rx_pin;
    this->tx_pin = tx_pin;
    this->pwr_pin = pwr_pin;

    if (!_sim_general_flags) {
        _sim_general_flags = xEventGroupCreate();
        if (!_sim_general_flags) {
            GSM_LOG_E("Evant flag of SIM general create fail");
        }
    }
}

bool SIM76XX::begin() {
    pinMode(this->pwr_pin, OUTPUT);
    digitalWrite(this->pwr_pin, LOW);

    static bool init_serial = false;
    if (!init_serial) {
        _SIM_Base.setTimeout(100);
        _SIM_Base.begin(115200, SERIAL_8N1, rx_pin, tx_pin);
        _SIM_Base.URCServiceStart();
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

    // Configs
    GSM_LOG_I("Network close... ");
    if (!Network.networkClose()) {
        GSM_LOG_I("FAIL");
    } else {
        GSM_LOG_I("OK");
    }

    GSM_LOG_I("Network Open again... ");
    if (!Network.networkOpen()) {
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

struct tm _sim_rtc;
int _sim_rtc_timezone = 7;

unsigned long SIM76XX::_getTimeFromSIM(String ntp_server) {
    GSM_LOG_I("Config NTP server...");
    if (!_SIM_Base.sendCommandFindOK("AT+CNTP=\"" + ntp_server + "\",28", 300)) {
      GSM_LOG_E("FAIL, wait try...");
      return 0;
    }
    GSM_LOG_I("OK !");

    xEventGroupClearBits(_sim_general_flags, SIM_UPDATE_TIME_FROM_NTP_SUCCESS_FLAG | SIM_UPDATE_TIME_FROM_NTP_FAIL_FLAG);
    
    _SIM_Base.URCRegister("+CNTP:", [](String urcText) {
        _SIM_Base.URCDeregister("+CNTP");
            
        int error = -1;
        if (sscanf(urcText.c_str(), "+CNTP: %d", &error) != 1) {
            GSM_LOG_E("+CNTP: Respont format error");
            xEventGroupSetBits(_sim_general_flags, SIM_UPDATE_TIME_FROM_NTP_FAIL_FLAG);
            return;
        }

        if (error != 0) {
            GSM_LOG_E("Update time from NTP error code : %d", error);
            xEventGroupSetBits(_sim_general_flags, SIM_UPDATE_TIME_FROM_NTP_FAIL_FLAG);
            return;
        }

        xEventGroupSetBits(_sim_general_flags, SIM_UPDATE_TIME_FROM_NTP_SUCCESS_FLAG);
    });

    GSM_LOG_V("Update time from NTP server...");
    if (!_SIM_Base.sendCommandFindOK("AT+CNTP", 10000)) {
      GSM_LOG_I("Send command error");
      return 0;
    }

    EventBits_t flags;
    flags = xEventGroupWaitBits(_sim_general_flags, SIM_UPDATE_TIME_FROM_NTP_SUCCESS_FLAG | SIM_UPDATE_TIME_FROM_NTP_FAIL_FLAG, pdTRUE, pdFALSE, 10000 / portTICK_PERIOD_MS);
    if (flags & SIM_UPDATE_TIME_FROM_NTP_SUCCESS_FLAG) {
        GSM_LOG_I("Send update time from NTP OK");
    } else if (flags & SIM_UPDATE_TIME_FROM_NTP_FAIL_FLAG) {
        GSM_LOG_E("Send update time from NTP error");
        return 0;
    } else {
        GSM_LOG_E("Send update time from NTP timeout");
        return 0;
    }

    GSM_LOG_V("Get datetime from RTC...");
    _SIM_Base.URCRegister("+CCLK:", [](String urcText) {
        _SIM_Base.URCDeregister("+CCLK");
            
        if (sscanf(urcText.c_str(), "+CCLK: \"%2d/%2d/%2d,%2d:%2d:%2d%3d\"", &_sim_rtc.tm_year, &_sim_rtc.tm_mon, &_sim_rtc.tm_mday, &_sim_rtc.tm_hour, &_sim_rtc.tm_min, &_sim_rtc.tm_sec, &_sim_rtc_timezone) != 7) {
            GSM_LOG_E("+CCLK: Respont format error");
            xEventGroupSetBits(_sim_general_flags, SIM_GET_TIME_FROM_RTC_FAIL_FLAG);
            return;
        }

        _sim_rtc.tm_year = (_sim_rtc.tm_year + 2000) - 1900;
        _sim_rtc_timezone = _sim_rtc_timezone / 4;

        xEventGroupSetBits(_sim_general_flags, SIM_GET_TIME_FROM_RTC_SUCCESS_FLAG);
    });

    xEventGroupClearBits(_sim_general_flags, SIM_GET_TIME_FROM_RTC_SUCCESS_FLAG | SIM_GET_TIME_FROM_RTC_FAIL_FLAG);

    if (!_SIM_Base.sendCommandFindOK("AT+CCLK?", 500)) {
      GSM_LOG_E("Send command get time from RTC error");
      return 0;
    }

    flags = xEventGroupWaitBits(_sim_general_flags, SIM_GET_TIME_FROM_RTC_SUCCESS_FLAG | SIM_GET_TIME_FROM_RTC_FAIL_FLAG, pdTRUE, pdFALSE, 500 / portTICK_PERIOD_MS);
    if (flags & SIM_GET_TIME_FROM_RTC_SUCCESS_FLAG) {
        GSM_LOG_I("Get time from RTC OK");
    } else if (flags & SIM_GET_TIME_FROM_RTC_FAIL_FLAG) {
        GSM_LOG_E("Get time from RTC error");
        return 0;
    } else {
        GSM_LOG_E("Get time from RTC timeout");
        return 0;
    }

    return mktime(&_sim_rtc);
}

unsigned long SIM76XX::getTime(String ntp_server) {
    unsigned long t = getLocalTime(ntp_server);
    if (t > 0) {
        t -= _sim_rtc_timezone * 60 * 60; // +7 hour to 0 (GMT)
    }
    return t;
}

unsigned long SIM76XX::getLocalTime(String ntp_server) {
    unsigned long t = 0;
    for (int i=0;i<5;i++) {
        t = _getTimeFromSIM(ntp_server);
        if (t > 0) {
            break;
        }
    }

    return t;
}

int SIM76XX::lowPowerMode() { // Not support
    return 0;
}

int SIM76XX::noLowPowerMode() { // Not support
    return 0;
}

bool SIM76XX::AT() {
    return _SIM_Base.sendCommandFindOK("AT", 300);
}

String SIM76XX::getIMEI() {
    GSM_LOG_I("Get IMEI...");
    String imei;
    if (!_SIM_Base.sendCommandGetRespondOneLine("AT+CGSN", &imei, 300)) {
        GSM_LOG_E("Get IMEI FAIL");
        return "";
    }
    GSM_LOG_I("OK !, %s", imei.c_str());

    return imei;
}

SIM76XX GSM(16, 13, 21); // Rx, Tx, PWR
