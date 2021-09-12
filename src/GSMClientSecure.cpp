#include "GSMClientSecure.h"
#include "GSMNetwok.h"
#include "GSM_LOG.h"

GSMClientSecure::GSMClientSecure() {
    sslclient = new gsm_sslclient_context;
    gsm_ssl_init(sslclient);
    sslclient->handshake_timeout = 120000;
}

void GSMClientSecure::setInsecure() {
    this->insecure = true;
}

void GSMClientSecure::setPreSharedKey(const char *pskIdent, const char *psKey) {
    this->pskIdent = pskIdent;
    this->psKey = psKey;
}

void GSMClientSecure::setCACert(const char *rootCA) {
    this->CA_cert = rootCA;
}

void GSMClientSecure::setCertificate(const char *client_ca) {
    this->cert = client_ca;
}
void GSMClientSecure::setPrivateKey (const char *private_key) {
    this->private_key = private_key;
}

bool GSMClientSecure::verify(const char* fingerprint, const char* domain_name) {
    if (!this->sslclient) {
        return false;
    }

    return gsm_verify_ssl_fingerprint(sslclient, fingerprint, domain_name);
}

void GSMClientSecure::setHandshakeTimeout(unsigned long handshake_timeout) {
    sslclient->handshake_timeout = handshake_timeout * 1000;
}

int GSMClientSecure::connect(IPAddress ip, uint16_t port, int32_t timeout) {
    return this->connect(ip.toString().c_str(), port, timeout);
}

int GSMClientSecure::connect(IPAddress ip, uint16_t port, const char *rootCABuff, const char *cli_cert, const char *cli_key) {
    this->CA_cert = rootCABuff;
    this->cert = cli_cert;
    this->private_key = cli_key;
    return this->connect(ip.toString().c_str(), port);
}

int GSMClientSecure::connect(const char *host, uint16_t port, const char *rootCABuff, const char *cli_cert, const char *cli_key) {
    this->CA_cert = rootCABuff;
    this->cert = cli_cert;
    this->private_key = cli_key;
    return this->connect(host, port);
}

int GSMClientSecure::connect(IPAddress ip, uint16_t port, const char *pskIdent, const char *psKey) {
    this->pskIdent = pskIdent;
    this->psKey = psKey;
    return this->connect(ip.toString().c_str(), port);
}

int GSMClientSecure::connect(const char *host, uint16_t port, const char *pskIdent, const char *psKey) {
    this->pskIdent = pskIdent;
    this->psKey = psKey;
    return this->connect(host, port);
}

int GSMClientSecure::connect(const char *host, uint16_t port, int32_t timeout) {
    return gsm_start_ssl_client(
        this->sslclient, 
        host, port, 
        timeout, 
        this->CA_cert, this->cert, this->private_key, 
        this->pskIdent, this->psKey,
        this->insecure
    );
}

size_t GSMClientSecure::write(uint8_t c) {
    return this->write((const uint8_t *)&c, 1);
}

size_t GSMClientSecure::write(const uint8_t *buf, size_t size) {
    if (!this->sslclient) {
        return -1;
    }

    if (!this->sslclient->client) {
        return -1;
    }

    return gsm_send_ssl_data(this->sslclient, buf, size);
}

int GSMClientSecure::available() {
    if (!this->sslclient) {
        return 0;
    }

    if (!this->sslclient->client) {
        return 0;
    }

    int data_in_buffer = gsm_data_to_read(this->sslclient);
    if (data_in_buffer < 0) {
        if (data_in_buffer == -0x7880) { // MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY
            this->stop();
        }
        return 0;
    }

    return data_in_buffer;
}

int GSMClientSecure::read() {
    char c;
    if (this->read((uint8_t*)&c, 1) > 0) {
        return c;
    } else {
        return -1;
    }
}

int GSMClientSecure::read(uint8_t *buf, size_t size) {
    if (!this->sslclient) {
        return -1;
    }

    if (!this->sslclient->client) {
        return -1;
    }

    int in_buffer = this->available();
    if (in_buffer <= 0) {
        return -1;
    }

    return gsm_get_ssl_receive(this->sslclient, buf, min(size, (size_t)in_buffer));
}

int GSMClientSecure::peek() {
    return -1; // Not support
}

void GSMClientSecure::flush() { // Not support
    
}

uint8_t GSMClientSecure::connected() {
    if (this->available() > 0) {
        return 1;
    }

    if (!this->sslclient) {
        return 0;
    }

    if (!this->sslclient->client) {
        return 0;
    }

    return this->sslclient->client->connected();
}

GSMClientSecure::operator bool() {
    return this->connected();
}

void GSMClientSecure::stop() {
    if (!this->sslclient) {
        return;
    }

    if (!this->sslclient->client) {
        return;
    }

    gsm_stop_ssl_socket(this->sslclient);
}

GSMClientSecure::~GSMClientSecure() {
    this->stop();
    free(sslclient);
}
