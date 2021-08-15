#define DEBUG_SERAIL Serial
#define GSM_LOG_E(FORMAT, ...) DEBUG_SERAIL.printf("[E]: " FORMAT "\n", ## __VA_ARGS__)
#define GSM_LOG_I(FORMAT, ...) DEBUG_SERAIL.printf("[I]: " FORMAT "\n", ## __VA_ARGS__)
