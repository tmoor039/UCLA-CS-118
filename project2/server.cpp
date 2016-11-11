#include <iostream>
#include "Udp.h"                                                                   
#include <sys/types.h>                                                             
#include <sys/socket.h>                                                            
#include <netdb.h>                                                                 
#include <arpa/inet.h>                                                             
#include <unistd.h>                                                                
#include "globals.h"                                                               
#include <stdlib.h>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port-number> <file-name>\n", argv[0]);
        exit(1);
    }
    int port = atoi(argv[1]);
    
    sockaddr_storage peerAddr;
    socklen_t peerAddrLen = sizeof(sockaddr_storage);

    UdpServer udpServer(port);
    udpServer.receive_packet((sockaddr *) &peerAddr, &peerAddrLen);
    udpServer.set_send_buf("Message from server\n");
    udpServer.send_packet((sockaddr *) &peerAddr, peerAddrLen);

    return 0;
}
