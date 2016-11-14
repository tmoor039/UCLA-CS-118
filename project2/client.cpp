#include <iostream>
#include "Udp.h"
#include <sys/time.h>

using namespace std;

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <server-host-or-ip> <port-number>\n", argv[0]);
    exit(1);
  }
  string serverHost = string(argv[1]);
  int serverPort = atoi(argv[2]);

  UdpClient udpClient(serverHost, serverPort);

  // Set recv timeout
  udpClient.set_timeout(0, 500000);

  // Create and send packet
  udpClient.set_send_buf("Message from client\n");
  udpClient.send_packet();

  // Retransmit in the case of a timeout
  while (udpClient.receive_packet() < 0) {
    fprintf(stderr, "The packet timed out. Retransmitting\n");
    udpClient.set_send_buf("Message from client\n");
    udpClient.send_packet();
  }

  return 0;
}
