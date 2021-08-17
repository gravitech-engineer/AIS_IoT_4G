/* Provide SSL/TLS functions to ESP32 with Arduino IDE
 * by Evandro Copercini - 2017 - Apache 2.0 License
 */

#ifndef GSM_ARD_SSL_H
#define GSM_ARD_SSL_H

#include "mbedtls/platform.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

#include "GSMClient.h"

typedef struct gsm_sslclient_context {
    GSMClient *client = NULL;
    mbedtls_ssl_context ssl_ctx;
    mbedtls_ssl_config ssl_conf;

    mbedtls_ctr_drbg_context drbg_ctx;
    mbedtls_entropy_context entropy_ctx;

    mbedtls_x509_crt ca_cert;
    mbedtls_x509_crt client_cert;
    mbedtls_pk_context client_key;

    unsigned long handshake_timeout;
} gsm_sslclient_context;

void gsm_ssl_init(gsm_sslclient_context *ssl_client);
int gsm_start_ssl_client(gsm_sslclient_context *ssl_client, const char *host, uint32_t port, int timeout, const char *rootCABuff, const char *cli_cert, const char *cli_key, const char *pskIdent, const char *psKey, bool insecure);
void gsm_stop_ssl_socket(gsm_sslclient_context *ssl_client);
int gsm_data_to_read(gsm_sslclient_context *ssl_client);
int gsm_send_ssl_data(gsm_sslclient_context *ssl_client, const uint8_t *data, size_t len);
int gsm_get_ssl_receive(gsm_sslclient_context *ssl_client, uint8_t *data, int length);
bool gsm_verify_ssl_fingerprint(gsm_sslclient_context *ssl_client, const char* fp, const char* domain_name);
bool gsm_verify_ssl_dn(gsm_sslclient_context *ssl_client, const char* domain_name);

#endif