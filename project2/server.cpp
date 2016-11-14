#include <iostream>
#include "Udp.h"                                                                   
#include "packet.h"
#include <sys/types.h>                                                             
#include <sys/socket.h>                                                            
#include <netdb.h>                                                                 
#include <arpa/inet.h>                                                             
#include <unistd.h>                                                                
#include <stdlib.h>

using namespace std;

bool TcpSendData(UdpServer &udpServer, string fileName) {
	sleep(500);
	return 0;
}

bool TcpHandshake(UdpServer &udpServer) {
	uint8_t* encodedPacket;
	uint16_t seq, ack;
	sockaddr_storage peerAddr;
	socklen_t peerAddrLen = sizeof(sockaddr_storage);

	// Initial infinite timeout
	udpServer.set_timeout(0, 0);

	while(udpServer.receive_packet((sockaddr *) &peerAddr, &peerAddrLen) < 0)
	{
		// Just keep trying
	}
	// Get the information about the SYN Packet from client
	TCP_Packet receivedPacket((uint8_t*)udpServer.getRecvPacket());
	ack = receivedPacket.getHeader().fields[ACK];
	seq = receivedPacket.getHeader().fields[SEQ];
	fprintf(stdout, "Receiving packet %hu\n", ack);

	udpServer.set_timeout(0, RTO);

	// Send SYN-ACK
	fprintf(stdout, "Sending packet %d %d %d SYN\n", seq+1, PACKET_SIZE, SSTHRESH); 
	TCP_Packet sendPacket(rand() % MAX_SEQ + 1, seq + 1, PACKET_SIZE, 1, 1, 0);
	encodedPacket = sendPacket.encode();
	// Set the sender info to the encoded information
	udpServer.set_send_buf(string((char *)encodedPacket));
	udpServer.send_packet((sockaddr *) &peerAddr, peerAddrLen);
	// Retransmit if failure
	while(udpServer.receive_packet((sockaddr *) &peerAddr, &peerAddrLen) < 0)
	{
		fprintf(stdout, "Sending packet %d %d %d Retransmission SYN\n", seq+1, PACKET_SIZE, SSTHRESH); 
		udpServer.send_packet((sockaddr *) &peerAddr, peerAddrLen);
	}
	// Avoid Memory Leak
	delete encodedPacket;
	// Receive SYN ACK from client, to begin sending data
	TCP_Packet received_syn_ack((uint8_t*)udpServer.getRecvPacket());
	ack = received_syn_ack.getHeader().fields[ACK];
	fprintf(stdout, "Receiving packet %hu\n", ack);
	return 0;
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <port-number> <file-name>\n", argv[0]);
		exit(1);
	}
	int port = atoi(argv[1]);
	string filename = argv[2];

	UdpServer udpServer(port);

	if (TcpHandshake(udpServer) != 0) {
		fprintf(stderr, "The TCP handshake failed\n");
		exit(1);
	}

	if (TcpSendData(udpServer, filename) != 0) {
		fprintf(stderr, "Failed to send data\n");
		exit(1);
	}

	return 0;
}
