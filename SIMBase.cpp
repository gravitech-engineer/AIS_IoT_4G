#include "SIMBase.h"
#include "GSM_LOG.h"

bool SIMBase::readStringWithTimeout(String *out, uint32_t size, uint32_t timeout) {
    uint32_t beforeTimeout = this->getTimeout();
    this->setTimeout(timeout);

    char *buff = (char*)malloc(size + 1);
    memset(buff, 0, size + 1);
    uint32_t len = this->readBytes(buff, size);
    *out = String(buff);
    free(buff);

    this->setTimeout(beforeTimeout);

    return len == size;
}

bool SIMBase::readEndsWith(String *out, uint16_t max_size, String endswith, uint32_t timeout) {
    bool foundEndOfString = false;
    uint32_t startTime = millis();
    while(((millis() - startTime) < timeout) && foundEndOfString == false) {
        delay(1);
        while(this->available() && foundEndOfString == false) {
            out->concat((char)this->read());
            if (out->endsWith(endswith)) {
                foundEndOfString = true;
                break;
            }
            startTime = millis();
        }
    }

    return foundEndOfString;
}

bool SIMBase::wait(String str, uint32_t timeout) {
    uint32_t beforeTimeout = this->getTimeout();
    this->setTimeout(timeout);

    bool isFound = this->find((const char *)str.c_str());

    this->setTimeout(beforeTimeout);

    return isFound;
}

bool SIMBase::send(String str) {
    while(this->available()) (void)this->read();

    this->print(str);

    return true;
}

bool SIMBase::sendCommand(String cmd, uint32_t timeout) {
    String finalCommand = cmd + "\r";
    this->send(finalCommand);

    return this->wait(finalCommand, timeout);
}


bool SIMBase::sendCommandFindOK(String cmd, uint32_t timeout) {
    if (!this->sendCommand(cmd)) {
        return false;
    }

    return this->wait("\r\nOK\r\n", timeout);
}

bool SIMBase::sendCommandGetRespondOneLine(String cmd, String* respond, uint32_t timeout) {
    String status = "";
    respond->clear();

    if (!this->sendCommand(cmd)) {
        GSM_LOG_I("Send command \"%s\" error timeout (1)", cmd.c_str());
        return false;
    }

    if (!this->wait("\r\n", timeout)) {
        GSM_LOG_I("Send command \"%s\" error timeout (2) [%d]", cmd.c_str(), timeout);
        return false;
    }

    if (!this->readEndsWith(&status, 255, "\r\n", 300)) {
        GSM_LOG_I("Send command \"%s\" error timeout (3)", cmd.c_str());
        return false;
    }

    status = status.substring(0, status.length() - 2);
    if (status != "OK") {
        respond->concat(status);
        status.clear();

        if (!this->wait("\r\n", 300)) {
            GSM_LOG_I("Send command \"%s\" error timeout (4) [%d]", cmd.c_str(), timeout);
            return false;
        }

        if (!this->readEndsWith(&status, 20, "\r\n", 300)) {
            GSM_LOG_I("Send command \"%s\" error timeout (5)", cmd.c_str());
            return false;
        }

        status = status.substring(0, status.length() - 2);
    } else {
        respond->clear();

        if (!this->wait("\r\n", 300)) {
            GSM_LOG_I("Send command \"%s\" error timeout (4) [%d]", cmd.c_str(), timeout);
            return false;
        }

        if (!this->readEndsWith(respond, 255, "\r\n", 300)) {
            GSM_LOG_I("Send command \"%s\" error timeout (5)", cmd.c_str());
            return false;
        }

        *respond = respond->substring(0, respond->length() - 2);
    }

    GSM_LOG_I("Send command \"%s\" got : %s || %s", cmd.c_str(), respond->c_str(), status.c_str());
    if (status == "OK") {
        return true;
    } else if (status == "ERROR") {
        return false;
    }

    return false;
}

bool SIMBase::sendCommandCheckRespond(String cmd, uint32_t timeout) {
    if (!this->sendCommand(cmd)) {
        GSM_LOG_I("Send command \"%s\" error timeout (1)", cmd.c_str());
        return false;
    }

    if (!this->wait("\r\n", timeout)) {
        GSM_LOG_I("Send command \"%s\" error timeout (2) [%d]", cmd.c_str(), timeout);
        return false;
    }

    String status = "";
    if (!this->readEndsWith(&status, 255, "\r\n", 300)) {
        GSM_LOG_I("Send command \"%s\" error timeout (3)", cmd.c_str());
        return false;
    }

    status = status.substring(0, status.length() - 2);
    GSM_LOG_I("Send command \"%s\" got : %s", cmd.c_str(), status.c_str());
    if (status == "OK") {
        return true;
    } else if (status == "ERROR") {
        return false;
    }

    return false;
}

SIMBase _SIM_Base(2); // Serial 2

