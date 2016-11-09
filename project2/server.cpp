#include <iostream>
#include "UdpServer.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port-number> <file-name>\n", argv[0]);
    }
    UdpServer udpServer(atoi(argv[1]));
    udpServer.accept();

    return 0;
}
