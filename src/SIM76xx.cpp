#include "SIM76xx.h"
#include "GSM_LOG.h"

EventGroupHandle_t _sim_general_flags = NULL;

#define SIM_UPDATE_TIME_FROM_NTP_FAIL_FLAG       (1 << 0)
#define SIM_UPDATE_TIME_FROM_NTP_SUCCESS_FLAG    (1 << 1)
#define SIM_GET_TIME_FROM_RTC_FAIL_FLAG          (1 << 2)
#define SIM_GET_TIME_FROM_RTC_SUCCESS_FLAG       (1 << 3)
#define SIM_GET_POWER_MODE_FAIL_FLAG             (1 << 4)
#define SIM_GET_POWER_MODE_SUCCESS_FLAG          (1 << 5)
#define SIM_READY_FLAG                           (1 << 6)
#define SIM_CPIN_READY_FLAG                      (1 << 7)
#define SIM_SMS_DONE_FLAG                        (1 << 8)
#define SIM_PB_DONE_FLAG                         (1 << 9)
#define SIM_GET_GPIO_VALUE_FAIL_FLAG             (1 << 10)
#define SIM_GET_GPIO_VALUE_SUCCESS_FLAG          (1 << 11)
#define SIM_GET_ICCID_FAIL_FLAG                  (1 << 12)
#define SIM_GET_ICCID_SUCCESS_FLAG               (1 << 13)

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
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[this->pwr_pin], PIN_FUNC_GPIO);
    gpio_set_direction(static_cast<gpio_num_t>(this->pwr_pin), GPIO_MODE_OUTPUT);
    gpio_set_level(static_cast<gpio_num_t>(this->pwr_pin), 0);

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
        xEventGroupClearBits(_sim_general_flags, SIM_READY_FLAG | SIM_CPIN_READY_FLAG | SIM_SMS_DONE_FLAG | SIM_PB_DONE_FLAG);

        // Turn ON
        for (int i=0;i<10;i++) {
            gpio_set_level(static_cast<gpio_num_t>(this->pwr_pin), 1);
            delay(100);
            gpio_set_level(static_cast<gpio_num_t>(this->pwr_pin), 0);
            delay(100); 
        }

        while (_SIM_Base.available()) (void)_SIM_Base.read();

        _SIM_Base.URCRegister("RDY", [](String urcText) {
            _SIM_Base.URCDeregister("RDY");

            xEventGroupSetBits(_sim_general_flags, SIM_READY_FLAG);
        });
        _SIM_Base.URCRegister("+CPIN: READY", [](String urcText) {
            _SIM_Base.URCDeregister("+CPIN: READY");

            xEventGroupSetBits(_sim_general_flags, SIM_CPIN_READY_FLAG);
        });
        _SIM_Base.URCRegister("SMS DONE", [](String urcText) {
            _SIM_Base.URCDeregister("SMS DONE");

            xEventGroupSetBits(_sim_general_flags, SIM_SMS_DONE_FLAG);
        });
        _SIM_Base.URCRegister("PB DONE", [](String urcText) {
            _SIM_Base.URCDeregister("PB DONE");

            xEventGroupSetBits(_sim_general_flags, SIM_PB_DONE_FLAG);
        });

        EventBits_t flags = xEventGroupWaitBits(_sim_general_flags, SIM_READY_FLAG | SIM_CPIN_READY_FLAG | SIM_SMS_DONE_FLAG | SIM_PB_DONE_FLAG, pdTRUE, pdTRUE, 30000 / portTICK_PERIOD_MS); // Max 30s
        if ((flags & SIM_READY_FLAG) == 0) {
            GSM_LOG_E("RDY wait timeout");
            sim_is_ready = false;
        } else if ((flags & SIM_CPIN_READY_FLAG) == 0) {
            GSM_LOG_E("+CPIN: READY wait timeout");
            sim_is_ready = false;
        } else if ((flags & SIM_SMS_DONE_FLAG) == 0) {
            GSM_LOG_E("SMS DONE wait timeout");
            sim_is_ready = false;
        } else if ((flags & SIM_PB_DONE_FLAG) == 0) {
            GSM_LOG_E("PB DONE wait timeout");
            sim_is_ready = false;
        } else {
            GSM_LOG_I("Ready !");
            sim_is_ready = true;
        }
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

