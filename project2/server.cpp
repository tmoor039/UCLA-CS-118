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

bool TcpSendData(UdpServer &udpServer, string fileName) {
  return 0;
}

bool TcpHandshake(UdpServer &udpServer) {
  sockaddr_storage peerAddr;
  socklen_t peerAddrLen = sizeof(sockaddr_storage);
  udpServer.receive_packet((sockaddr *) &peerAddr, &peerAddrLen);
  udpServer.set_send_buf("Message from server\n");
  //udpServer.send_packet((sockaddr *) &peerAddr, peerAddrLen);
  sleep(500);
  return 0;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <port-number> <file-name>\n", argv[0]);
    exit(1);
  }
  int port = atoi(argv[1]);

  UdpServer udpServer(port);

  if (TcpHandshake(udpServer) != 0) {
    fprintf(stderr, "The TCP handshake failed\n");
    exit(1);
  }

  if (TcpSendData(udpServer, argv[2]) != 0) {
    fprintf(stderr, "Failed to send data\n");
    exit(1);
  }

  return 0;
}
