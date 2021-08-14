#include "GSMNetwok.h"
#include "GSM_LOG.h"

bool GSMNetwork::isNetworkOpen() {
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
