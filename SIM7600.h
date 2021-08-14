#pragma once

#include <stdbool.h>  

bool GSM_AT() ;
bool GSM_init(int rx_pin, int tx_pin, int power_on_pin) ;
bool GSM_internet_configs(void) ;
bool GSM_internet_is_connected(bool *net) ;
bool GSM_ssl_configs(void) ;
bool GSM_send_http_get(char *url) ;
bool GSM_send_http_post(char *url, char* data, char* dataOut) ;

bool GSM_get_imei(char *imei) ;
bool GSM_get_local_datetime(int *year, int *month, int *day, int *hour, int *minute, int *second) ;

bool GSM_gnss_setup() ;
bool GSM_get_location(bool *fixed, float *lat, float *lng) ;

bool GSM_power_down() ;
