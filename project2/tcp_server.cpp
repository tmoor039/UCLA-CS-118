#include "tcp.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include "globals.h"
#include <cmath>
#include <iterator>

using namespace std;

TCP_Server::TCP_Server(uint16_t port, string filename)
    : TCP(port), m_filename(filename)
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

    m_nextPacket = 0;
    m_basePacket = 0;
    m_cwnd = 1;
    // Open up file to read from
    m_file.open(m_filename.c_str(), ios::in | ios::binary);
    // Server will ensure this is effectively handled
    // That is, exit the program if this happens
    if(!m_file || !m_file.is_open()){
        perror("File failed to open\n");
    	m_status = false;
    }
    // Get length of file while restoring file pointer state
    m_file.seekg(0, m_file.end);
    m_bytes = m_file.tellg();
    m_file.seekg(0, m_file.beg);
    
    // start with slow-start
    m_CCMode = SS;
}
TCP_Server::~TCP_Server(){
	// Close file if it was left open
	if(m_file.is_open()){
		m_file.close();
	}
}

bool TCP_Server::sendData(uint8_t* data, ssize_t data_size) {
    // Clear out old content of send buffer
    memset(m_sendBuffer, '\0', sizeof(m_sendBuffer));
    // Copy the encoded data into the send buffer
    copy(data, data + data_size, m_sendBuffer);
    if(sendto(m_sockFD, m_sendBuffer, data_size, 0, (struct sockaddr*)&m_clientInfo, m_cliLen) == -1){
        perror("Sending Error");
        return false;
    }
    return true;
}
bool TCP_Server::receiveData() {
    // Clear out old content of receive buffer
    memset(m_recvBuffer, '\0', sizeof(m_recvBuffer));

    // MSG_DONTWAIT makes it nonblocking
	ssize_t m_recvSize = recvfrom(m_sockFD, m_recvBuffer, MSS, 0, (struct sockaddr*)&m_clientInfo, &m_cliLen);
    if(m_recvSize == -1){
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
        continue;
    }

	// Determine packet just received
    m_packet = new TCP_Packet(m_recvBuffer);
    uint16_t ack = m_packet->getHeader().fields[ACK];
    uint16_t seq = m_packet->getHeader().fields[SEQ];
    fprintf(stdout, "Receiving packet %hu\n", ack);
    delete m_packet;

    // Begin Sending Timeout
    setTimeout(0, RTO, 1);

    // Send SYN-ACK
    srand(time(NULL));
    m_nextSeq = rand() % MAX_SEQ + 1;
    m_baseSeq = m_nextSeq;
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
    delete m_packet;
    return true;
}

bool TCP_Server::grabChunk(ssize_t num_chunks){
    // Create temp buffer to hold at most PACKET_SIZE bytes
    char data[PACKET_SIZE];
    // Total number of chunks
	int tot_chunks = ceil((float)m_bytes/PACKET_SIZE);
	if(num_chunks > tot_chunks){
		return false;
	}
	bool shouldEnd = false;
	if(m_file.is_open()){
		for(ssize_t i = 0; i < num_chunks; i++){
			ssize_t data_size = 0;
			// Clear out the data
			memset(data, '\0', sizeof(data));
			ssize_t file_pos = m_file.tellg();
			// There is no longer a fixed chunk of data left
			if(file_pos + PACKET_SIZE > m_bytes){
				// Get the remaining bytes
				ssize_t remaining = m_bytes - m_file.tellg();
				m_file.read(data, remaining);
				data_size = remaining;
				shouldEnd = true;
			} else {
				// Read fixed chunks
				m_file.read(data, PACKET_SIZE);
				data_size = PACKET_SIZE;
			}
			// Create a packet based on data
			TCP_Packet packet(m_nextSeq, 0, m_cwnd*PACKET_DATA_SIZE, 0, 0, 0);
			packet.setData(data, data_size);
			m_filePackets.push_back(packet);
			// Set the next sequence
			m_nextSeq = (m_nextSeq + data_size) % MAX_SEQ;
			// If we just grabbed the remaining chunk, we're done with data
			if(shouldEnd)
				m_file.close();
			break;
		}
		return true;
	}
	return false;
}

bool TCP_Server::testWrite() {
	std::ofstream outf("test.dat");

	if (!outf) {
		std::cerr << "testWrite failed to open file for writing\n";
		return false;
	}

	int nPackets = m_filePackets.size();
	// Write each packet's data to ensure the breaking of the file is correct
	for (int i = 0; i < nPackets; i++) {
		vector<uint8_t>* curr_packet = m_filePackets.at(i).getData();
		copy(curr_packet->begin(), curr_packet->end(), ostream_iterator<uint8_t>(outf));
	}

	return true;
}

ssize_t TCP_Server::removeAcked(){
	// Use i to count the number of elements popped
	ssize_t i = 0;
	// Pop all the front ACKed Packets
	while(m_filePackets.at(0).isAcked()){
		m_filePackets.erase(m_filePackets.begin());
		i++;
		if(m_filePackets.empty())
			break;
	}
	return i;
}

