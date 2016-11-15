#include <iostream>
#include "Udp.h"
#include "packet.h"
#include <stdlib.h>
#include <sys/time.h>

using namespace std;

bool TcpReceiveData(UdpClient &udpClient) {
  return 0;
}

bool TcpHandshake(UdpClient &udpClient) {
  uint8_t* encodedPacket;
  uint16_t seq, ack;

  // Sending first packet
  fprintf(stdout, "Sending packet 0 SYN\n");

  // Set recv timeout
  udpClient.set_timeout(0, 500000);

  // Create initial TCP packet
  TCP_Packet packet(rand() % 30721, 0, 1024, 0, 1, 0);
  encodedPacket = packet.encode();

  // Create and send packet
  udpClient.set_send_buf(string((char *)encodedPacket));
  udpClient.send_packet();

  // Retransmit in the case of a timeout
  while (udpClient.receive_packet() < 0) {
    fprintf(stdout, "Sending packet 0 Retransmission SYN");
    udpClient.send_packet();
  }

  // Receiving first packet
  delete encodedPacket;
  TCP_Packet receivedPacket((uint8_t*)udpClient.getRecvPacket());
  seq = receivedPacket.getHeader().fields[SEQ];
  ack = receivedPacket.getHeader().fields[ACK];
  fprintf(stdout, "Receiving packet %hu\n", seq);

  // Sending second packet
  fprintf(stdout, "Sending packet %d\n", seq + 1);
  TCP_Packet temp(ack, seq + 1, 1024, 1, 0, 0);
  packet = temp;
  encodedPacket = packet.encode();

  // Create and send packet
  udpClient.set_send_buf(string((char *)encodedPacket));
  udpClient.send_packet();

  // Retransmit in the case of a timeout
  while (udpClient.receive_packet() < 0) {
    fprintf(stdout, "Sending packet %d Retransmission\n", seq + 1);
    udpClient.send_packet();
  }

  // Receiving second packet
  delete encodedPacket;
  TCP_Packet tempReceived((uint8_t*)udpClient.getRecvPacket());
  receivedPacket = tempReceived;
  seq = receivedPacket.getHeader().fields[SEQ];
  fprintf(stdout, "Receiving packet %hu\n", seq);

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
    udpClient.handshake();

  /*if (TcpHandshake(udpClient) != 0) {
    fprintf(stderr, "The TCP handshake failed\n");
    exit(1);
  }

  if (TcpReceiveData(udpClient) != 0) {
    fprintf(stderr, "Failed to receive data\n");
    exit(1);
  }*/

  return 0;
}
