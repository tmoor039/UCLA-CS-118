#include <iostream>
#include "Udp.h"
#include "tcp.h"
#include <stdlib.h>
#include <sys/time.h>

using namespace std;

bool TcpReceiveData(UdpClient &udpClient) {
  return 0;
}

bool TcpHandshake(UdpClient &udpClient) {
  fprintf(stdout, "Sending packet 0 SYN");

  // Set recv timeout
  udpClient.set_timeout(0, 500000);

  // Create initial TCP packet
  TCP_Packet packet(rand() % 30721, 0, 1024, 0, 1, 0);

  // Create and send packet
  udpClient.set_send_buf("Message from client\n");
  udpClient.send_packet();

  // Retransmit in the case of a timeout
  while (udpClient.receive_packet() < 0) {
    fprintf(stdout, "Sending packet 0 SYN");
    udpClient.set_send_buf("Message from client\n");
    udpClient.send_packet();
  }

  return 0;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <server-host-or-ip> <port-number>\n", argv[0]);
    exit(1);
  }
  string serverHost = string(argv[1]);
  int serverPort = atoi(argv[2]);

  UdpClient udpClient(serverHost, serverPort);

  if (TcpHandshake(udpClient) != 0) {
    fprintf(stderr, "The TCP handshake failed\n");
    exit(1);
  }

  if (TcpReceiveData(udpClient) != 0) {
    fprintf(stderr, "Failed to receive data\n");
    exit(1);
  }

  return 0;
}
