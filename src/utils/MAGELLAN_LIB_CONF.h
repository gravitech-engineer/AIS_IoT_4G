#ifndef MAGELLAN_LIB_CONF_H
#define MAGELLAN_LIB_CONF_H

// =============================================================
// Magellan/AIS 4G Library Configuration (single source of truth)
// =============================================================
// Edit values in this file for Arduino IDE / PlatformIO projects.
// Do not define these in main.ino because library .cpp files are
// compiled in separate translation units.

// 0: Use bundled ArduinoJson v6.18.3 (default)
// 1: Use external ArduinoJson v7 (install via Library Manager)
// Override via platformio.ini: build_flags = -DMAGELLAN_USE_ARDUINOJSON7=1
#ifndef MAGELLAN_USE_ARDUINOJSON7
#define MAGELLAN_USE_ARDUINOJSON7 0
#endif

// Log level: 0=none, 1=error, 2=info, 3=debug (default)
// Override via platformio.ini: build_flags = -DMAGELLAN_LOG_LEVEL=2
#ifndef MAGELLAN_LOG_LEVEL
#define MAGELLAN_LOG_LEVEL 3
#endif

// Filesystem backend on ESP32: 0=LittleFS (default), 1=SPIFFS
// Override via platformio.ini: build_flags = -DMAGELLAN_USE_SPIFFS=1
#ifndef MAGELLAN_USE_SPIFFS
#define MAGELLAN_USE_SPIFFS 0
#endif

#if MAGELLAN_USE_SPIFFS
#ifndef MG_USE_SPIFFS
#define MG_USE_SPIFFS
#endif
#endif

// Token behavior: 0=auto request (default), 1=bypass auto token
// Override via platformio.ini: build_flags = -DMAGELLAN_BYPASS_REQTOKEN=1
#ifndef MAGELLAN_BYPASS_REQTOKEN
#define MAGELLAN_BYPASS_REQTOKEN 0
#endif
#if MAGELLAN_BYPASS_REQTOKEN
#ifndef BYPASS_REQTOKEN
#define BYPASS_REQTOKEN
#endif
#endif

// Max reconnect attempts before board restart (default: 1)  1: enable, 0: disable
// Override via platformio.ini: build_flags = -DUSE_MAX_ATTEMPT_RECONNECT=1
#ifndef USE_MAX_ATTEMPT_RECONNECT
#define USE_MAX_ATTEMPT_RECONNECT 1
#if USE_MAX_ATTEMPT_RECONNECT
#ifndef USE_ATTEMPT_LIMIT
#define USE_ATTEMPT_LIMIT
#endif
#endif
#endif

// Max reconnect attempts before board restart (default: 10)
// Override via platformio.ini: build_flags = -DMAX_ATTEMPT_RECONNECT=10
#ifndef MAX_ATTEMPT_RECONNECT
#define MAX_ATTEMPT_RECONNECT 10
#endif

// Max reconnect attempts reset module to recovery modem (default: 2)
#ifndef MAX_RECONNECT_RST_MODEM
#define MAX_RECONNECT_RST_MODEM 2
#endif

#ifndef SKIP_RESET_MODULE_AT_BOOT
#define SKIP_RESET_MODULE_AT_BOOT 1
#endif
#if SKIP_RESET_MODULE_AT_BOOT
#ifndef SKIP_RESET_AT_BOOT
#define SKIP_RESET_AT_BOOT
#endif
#endif

#ifndef SKIP_RESOLVE_DNS_AT_BOOT
#define SKIP_RESOLVE_DNS_AT_BOOT 0
#endif
#if SKIP_RESOLVE_DNS_AT_BOOT
#ifndef SKIP_RESOLVE_DOMAIN_AT_BOOT
#define SKIP_RESOLVE_DOMAIN_AT_BOOT
#endif
#endif

#if MAGELLAN_USE_ARDUINOJSON7
#include <ArduinoJson.h>
#else
#include "./ArduinoJson-v6.18.3.h"
#endif

#endif
