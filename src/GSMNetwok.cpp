#include "GSMNetwok.h"
#include "GSM_LOG.h"

EventGroupHandle_t _gsm_network_flags = NULL;

GSMNetwork::GSMNetwork() {
    if (!_gsm_network_flags) {
        _gsm_network_flags = xEventGroupCreate();
        if (!_gsm_network_flags) {
            GSM_LOG_E("Evant flag of GSM Netwok create fail");
        }
    }
}

#define GSM_NETWORK_STATUS_UPDATE_FLAG               (1 << 0)
#define GSM_NETWORK_OPEN_UPDATE_FLAG                 (1 << 1)
#define GSM_NETWORK_CLOSE_UPDATE_FLAG                (1 << 2)
#define GSM_NETWORK_GET_SIGNAL_SUCCESS_FLAG          (1 << 3)
#define GSM_NETWORK_GET_SIGNAL_FAIL_FLAG             (1 << 4)
#define GSM_NETWORK_GET_CURRENT_CARRIER_SUCCESS_FLAG (1 << 5)
#define GSM_NETWORK_GET_CURRENT_CARRIER_FAIL_FLAG    (1 << 6)
#define GSM_NETWORK_GET_DEVICE_IP_SUCCESS_FLAG       (1 << 7)
#define GSM_NETWORK_GET_DEVICE_IP_FAIL_FLAG          (1 << 8)
#define GSM_NETWORK_PING_IP_SUCCESS_FLAG             (1 << 9)
#define GSM_NETWORK_PING_IP_FAIL_FLAG                (1 << 10)

int _net_status = 0;