bool TCP_Server::sendNextPacket(ssize_t pos, bool resend) {
	// next packet is within window and does not exceed the file
	if(pos < m_cwnd){
		// Only send the variable packet length
		ssize_t packet_size = m_filePackets.at(pos).getLength();
		if(!sendData(m_filePackets.at(pos).encode(), packet_size)){
			return false;
		}
		if(resend) { 
			fprintf(stdout, "Sending packet %d Retransmission\n", m_filePackets.at(pos).getHeader().fields[SEQ]);
		} else {
			fprintf(stdout, "Sending packet %d\n", m_filePackets.at(pos).getHeader().fields[SEQ]);
			m_filePackets.at(pos).setSent();
		}
		return true;
	}
	// either next packet is out of window range or last packet has been sent
	return false;
}

bool TCP_Server::sendFile() {
	// Get the very first chunk
	uint16_t ack = 0;
	grabChunk();
	while(!m_filePackets.empty()){
		// Window size based on stored packets
		ssize_t win_size = m_filePackets.size();
		// Send everything in the current window
		for(ssize_t i = 0; i < win_size; i++){
			if(!m_filePackets.at(i).isAcked()){
				if(m_filePackets.at(i).isSent()){
					// Retransmission - if its been sent and timed out
					if(m_filePackets.at(i).hasTimedOut())
						sendNextPacket(i, true);
					cout << "Enter IF" << endl;
				} else {
					// Normal sent - Start the timer after the send
					cout << "Enter else" << endl;
					sendNextPacket(i, false);
					m_filePackets.at(i).startTimer();
				}
			}
		}
		// Wait to receive data containing ack
		while(!receiveData()) { continue; }
		// Mark the packet as acked
		ack = receiveAck();
		fprintf(stdout, "Receiving packet %d\n", ack);
		ssize_t move_forward = removeAcked();
		// Based on congestion window, update cwnd
		m_cwnd++;
		// If we just received an Ack for one of the first packets
		// We can move our window forward to the right
		cout << "Forward: " << move_forward << endl;
		cout << "Buffer size: " << m_filePackets.size() << endl;
		//if(m_cwnd > m_filePackets.size()){ cerr << "VBHEFOVBEAIRBVQEIRV" << endl; }
		if(move_forward > 0){
			// Grab as much as we can
			grabChunk(m_cwnd - m_filePackets.size());
		}
	}
	// TODO: Change timeout
	setTimeout(0, RTO, 1);

	// Send FIN
	m_baseSeq = (m_baseSeq + PACKET_DATA_SIZE) % MAX_SEQ;
	fprintf(stdout, "Sending packet %d %d %d FIN\n", m_nextSeq, PACKET_SIZE, SSTHRESH);
	m_packet = new TCP_Packet(m_nextSeq, m_baseSeq, PACKET_SIZE, 0, 0, 1);
	sendData(m_packet->encode());

	// Retransmit FIN if timeout
	while(!receiveData()){
		fprintf(stdout, "Sending packet %d %d %d Retransmission FIN\n", m_nextSeq, PACKET_SIZE, SSTHRESH);
		sendData(m_packet->encode());
	}
	delete m_packet;

	m_nextSeq = (m_nextSeq + 1) % MAX_SEQ;

	// Receive FIN/ACK from client
	m_packet = new TCP_Packet(m_recvBuffer);
	uint16_t seq = m_packet->getHeader().fields[SEQ];
	fprintf(stdout, "Receiving packet %hu\n", ack);
	delete m_packet;

	// Send ACK
	m_baseSeq = (m_baseSeq + PACKET_DATA_SIZE) % MAX_SEQ;
	fprintf(stdout, "Sending packet %d %d %d FIN\n", m_nextSeq, PACKET_SIZE, SSTHRESH);
	m_packet = new TCP_Packet(m_nextSeq, seq + 1, PACKET_SIZE, 0, 0, 1);
	sendData(m_packet->encode());

	// Timed Wait
	while(1){
		// TODO: Change timeout
		setTimeout(0, RTO, 0);

		// Check if the timer successfully finishes (no more data was received)
		if(!receiveData()){
			break;
		}

		fprintf(stdout, "Sending packet %d %d %d Retransmission FIN\n", m_nextSeq, PACKET_SIZE, SSTHRESH);
		sendData(m_packet->encode());
	}

	return true;
}

uint16_t TCP_Server::receiveAck() {
	TCP_Packet* ackPacket = new TCP_Packet(m_recvBuffer);
	uint16_t ack = ackPacket->getHeader().fields[ACK];
	delete ackPacket;

	// mark packet as acked
	int packet = seq2index(ack);
	if(packet < 0){ 
		perror("Couldn't find packet");
		exit(1);
	}
	m_filePackets.at(packet).setAcked();
	return ack;
}

int TCP_Server::seq2index(uint16_t seq) {
	// Run a linear scan within the window to find corresponding
	// packet to sequence number
	ssize_t packet_buffer_size = m_filePackets.size();
	for(ssize_t i = 0; i < packet_buffer_size; i++){
		if((m_filePackets.at(i).getHeader().fields[SEQ] + m_filePackets.at(i).getData()->size()) % MAX_SEQ == seq){
			return i;
		}
	}
	// Couldn't find packet with given sequence number
	return -1;
}

uint16_t TCP_Server::index2seq(int index) {
	int indexOffset = index - m_basePacket;
	uint16_t seqOffset = indexOffset * PACKET_DATA_SIZE;
	return (m_baseSeq + seqOffset) % MAX_SEQ;
}
