#include <iostream>
#include "Udp.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server-host-or-ip> <port-number>\n", argv[0]);
        exit(1);
    }
    string serverHost = string(argv[1]);
    int port = atoi(argv[2]);

    UdpClient udpClient(serverHost, port);
    udpClient.set_send_buf("Message from client\n");
    udpClient.send_packet();
}