bool GSMNetwork::isNetworkOpen() {
    _SIM_Base.URCRegister("+NETOPEN", [](String urcText) {
        _SIM_Base.URCDeregister("+NETOPEN");

        _net_status = 0;
        if (sscanf(urcText.c_str(), "+NETOPEN: %d", &_net_status) != 1) {
            GSM_LOG_E("Get net status format fail");
        }

        xEventGroupSetBits(_gsm_network_flags, GSM_NETWORK_STATUS_UPDATE_FLAG);
    });

    GSM_LOG_I("Check NETOPEN");

    if (!_SIM_Base.sendCommandFindOK("AT+NETOPEN?")) {
        GSM_LOG_E("Get status of NETOPEN error");
        return false;
    }

    EventBits_t flags = xEventGroupWaitBits(_gsm_network_flags, GSM_NETWORK_STATUS_UPDATE_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
    if (flags & GSM_NETWORK_STATUS_UPDATE_FLAG) {
        GSM_LOG_I("NETOPEN status is %d", _net_status);
        return _net_status == 1;
    }

    GSM_LOG_E("Get status of NETOPEN timeout (2)");
    return false;
}

bool net_opened = false;

bool GSMNetwork::networkOpen(uint32_t timeout) {
    if (this->isNetworkOpen()) {
        return true;
    }

    _SIM_Base.URCRegister("+NETOPEN", [](String urcText) {
        _SIM_Base.URCDeregister("+NETOPEN");

        int error = -1;
        if (sscanf(urcText.c_str(), "+NETOPEN: %d", &error) != 1) {
            GSM_LOG_E("Get net open status format fail");
        }

        net_opened = error == 0;
        xEventGroupSetBits(_gsm_network_flags, GSM_NETWORK_OPEN_UPDATE_FLAG);
    });

    GSM_LOG_I("NET OPEN");

    if (!_SIM_Base.sendCommandFindOK("AT+NETOPEN")) {
        GSM_LOG_E("Send NETOPEN error timeout");
        return false;
    }

    EventBits_t flags = xEventGroupWaitBits(_gsm_network_flags, GSM_NETWORK_OPEN_UPDATE_FLAG, pdTRUE, pdFALSE, (timeout + 3000) / portTICK_PERIOD_MS);
    if (flags & GSM_NETWORK_OPEN_UPDATE_FLAG) {
        GSM_LOG_I("NETOPEN is %d", net_opened ? 1 : 0);
        return net_opened;
    }

    GSM_LOG_E("Get status of NETOPEN timeout (2)");
    return false;
}

bool net_closeed = true;

bool GSMNetwork::networkClose() {
    if (!this->isNetworkOpen()) {
        return true;
    }

    _SIM_Base.URCRegister("+NETCLOSE", [](String urcText) {
        _SIM_Base.URCDeregister("+NETCLOSE");

        int error = -1;
        if (sscanf(urcText.c_str(), "+NETCLOSE: %d", &error) != 1) {
            GSM_LOG_E("Get net open status format fail");
        }

        net_closeed = error == 0;
        xEventGroupSetBits(_gsm_network_flags, GSM_NETWORK_CLOSE_UPDATE_FLAG);
    });

    GSM_LOG_I("NET Close");

    if (!_SIM_Base.sendCommandFindOK("AT+NETCLOSE", 3000)) {
        GSM_LOG_E("Send NETCLOSE error timeout");
        return false;
    }

    EventBits_t flags = xEventGroupWaitBits(_gsm_network_flags, GSM_NETWORK_CLOSE_UPDATE_FLAG, pdTRUE, pdFALSE, 3000 / portTICK_PERIOD_MS);
    if (flags & GSM_NETWORK_CLOSE_UPDATE_FLAG) {
        GSM_LOG_I("NET Close is %d", net_closeed ? 1 : 0);
        return net_opened;
    }

    GSM_LOG_E("Get status of NET Close timeout (2)");
    return false;
}

char _network_current_carrier[40];

String GSMNetwork::getCurrentCarrier() {
    xEventGroupClearBits(_gsm_network_flags, GSM_NETWORK_GET_CURRENT_CARRIER_SUCCESS_FLAG | GSM_NETWORK_GET_CURRENT_CARRIER_FAIL_FLAG);
    _SIM_Base.URCRegister("+COPS:", [](String urcText) {
        _SIM_Base.URCDeregister("+COPS:");

        if (sscanf(urcText.c_str(), "+COPS: %*d,%*d,\"%39[^\"]\"", _network_current_carrier) <= 0) {
            GSM_LOG_E("Get current carrier format fail");
            xEventGroupSetBits(_gsm_network_flags, GSM_NETWORK_GET_CURRENT_CARRIER_FAIL_FLAG);
        }

        xEventGroupSetBits(_gsm_network_flags, GSM_NETWORK_GET_CURRENT_CARRIER_SUCCESS_FLAG);
    });

    if (!_SIM_Base.sendCommandFindOK("AT+COPS?")) {
        GSM_LOG_E("Send get current carrier command error");
        return String();
    }

    EventBits_t flags = xEventGroupWaitBits(_gsm_network_flags, GSM_NETWORK_GET_CURRENT_CARRIER_SUCCESS_FLAG | GSM_NETWORK_GET_CURRENT_CARRIER_FAIL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
    if (flags & GSM_NETWORK_GET_CURRENT_CARRIER_SUCCESS_FLAG) {
        GSM_LOG_I("Connected to %s", _network_current_carrier);
        return String(_network_current_carrier);
    } else if (flags & GSM_NETWORK_GET_CURRENT_CARRIER_FAIL_FLAG) {
        GSM_LOG_E("Get current carrier error");
        return String();
    } else {
        GSM_LOG_E("Get current carrier timeout");
        return String();
    }

    return String();
}

int _network_signal = -1;
int GSMNetwork::getSignalStrength() {
    xEventGroupClearBits(_gsm_network_flags, GSM_NETWORK_GET_SIGNAL_SUCCESS_FLAG | GSM_NETWORK_GET_SIGNAL_FAIL_FLAG);
    _SIM_Base.URCRegister("+CSQ:", [](String urcText) {
        _SIM_Base.URCDeregister("+CSQ:");

        if (sscanf(urcText.c_str(), "+CSQ: %d", &_network_signal) != 1) {
            GSM_LOG_E("Get network signal format fail");
            xEventGroupSetBits(_gsm_network_flags, GSM_NETWORK_GET_SIGNAL_FAIL_FLAG);
        }

        xEventGroupSetBits(_gsm_network_flags, GSM_NETWORK_GET_SIGNAL_SUCCESS_FLAG);
    });

    if (!_SIM_Base.sendCommandFindOK("AT+CSQ")) {
        GSM_LOG_E("Send get signal command error");
        return false;
    }

    EventBits_t flags = xEventGroupWaitBits(_gsm_network_flags, GSM_NETWORK_GET_SIGNAL_SUCCESS_FLAG | GSM_NETWORK_GET_SIGNAL_FAIL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
    if (flags & GSM_NETWORK_GET_SIGNAL_SUCCESS_FLAG) {
        GSM_LOG_I("Network Signal is %d", _network_signal);
        return _network_signal;
    } else if (flags & GSM_NETWORK_GET_SIGNAL_FAIL_FLAG) {
        GSM_LOG_E("Get network signal error");
        return -1;
    } else {
        GSM_LOG_E("Get network signal timeout");
        return -1;
    }

    return -1;
}

IPAddress _gsm_device_ip_address((uint32_t)0x00000000);

IPAddress GSMNetwork::getDeviceIP() {
    xEventGroupClearBits(_gsm_network_flags, GSM_NETWORK_GET_DEVICE_IP_SUCCESS_FLAG | GSM_NETWORK_GET_DEVICE_IP_FAIL_FLAG);
    _SIM_Base.URCRegister("+IPADDR:", [](String urcText) {
        _SIM_Base.URCDeregister("+IPADDR:");

        char ip_address[20];
        if (sscanf(urcText.c_str(), "+IPADDR: %20s", ip_address) != 1) {
            GSM_LOG_E("Get ip format fail");
            xEventGroupSetBits(_gsm_network_flags, GSM_NETWORK_GET_DEVICE_IP_FAIL_FLAG);
        }

        _gsm_device_ip_address.fromString(String(ip_address));
        xEventGroupSetBits(_gsm_network_flags, GSM_NETWORK_GET_DEVICE_IP_SUCCESS_FLAG);
    });

    if (!_SIM_Base.sendCommandFindOK("AT+IPADDR")) {
        GSM_LOG_E("Send get ip command error");
        return IPAddress((uint32_t)0x00000000);
    }

    EventBits_t flags = xEventGroupWaitBits(_gsm_network_flags, GSM_NETWORK_GET_DEVICE_IP_SUCCESS_FLAG | GSM_NETWORK_GET_DEVICE_IP_FAIL_FLAG, pdTRUE, pdFALSE, 300 / portTICK_PERIOD_MS);
    if (flags & GSM_NETWORK_GET_DEVICE_IP_SUCCESS_FLAG) {
        GSM_LOG_I("Device IP is %s", _gsm_device_ip_address.toString().c_str());
        return _gsm_device_ip_address;
    } else if (flags & GSM_NETWORK_GET_DEVICE_IP_FAIL_FLAG) {
        GSM_LOG_E("Get ip error");
        return IPAddress((uint32_t)0x00000000);
    } else {
        GSM_LOG_E("Get ip timeout");
        return IPAddress((uint32_t)0x00000000);
    }

    return IPAddress((uint32_t)0x00000000);
}

bool GSMNetwork::pingIP(String host, int timeout) {
    timeout = max(timeout, 10000); // Min 10000 ms

    xEventGroupClearBits(_gsm_network_flags, GSM_NETWORK_PING_IP_SUCCESS_FLAG | GSM_NETWORK_PING_IP_FAIL_FLAG);
    _SIM_Base.URCRegister("+CPING:", [](String urcText) {
        int type = -1;
        if (sscanf(urcText.c_str(), "+CPING: %d", &type) != 1) {
            GSM_LOG_E("Ping format fail (1)");
            xEventGroupSetBits(_gsm_network_flags, GSM_NETWORK_PING_IP_FAIL_FLAG);
        }

        if (type != 3) {
            return;
        }

        _SIM_Base.URCDeregister("+CPING:");

        int recvd, lost;
        if (sscanf(urcText.c_str(), "+CPING: 3,%*d,%d,%d", &recvd, &lost) <= 0) {
            GSM_LOG_E("Ping format fail (2)");
            xEventGroupSetBits(_gsm_network_flags, GSM_NETWORK_PING_IP_FAIL_FLAG);
        }

        if ((recvd == 0) && (lost == 1)) {
            GSM_LOG_E("Ping fail");
            xEventGroupSetBits(_gsm_network_flags, GSM_NETWORK_PING_IP_FAIL_FLAG);
        }

        xEventGroupSetBits(_gsm_network_flags, GSM_NETWORK_PING_IP_SUCCESS_FLAG);
    });

    if (!_SIM_Base.sendCommandFindOK("AT+CPING=\"" + host + "\",1,1,64,1000," + String(timeout), 500)) {
        GSM_LOG_E("Send ping command error");
        return false;
    }

    EventBits_t flags = xEventGroupWaitBits(_gsm_network_flags, GSM_NETWORK_PING_IP_SUCCESS_FLAG | GSM_NETWORK_PING_IP_FAIL_FLAG, pdTRUE, pdFALSE, (timeout + 3000) / portTICK_PERIOD_MS);
    if (flags & GSM_NETWORK_PING_IP_SUCCESS_FLAG) {
        GSM_LOG_I("Ping OK");
        return true;
    } else if (flags & GSM_NETWORK_PING_IP_FAIL_FLAG) {
        GSM_LOG_E("Ping error");
        return false;
    } else {
        GSM_LOG_E("Ping timeout");
        return false;
    }

    return false;
}

GSMNetwork Network;
