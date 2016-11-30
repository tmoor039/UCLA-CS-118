#include "tcp.h"
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <utility>
#include <time.h>

using namespace std;

TCP_Client::TCP_Client(string serverHost, uint16_t port)
	: TCP(port), m_serverHost(serverHost)
{

	// Create a socket under UDP Protocol
	m_sockFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(m_sockFD == -1){
		perror("Socket FD Error");
		m_status = false;
	}

	// Zero out the server address fields
	memset((char *)&m_serverInfo, 0, m_serverLen);
	m_serverInfo.sin_family = AF_INET;
	m_serverInfo.sin_port = htons(port);
	if(m_status){

		// Convert hostname to ip address
		string hostname = serverHost;
		if(serverHost == "localhost") { hostname = "127.0.0.1"; }
		if(inet_aton(hostname.c_str(), &m_serverInfo.sin_addr) == 0){
			perror("Host name conversion Error");
			m_status = false;
		}
	}
}

bool TCP_Client::sendData(uint8_t* data, ssize_t data_size){

	// Clear out the send buffer
	memset(m_sendBuffer, '\0', sizeof(m_sendBuffer));

	// Copy the encoded data to the send buffer
	copy(data, data + data_size, m_sendBuffer);
	if(sendto(m_sockFD, m_sendBuffer, data_size, 0, (struct sockaddr*)&m_serverInfo, m_serverLen) == -1){
		perror("Sending Error");
		return false;
	}

	return true;
}

bool TCP_Client::receiveData(){

	// Clear out the old content of the receive buffer
	memset(m_recvBuffer, '\0', sizeof(m_recvBuffer));
	m_recvSize = recvfrom(m_sockFD, m_recvBuffer, MSS, 0, (struct sockaddr*)&m_serverInfo, &m_serverLen);
    if(m_recvSize == -1){
		perror("Receiving Error");
		return false;
	}

	return true;
}

bool TCP_Client::receiveFile(){

    // Open a new file
    ofstream outputFile(RECEIVED_FILE_NAME);

    // Create a vector to hold buffered packets
    vector<TCP_Packet*> packet_buffer;

    // Receive the file
    int nPackets = 0;
    while(1){
        if(receiveData()){

            // Parse packet data
            m_packet = new TCP_Packet(m_recvBuffer, m_recvSize);
            uint16_t ack = m_packet->getHeader().fields[ACK];
            uint16_t seq = m_packet->getHeader().fields[SEQ];
            uint16_t flags = m_packet->getHeader().fields[FLAGS];
            vector<uint8_t>* data = m_packet->getData();
            ssize_t data_size = data->size();
			fprintf(stdout, "Receiving packet %hu\n", seq);

            // If an expected packet was received
            if (seq == m_expected_seq) {

                // Write the data to the file
			    for(int i = 0; i < data_size; i++){
				    outputFile << data->at(i);
			    }
			    delete m_packet;

                // Check if there are more correctly-ordered packets in the buffer
                m_expected_seq = (m_expected_seq + m_packet->getLength() - HEADER_SIZE) % MAX_SEQ;
                while (packet_buffer.size() > 0 && m_expected_seq == packet_buffer[0]->getHeader().fields[SEQ]){
                    m_expected_seq = (m_expected_seq + packet_buffer[0]->getLength() - HEADER_SIZE) % MAX_SEQ;
                    data = packet_buffer[0]->getData();
			        for(int i = 0; i < data_size; i++){
				        outputFile << data->at(i);
			        }
                    delete packet_buffer[0];
                    packet_buffer.erase(packet_buffer.begin());
                }
            }

            // If an unexpected packet was received, add it to the buffer in sequence order
            else {
                bool saved = false;
                for (vector<TCP_Packet*>::iterator it = packet_buffer.begin(); it != packet_buffer.end(); it++){
                    TCP_Packet* current_packet = *it;
                    if (seq < current_packet->getHeader().fields[SEQ]) {
                        saved = true;
                        packet_buffer.insert(it, m_packet);
                    }
                }
                if (!saved) {
                    packet_buffer.push_back(m_packet);
                }
            }

			// Detect FIN bit
			if (0x0001 & flags) {
				// Send the FIN/ACK
				fprintf(stdout, "Sending packet %d FIN\n", (seq + m_recvSize - HEADER_SIZE) % MAX_SEQ);
				m_packet = new TCP_Packet(ack, (seq + m_recvSize - HEADER_SIZE) % MAX_SEQ, PACKET_SIZE, 1, 0, 1);
				sendData(m_packet->encode());
				nPackets++;

				setTimeout(0, RTO, 0);

				// Retransmit in case of timeout
				while(!receiveData()){
					fprintf(stdout, "Sending packet %d Retransmission FIN\n", (seq + m_recvSize - HEADER_SIZE) % MAX_SEQ);
					sendData(m_packet->encode());
				}

				outputFile.close();
				return true;
			}

			// Send the ACK
			fprintf(stdout, "Sending packet %d\n", (seq + m_recvSize - HEADER_SIZE) % MAX_SEQ);
			m_packet = new TCP_Packet(ack, (seq + m_recvSize - HEADER_SIZE) % MAX_SEQ, PACKET_SIZE, 1, 0, 0);
			sendData(m_packet->encode());
			nPackets++;
		}
	}
}

bool TCP_Client::setTimeout(float sec, float usec, bool flag){
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

bool TCP_Client::handshake(){

	// Send the very first packet
	fprintf(stdout, "Sending packet 0 SYN\n");

	// Set Sending timeout
	setTimeout(0, RTO, 1);

	srand(time(NULL));

	m_packet = new TCP_Packet(rand() % MAX_SEQ + 1, 0, PACKET_SIZE, 0, 1, 0);
	sendData(m_packet->encode());

	// Retransmit in case of timeout
	while(!receiveData()){
		fprintf(stdout, "Sending packet 0 Retransmission SYN\n");
		sendData(m_packet->encode());
	}
	delete m_packet;

	// Receive SYN-ACK from server
	m_packet = new TCP_Packet(m_recvBuffer);
	uint16_t ack = m_packet->getHeader().fields[ACK];
	uint16_t seq = m_packet->getHeader().fields[SEQ];
    m_expected_seq = (seq + 1) % MAX_SEQ;
	fprintf(stdout, "Receiving packet %hu\n", seq);
	delete m_packet;

	// Send the ACK from the client to begin data transmission
	fprintf(stdout, "Sending packet %d\n", ack);
	m_packet = new TCP_Packet(ack, m_expected_seq, PACKET_SIZE, 1, 0, 0);
	sendData(m_packet->encode());

	return true;
}
