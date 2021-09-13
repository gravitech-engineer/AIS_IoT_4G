
#include "GSMSocket.h"
#include "SIMBase.h"
#include "GSM_LOG.h"

struct Socket_TCPUDP ClientSocketInfo[10];

void setup_Socket() {
    static bool setup_socket = false;

    if (setup_socket) {
        return;
    }

    _SIM_Base.URCRegister("+CIPRXGET: 1", [](String urcText) {
        int socket_id = -1;
        if (sscanf(urcText.c_str(), "+CIPRXGET: 1,%d", &socket_id) != 1) {
            GSM_LOG_E("+CIPRXGET: 1 format fail");
            GSM_LOG_I("Triger read_request_flag all socket for fixed");
            for (uint8_t i=0;i<10;i++) {
                if (ClientSocketInfo[i].itUsing) {
                    ClientSocketInfo[i].read_request_flag = true;
                }
            }
            return;
        }

        if ((socket_id < 0) || (socket_id > 9)) {
            GSM_LOG_E("+CIPRXGET: 1 | Socket %d is out of range", socket_id);
            GSM_LOG_I("Triger read_request_flag all socket for fixed");
            for (uint8_t i=0;i<10;i++) {
                if (ClientSocketInfo[i].itUsing) {
                    ClientSocketInfo[i].read_request_flag = true;
                }
            }
            return;
        }

        ClientSocketInfo[socket_id].read_request_flag = true;
    });

    setup_socket = true;
}
