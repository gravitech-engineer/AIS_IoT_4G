// #define DEBUG_GSM

#define DEBUG_SERAIL Serial
#define GSM_LOG_E(FORMAT, ...) DEBUG_SERAIL.printf("[E]: " FORMAT "\n", ## __VA_ARGS__)

#ifdef DEBUG_GSM
#define GSM_LOG_W(FORMAT, ...) DEBUG_SERAIL.printf("[W]: " FORMAT "\n", ## __VA_ARGS__)
#define GSM_LOG_I(FORMAT, ...) DEBUG_SERAIL.printf("[I]: " FORMAT "\n", ## __VA_ARGS__)
#define GSM_LOG_V(FORMAT, ...) DEBUG_SERAIL.printf("[V]: " FORMAT "\n", ## __VA_ARGS__)
#else
#define GSM_LOG_W(FORMAT, ...) ;
#define GSM_LOG_I(FORMAT, ...) ;
#define GSM_LOG_V(FORMAT, ...) ;
#endif