#include "GPS.h"
#include "SIMBase.h"
#include "GSM_LOG.h"

EventGroupHandle_t _gps_flags = NULL;

GPSClass::GPSClass() {
    if (!_gps_flags) {
        _gps_flags = xEventGroupCreate();
        if (!_gps_flags) {
            GSM_LOG_E("Evant flag of GPS create fail");
        }
    }
}

#define GPS_STATUS_UPDATE_SUCCESS_FLAG   (1 << 0)
#define GPS_STATUS_UPDATE_FALL_FLAG       (1 << 1)
#define GPS_LOCATION_UPDATE_SUCCESS_FLAG (1 << 2)
#define GPS_LOCATION_UPDATE_FALL_FLAG    (1 << 3)

int _gps_status = 0;

bool GPSClass::isOn() {
    xEventGroupClearBits(_gps_flags, GPS_STATUS_UPDATE_SUCCESS_FLAG | GPS_STATUS_UPDATE_FALL_FLAG);
    _SIM_Base.URCRegister("+CGPS", [](String urcText) {
        _SIM_Base.URCDeregister("+CGPS");

        _gps_status = -1;
        if (sscanf(urcText.c_str(), "+CGPS: %d", &_gps_status) != 1) {
            GSM_LOG_E("Get GPS status format fail");
            xEventGroupSetBits(_gps_flags, GPS_STATUS_UPDATE_FALL_FLAG);
            return;
        }

        xEventGroupSetBits(_gps_flags, GPS_STATUS_UPDATE_SUCCESS_FLAG);
    });

    GSM_LOG_I("Check GPS is ON");

    if (!_SIM_Base.sendCommandFindOK("AT+CGPS?")) {
        GSM_LOG_E("Get status of GPS error");
        return false;
    }

    EventBits_t flags = xEventGroupWaitBits(_gps_flags, GPS_STATUS_UPDATE_SUCCESS_FLAG | GPS_STATUS_UPDATE_FALL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
    if (flags & GPS_STATUS_UPDATE_SUCCESS_FLAG) {
        GSM_LOG_I("GPS status is %d", _gps_status);
        return _gps_status == 1;
    } else if (flags & GPS_STATUS_UPDATE_FALL_FLAG) {
        GSM_LOG_E("GPS get status fail");
        return false;
    } else {
        GSM_LOG_E("GPS get status timeout");
        return false;
    }

    return false;
}

int GPSClass::begin() {
    return this->wakeup() ? 1 : 0;
}

float _gps_lat, _gps_lng, _gps_alt, _gps_speed, _gps_course;
unsigned long _gps_timeUTC;
int GPSClass::available() {
    xEventGroupClearBits(_gps_flags, GPS_LOCATION_UPDATE_SUCCESS_FLAG | GPS_LOCATION_UPDATE_FALL_FLAG);
    _SIM_Base.URCRegister("+CGPSINFO", [](String urcText) {
        _SIM_Base.URCDeregister("+CGPSINFO");

        char ns = 0, we = 0;
        struct tm time;

        _gps_lat = 0;
        _gps_lng = 0;
        _gps_alt = 0;
        _gps_speed = 0;
        _gps_course = 0;

        if (sscanf(urcText.c_str(), "+CGPSINFO: %f,%c,%f,%c,%2d%2d%2d,%2d%2d%2d.%*c,%f,%f,%f", &_gps_lat, &ns, &_gps_lng, &we, &time.tm_mday, &time.tm_mon, &time.tm_year, &time.tm_hour, &time.tm_min, &time.tm_sec, &_gps_alt, &_gps_speed, &_gps_course) <= 0) {
            GSM_LOG_E("Get location format fail");
            xEventGroupSetBits(_gps_flags, GPS_LOCATION_UPDATE_FALL_FLAG);
            return;
        }

        // return;

        int lat_deg = _gps_lat / 100;
        float lat_min = _gps_lat - (lat_deg * 100.0);
        int lng_deg = _gps_lng / 100;
        float lng_min = _gps_lng - (lng_deg * 100.0);

        _gps_lat = lat_deg + (lat_min / 60.0) * (ns == 'N' ? 1.0 : -1.0);
        _gps_lng = lng_deg + (lng_min / 60.0) * (we == 'E' ? 1.0 : -1.0);
        
        time.tm_year = (time.tm_year + 2000) - 1900;
        _gps_timeUTC = mktime(&time);

        xEventGroupSetBits(_gps_flags, GPS_LOCATION_UPDATE_SUCCESS_FLAG);
    });

    if (!_SIM_Base.sendCommand("AT+CGPSINFO")) {
        GSM_LOG_E("GPS get location send command timeout");
        return false;
    }

    EventBits_t flags = xEventGroupWaitBits(_gps_flags, GPS_LOCATION_UPDATE_SUCCESS_FLAG | GPS_STATUS_UPDATE_FALL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
    
    if (_SIM_Base.waitOKorERROR(100) != 1) {
        GSM_LOG_I("GPS get location not found OK");
        return false;
    }

    if (flags & GPS_LOCATION_UPDATE_SUCCESS_FLAG) {
        GSM_LOG_I("GPS get location OK");
        return true;
    } else if (flags & GPS_STATUS_UPDATE_FALL_FLAG) {
        GSM_LOG_E("GPS get location fail");
        return false;
    } else {
        GSM_LOG_E("GPS get location timeout");
        return false;
    }

    return false;
}

float GPSClass::latitude(){
    return _gps_lat;
}

float GPSClass::longitude() {
    return _gps_lng;
}

float GPSClass::speed() {
    return _gps_speed;
}

float GPSClass::course() {
    return _gps_course;
}

float GPSClass::variation() {
    return 0; // Not support
}

float GPSClass::altitude() {
    return _gps_alt;
}

int GPSClass::satellites() {
    return 0; // Not support
}

unsigned long GPSClass::getTime(){
    return _gps_timeUTC;
}

bool GPSClass::standby(){
    if (!this->isOn()) {
        return true;
    }

    xEventGroupClearBits(_gps_flags, GPS_STATUS_UPDATE_SUCCESS_FLAG | GPS_STATUS_UPDATE_FALL_FLAG);
    _SIM_Base.URCRegister("+CGPS", [](String urcText) {
        _SIM_Base.URCDeregister("+CGPS");

        _gps_status = -1;
        if (sscanf(urcText.c_str(), "+CGPS: %d", &_gps_status) != 1) {
            GSM_LOG_E("Get GPS status format fail");
            xEventGroupSetBits(_gps_flags, GPS_STATUS_UPDATE_FALL_FLAG);
            return;
        }

        if (_gps_status != 0) {
            GSM_LOG_E("GPS Turn OFF fail, code: %d", _gps_status);
            xEventGroupSetBits(_gps_flags, GPS_STATUS_UPDATE_FALL_FLAG);
            return;
        }

        xEventGroupSetBits(_gps_flags, GPS_STATUS_UPDATE_SUCCESS_FLAG);
    });

    if (!_SIM_Base.sendCommandFindOK("AT+CGPS=0")) {
        GSM_LOG_E("Turn OFF GPS error");
        return false;
    }

    EventBits_t flags = xEventGroupWaitBits(_gps_flags, GPS_STATUS_UPDATE_SUCCESS_FLAG | GPS_STATUS_UPDATE_FALL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
    if (flags & GPS_STATUS_UPDATE_SUCCESS_FLAG) {
        GSM_LOG_I("GPS Trun OFF OK");
        return true;
    } else if (flags & GPS_STATUS_UPDATE_FALL_FLAG) {
        GSM_LOG_E("GPS Turn OFF fail");
        return false;
    } else {
        GSM_LOG_E("GPS get status of trun off timeout");
        return false;
    }

    return false;
}

bool GPSClass::wakeup() {
    if (this->isOn()) {
        return true;
    }

    if (!_SIM_Base.sendCommandFindOK("AT+CGPS=1")) {
        GSM_LOG_E("Turn ON GPS error");
        return false;
    }

    GSM_LOG_I("GPS Turn ON OK !");
    return true;
}

GPSClass GPS;