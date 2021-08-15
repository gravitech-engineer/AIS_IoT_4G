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

#define GSM_NETWORK_STATUS_UPDATE_FLAG  (1 << 0)

int _net_status = 0;

bool GSMNetwork::isNetworkOpen() {
    _SIM_Base.URCRegister("+NETOPEN", [](String getNetStatusBuff) {
        _SIM_Base.URCDeregister("+NETOPEN");

        _net_status = 0;
        if (sscanf(getNetStatusBuff.c_str(), "+NETOPEN: %d", &_net_status) != 1) {
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



    String getNetStatusBuff = "";
    if (!_SIM_Base.sendCommandGetRespondOneLine("AT+NETOPEN?", &getNetStatusBuff, 300)) {
        GSM_LOG_E("Get net status fail");
        return false;
    }
    
    int net_status = 0;
    if (sscanf(getNetStatusBuff.c_str(), "+NETOPEN: %d", &net_status) != 1) {
        GSM_LOG_E("Get net status format fail");
        return false;
    }
    
    return net_status == 1;
}

bool GSMNetwork::networkOpen(uint32_t timeout) {
    if (this->isNetworkOpen()) {
        return true;
    }

    return true;

    String net_open_status = "";
    if (!_SIM_Base.sendCommandGetRespondOneLine("AT+NETOPEN", &net_open_status, timeout + 3000)) {
        GSM_LOG_E("NET Open error : %s", net_open_status.c_str());
    }

    int error = -1;
    if (sscanf(net_open_status.c_str(), "+NETOPEN: %d", &error) != 1) {
        GSM_LOG_E("NET Open format fail");
        return false;
    }
    
    return error == 0;
}

bool GSMNetwork::networkClose() {
    if (!this->isNetworkOpen()) {
        return true;
    }

    return true;

    String net_close_status = "";
    if (!_SIM_Base.sendCommandGetRespondOneLine("AT+NETCLOSE", &net_close_status, 3000)) {
        GSM_LOG_E("NET Close error : %s", net_close_status.c_str());
    }

    int error = -1;
    if (sscanf(net_close_status.c_str(), "+NETCLOSE: %d", &error) != 1) {
        GSM_LOG_E("NET Close format fail");
        return false;
    }
    
    return error == 0;
}

GSMNetwork Network;
