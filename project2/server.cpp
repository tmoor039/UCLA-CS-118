#include <iostream>
#include "UdpServer.h"
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port-number> <file-name>\n", argv[0]);
    }
    int port = atoi(argv[1]);
    UdpServer udpServer(port);
    udpServer.receive_packet();

    return 0;
}
