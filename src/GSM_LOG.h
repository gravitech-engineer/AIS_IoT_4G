#define DEBUG_ERROR   (1 << 0)
#define DEBUG_WARN    (1 << 1)
#define DEBUG_INFO    (1 << 2)
#define DEBUG_VERB    (1 << 3)

#define DEBUG_LEVEL (0)
#define DEBUG_SERAIL Serial

#if (DEBUG_LEVEL & 0x01)
#define GSM_LOG_E(FORMAT, ...) DEBUG_SERAIL.printf("[E]: " FORMAT "\n", ## __VA_ARGS__)
#else
#define GSM_LOG_E(FORMAT, ...) ;
#endif

#if ((DEBUG_LEVEL >> 1) & 0x01)
#define GSM_LOG_W(FORMAT, ...) DEBUG_SERAIL.printf("[W]: " FORMAT "\n", ## __VA_ARGS__)
#else
#define GSM_LOG_W(FORMAT, ...) ;
#endif

#if ((DEBUG_LEVEL >> 2) & 0x01)
#define GSM_LOG_I(FORMAT, ...) DEBUG_SERAIL.printf("[I]: " FORMAT "\n", ## __VA_ARGS__)
#else
#define GSM_LOG_I(FORMAT, ...) ;
#endif

#if ((DEBUG_LEVEL >> 3) & 0x01)
#define GSM_LOG_V(FORMAT, ...) DEBUG_SERAIL.printf("[V]: " FORMAT "\n", ## __VA_ARGS__)
#else
#define GSM_LOG_V(FORMAT, ...) ;
#endif
