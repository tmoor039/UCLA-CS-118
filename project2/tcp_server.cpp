#include "tcp.h"
#include <iostream>
#include <stdlib.h>

using namespace std;

TCP_Server::TCP_Server(uint16_t port)
	: TCP(port)
{
	// Create the socket file descriptor under UDP Protocol
	m_sockFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(m_sockFD == -1) { 
		perror("Socket FD Error");
		m_status = false;
	}
	// Zero out the server address info
	memset((char *) &m_serverInfo, 0, sizeof(m_serverInfo));

	// Set up server address information
	m_serverInfo.sin_family = AF_INET;
	m_serverInfo.sin_port = htons(port);
	// Allow to bind to localhost only - Change to INADDR_ANY for any address
	m_serverInfo.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	// Attempt binding
	if(m_status){
		int retval = ::bind(m_sockFD, (struct sockaddr*)&m_serverInfo, sizeof(m_serverInfo));
		if(retval == -1){
			perror("Binding Error");
			m_status = false;
		}
	}

    // other member variables
    m_cwnd = 1024;
}

bool TCP_Server::sendData(uint8_t* data) {
	// Clear out old content of send buffer
	memset(m_sendBuffer, '\0', sizeof(m_sendBuffer));
	// Copy the encoded data into the send buffer
	copy(data, data + MSS, m_sendBuffer);
	if(sendto(m_sockFD, m_sendBuffer, MSS, 0, (struct sockaddr*)&m_clientInfo, m_cliLen) == -1){
		perror("Sending Error");
		return false;
	}
	return true;
}
bool TCP_Server::receiveData() {
	// Clear out old content of receive buffer
	memset(m_recvBuffer, '\0', sizeof(m_recvBuffer));
	if(recvfrom(m_sockFD, m_recvBuffer, MSS, 0, (struct sockaddr*)&m_clientInfo, &m_cliLen) == -1){
		perror("Receiving Error");
		return false;
	}
	return true;
}
bool TCP_Server::setTimeout(float sec, float usec, bool flag){
	struct timeval tv;
	tv.tv_sec = sec;
	tv.tv_usec = usec;
	// If flag is 1 then send timeout else receive timeout
	if(flag){
		if(setsockopt(m_sockFD, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) < 0){
			perror("Send Timeout Error");
			return false;
		}
	} else {
		if(setsockopt(m_sockFD, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0){
			perror("Receive Timeout Error");
			return false;
		}
	}
	return true;
}

bool TCP_Server::handshake(){
	// First receive packet from client
	while(!receiveData()){
		// Just keep trying
	}

	m_packet = new TCP_Packet(m_recvBuffer);
	uint16_t ack = m_packet->getHeader().fields[ACK];
	uint16_t seq = m_packet->getHeader().fields[SEQ];
	fprintf(stdout, "Receiving packet %hu\n", ack);
	delete m_packet;

	// Begin Sending Timeout
	setTimeout(0, RTO, 1);

	// Send SYN-ACK
    m_nextSeq = rand() % MAX_SEQ + 1;
	fprintf(stdout, "Sending packet %d %d %d SYN\n", m_nextSeq, PACKET_SIZE, SSTHRESH);
	m_packet = new TCP_Packet(m_nextSeq, seq + 1, PACKET_SIZE, 1, 1, 0);
	sendData(m_packet->encode());

	// Retransmit data if timeout
	while(!receiveData()){
		fprintf(stdout, "Sending packet %d %d %d Retransmission SYN\n", seq + 1, PACKET_SIZE, SSTHRESH);
		sendData(m_packet->encode());
	}
	delete m_packet;

    m_nextSeq = (m_nextSeq + 1) % MAX_SEQ;

	// Receive ACK from client
	m_packet = new TCP_Packet(m_recvBuffer);
	ack = m_packet->getHeader().fields[ACK];
	fprintf(stdout, "Receiving packet %hu\n", ack);
	// Delete packet or not?
	return true;
}

bool TCP_Server::breakFile() {
    std::FILE* fp = NULL;
    fp = std::fopen(m_filename.c_str(), "r");
    if (fp == NULL) {
        std::perror("File failed to open\n");
        return false;
    }

    int c;  // NOTE: int required to handle EOF.
    uint8_t data[1024];
    std::memset(data, '\0', sizeof(data));
    int i = 0;
    while ((c = std::fgetc(fp)) != EOF) {
        if (i < 1024) {
            data[i] = (uint8_t) c;
            i++; 
        }
        else {
            TCP_Packet packet(m_nextSeq, 0, m_cwnd, 0, 0, 0);
            packet.setData(data);
            m_filePackets.push_back(packet);
            m_nextSeq = (m_nextSeq + i) % MAX_SEQ;
            i = 0;
        }
    }
    return true;
}

bool TCP_Server::sendNextPacket() {
    int nSent;
    if(m_nextPacket < m_sendBase + m_cwnd && m_nextPacket < m_filePackets.size()){
        nSent = sendto(m_sockFD, m_filePackets.at(m_nextPacket).encode(),
                    MSS, 0, (struct sockaddr*)&m_clientInfo, m_cliLen);
        m_filePackets.at(m_nextPacket).set_sent();
        return true;
	}
	return false;
}

void TCP_Server::sendFile() {
    // TODO: manage window

    int nPackets = m_filePackets.size();
    while (m_nextPacket < nPackets) {
        // wait for window to move forward
        while (m_sendBase + m_cwnd < m_nextSeq) {
            if (m_filePackets.at(m_sendBase).is_acked()) {
                m_sendBase++;
                break;
            }
            else {
                continue;
            }
        }

        sendNextPacket();
    }
}