int _sim_power_mode = 1;
int SIM76XX::_getPowerMode() {
    xEventGroupClearBits(_sim_general_flags, SIM_GET_POWER_MODE_SUCCESS_FLAG | SIM_GET_POWER_MODE_FAIL_FLAG);
    
    _SIM_Base.URCRegister("+CFUN:", [](String urcText) {
        _SIM_Base.URCDeregister("+CFUN:");

        if (sscanf(urcText.c_str(), "+CFUN: %d", &_sim_power_mode) != 1) {
            GSM_LOG_E("+CFUN: Respont format error");
            xEventGroupSetBits(_sim_general_flags, SIM_GET_POWER_MODE_FAIL_FLAG);
            return;
        }

        xEventGroupSetBits(_sim_general_flags, SIM_GET_POWER_MODE_SUCCESS_FLAG);
    });

    if (!_SIM_Base.sendCommandFindOK("AT+CFUN?", 300)) {
      GSM_LOG_I("Get power mode error");
      return -1;
    }

    EventBits_t flags;
    flags = xEventGroupWaitBits(_sim_general_flags, SIM_GET_POWER_MODE_SUCCESS_FLAG | SIM_GET_POWER_MODE_FAIL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
    if (flags & SIM_GET_POWER_MODE_SUCCESS_FLAG) {
        GSM_LOG_I("Power mode is %d", _sim_power_mode);
        return _sim_power_mode;
        return true;
    } else if (flags & SIM_GET_POWER_MODE_FAIL_FLAG) {
        GSM_LOG_E("Get power mode error");
        return -1;
    } else {
        GSM_LOG_E("Send get power mode timeout");
        return -1;
    }

    return -1;
}

int SIM76XX::_setPowerMode(int mode) {
    if (this->_getPowerMode() == mode) {
        return 1;
    }

    if (!_SIM_Base.sendCommandFindOK("AT+CFUN=" + String(mode))) {
        GSM_LOG_E("Send power mode error");
        return 0;
    }

    return 1;
}

int SIM76XX::lowPowerMode() {
    return this->_setPowerMode(0);
}

int SIM76XX::noLowPowerMode() {
    return this->_setPowerMode(1);
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

String SIM76XX::getIMSI() {
    GSM_LOG_I("Get IMSI...");
    String imsi;
    if (!_SIM_Base.sendCommandGetRespondOneLine("AT+CIMIM", &imsi, 300)) {
        GSM_LOG_E("Get IMSI FAIL");
        return "";
    }
    GSM_LOG_I("OK !, %s", imsi.c_str());

    return imsi;
}

String _iccid;
String SIM76XX::getICCID() {
    xEventGroupClearBits(_sim_general_flags, SIM_GET_ICCID_SUCCESS_FLAG | SIM_GET_ICCID_FAIL_FLAG);
    
    _SIM_Base.URCRegister("+ICCID:", [](String urcText) {
        _SIM_Base.URCDeregister("+ICCID:");

        char *iccid_buffer = (char*)malloc(40);
        if (sscanf(urcText.c_str(), "+ICCID: %38s", iccid_buffer) != 1) {
            GSM_LOG_E("+ICCID: Respont format error");
            xEventGroupSetBits(_sim_general_flags, SIM_GET_ICCID_FAIL_FLAG);
            return;
        }

        _iccid = String(iccid_buffer);
        xEventGroupSetBits(_sim_general_flags, SIM_GET_ICCID_SUCCESS_FLAG);
    });

    if (!_SIM_Base.sendCommandFindOK("AT+CICCID", 300)) {
      GSM_LOG_I("Get ICCID error");
      return String();
    }

    EventBits_t flags;
    flags = xEventGroupWaitBits(_sim_general_flags, SIM_GET_ICCID_SUCCESS_FLAG | SIM_GET_ICCID_FAIL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
    if (flags & SIM_GET_ICCID_SUCCESS_FLAG) {
        GSM_LOG_I("ICCID is %s", _iccid.c_str());
    } else if (flags & SIM_GET_ICCID_FAIL_FLAG) {
        GSM_LOG_E("Get ICCID error");
        return String();
    } else {
        GSM_LOG_E("Send get ICCID timeout");
        return String();
    }

    return _iccid;
}

bool SIM76XX::checkGPIOSupport(int pin) {
    if (pin == 3) return true;
    if (pin == 6) return true;
    if (pin == 40) return true;
    if (pin == 41) return true;
    if (pin == 43) return true;
    if (pin == 44) return true;
    if (pin == 77) return true;

    return false;
}

bool SIM76XX::pinMode(int pin, int mode) {
    if (!this->checkGPIOSupport(pin)) {
        return false;
    }

    if ((pin == 3) || (pin == 40) || (pin == 44)) { // Only 3, 40, 44 use AT+CGFUNC=x,0 to set GPIO function
        if (!_SIM_Base.sendCommandFindOK("AT+CGFUNC=" + String(pin) + ",0")) {
            GSM_LOG_E("Send command set GPIO function error timeout");
            return false;
        }
    }

    if (!_SIM_Base.sendCommandFindOK("AT+CGDRT=" + String(pin) + "," + String(mode == OUTPUT ? 1 : 0))) {
        GSM_LOG_E("Send command set GPIO direction error timeout");
        return false;
    }

    return true;
}

bool SIM76XX::digitalWrite(int pin, int value) {
    if (!this->checkGPIOSupport(pin)) {
        return false;
    }

    if (!_SIM_Base.sendCommandFindOK("AT+CGSETV=" + String(pin) + "," + String(value == HIGH ? 1 : 0))) {
        GSM_LOG_E("Send command set GPIO direction error timeout");
        return false;
    }

    return true;
}

int _gpio_focus = 0, _gpio_value = 0;

int SIM76XX::digitalRead(int pin) {
    if (!this->checkGPIOSupport(pin)) {
        return 0;
    }

    _gpio_focus = pin;

    xEventGroupClearBits(_sim_general_flags, SIM_GET_GPIO_VALUE_SUCCESS_FLAG | SIM_GET_GPIO_VALUE_FAIL_FLAG);
    
    _SIM_Base.URCRegister("+CGGETV:", [](String urcText) {
        _SIM_Base.URCDeregister("+CGGETV:");

        int gpio = 0;
        if (sscanf(urcText.c_str(), "+CGGETV: %d,%d", &gpio, &_gpio_value) != 2) {
            GSM_LOG_E("+CGGETV: Respont format error");
            xEventGroupSetBits(_sim_general_flags, SIM_GET_GPIO_VALUE_FAIL_FLAG);
            return;
        }

        if (gpio != _gpio_focus) {
            GSM_LOG_E("GPIO focus and GPIO value return is diff, Focus %d but got %d", _gpio_focus, gpio);
            xEventGroupSetBits(_sim_general_flags, SIM_GET_GPIO_VALUE_FAIL_FLAG);
            return;
        }

        xEventGroupSetBits(_sim_general_flags, SIM_GET_GPIO_VALUE_SUCCESS_FLAG);
    });

    if (!_SIM_Base.sendCommandFindOK("AT+CGGETV=" + String(pin), 300)) {
      GSM_LOG_I("Get GPIO value error timeout");
      return 0;
    }

    EventBits_t flags;
    flags = xEventGroupWaitBits(_sim_general_flags, SIM_GET_GPIO_VALUE_SUCCESS_FLAG | SIM_GET_GPIO_VALUE_FAIL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
    if (flags & SIM_GET_GPIO_VALUE_SUCCESS_FLAG) {
        GSM_LOG_I("GPIO %d value is %d", pin, _gpio_value);
        return _gpio_value;
    } else if (flags & SIM_GET_GPIO_VALUE_FAIL_FLAG) {
        GSM_LOG_E("Get GPIO value error");
        return 0;
    } else {
        GSM_LOG_E("Send GPIO value timeout");
        return 0;
    }

    return 0;
}

SIM76XX GSM(14, 13, 12); // Rx, Tx, PWR
