#define DEBUG_LEVEL 0
#define DEBUG_SERAIL Serial

#if DEBUG_LEVEL >= 1
#define GSM_LOG_E(FORMAT, ...) DEBUG_SERAIL.printf("[E]: " FORMAT "\n", ## __VA_ARGS__)
#else
#define GSM_LOG_E(FORMAT, ...) ;
#endif

#if DEBUG_LEVEL >= 2
#define GSM_LOG_W(FORMAT, ...) DEBUG_SERAIL.printf("[W]: " FORMAT "\n", ## __VA_ARGS__)
#else
#define GSM_LOG_W(FORMAT, ...) ;
#endif

#if DEBUG_LEVEL >= 3
#define GSM_LOG_I(FORMAT, ...) DEBUG_SERAIL.printf("[I]: " FORMAT "\n", ## __VA_ARGS__)
#else
#define GSM_LOG_I(FORMAT, ...) ;
#endif

#if DEBUG_LEVEL >= 4
#define GSM_LOG_V(FORMAT, ...) DEBUG_SERAIL.printf("[V]: " FORMAT "\n", ## __VA_ARGS__)
#else
#define GSM_LOG_V(FORMAT, ...) ;
#endif
