#include "tcp.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <time.h>

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

    m_nextPacket = 0;
    m_basePacket = 0;
    m_cwnd = 1;
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
        // perror("Receiving Error");
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

    //// Begin Sending Timeout
    //setTimeout(0, RTO, 1);

    // Send SYN-ACK
    srand(time(NULL));
    m_nextSeq = rand() % MAX_SEQ + 1;
    m_baseSeq = m_nextSeq;
    fprintf(stdout, "Sending packet %d %d %d SYN\n", m_nextSeq, PACKET_SIZE, SSTHRESH);
    m_packet = new TCP_Packet(m_nextSeq, seq + 1, PACKET_SIZE, 1, 1, 0);
    sendData(m_packet->encode());

    //// Retransmit data if timeout
    //while(!receiveData()){
    //    fprintf(stdout, "Sending packet %d %d %d Retransmission SYN\n", seq + 1, PACKET_SIZE, SSTHRESH);
    //    sendData(m_packet->encode());
    //}

    delete m_packet;

    m_nextSeq = (m_nextSeq + 1) % MAX_SEQ;

    // wait to receive
    while (!receiveData()) {
        continue;
    }

    // Receive ACK from client
    m_packet = new TCP_Packet(m_recvBuffer);
    ack = m_packet->getHeader().fields[ACK];
    fprintf(stdout, "Receiving packet %hu\n", ack);
    delete m_packet;
    // Delete packet or not?
    return true;
}

bool TCP_Server::breakFile() {
    ifstream file(m_filename.c_str(), ios::in | ios::binary);
    if(!file || !file.is_open()){
        perror("File failed to open\n");
        return false;
    }
    // Get length of file
    file.seekg(0, file.end);
    ssize_t length = file.tellg();
    m_bytes = length; 
    file.seekg(0, file.beg);
    // Create temp buffer to hold PACKET_SIZE bytes
    char data[PACKET_SIZE];
    // Clear out data
    memset(data, '\0', sizeof(data));
    int num_chunks = length/PACKET_SIZE;
    for(ssize_t i = 0; i < num_chunks; i++){
        // Read PACKET_SIZE bytes
        file.read(data, PACKET_SIZE);
        // Create a TCP_Packet from this data
        TCP_Packet packet(m_nextSeq, 0, m_cwnd, 0, 0, 0);
        packet.setData(data);
        // Store packets
        m_filePackets.push_back(packet);
        // Increment Sequence Number
        m_nextSeq = (m_nextSeq + PACKET_SIZE) % MAX_SEQ;
    }
    // Check if there are still some bytes left in the file
    ssize_t remaining = 0;
    if((remaining = length % PACKET_SIZE) != 0){
        // Clear out data	
        memset(data, '\0', sizeof(data));
        // Read remaining bytes and store and forward sequence number
        file.read(data, remaining);
        TCP_Packet packet(m_nextSeq, 0, m_cwnd, 0, 0, 0);
        packet.setData(data);
        m_filePackets.push_back(packet);
        m_nextSeq = (m_nextSeq + remaining) % MAX_SEQ;
    }
    
    return true;
}

bool TCP_Server::testWrite() {
    std::ofstream outf("test.dat");

    if (!outf) {
        std::cerr << "testWrite failed to open file for writing\n";
        return false;
    }

    int nPackets = m_filePackets.size();
    for (int i = 0; i < nPackets; i++) {
        outf << m_filePackets.at(i).getData();    

        // print data from just 1 packet
        break;
    }

    return true;
}

bool TCP_Server::sendNextPacket() {
    int nSent;
    if (m_filePackets.at(m_nextPacket).isSent()) {
        return false;
    }
    else if(m_nextPacket < m_basePacket + m_cwnd && m_nextPacket < m_filePackets.size()){
        // next packet is within window and does not exceed the file
        nSent = sendto(m_sockFD, m_filePackets.at(m_nextPacket).encode(),
                MSS, 0, (struct sockaddr*)&m_clientInfo, m_cliLen);
        m_filePackets.at(m_nextPacket).setSent();
        return true;
    }
    // either next packet is out of window range or last packet has been sent
    return false;
}

bool TCP_Server::sendFile() {
    // TODO: manage window
    
    int nPackets = m_filePackets.size();
/*
    int currentPacket = 0;
    fprintf(stdout, "NUMBER OF PACKETS: %d\n", nPackets);
    fprintf(stdout, "Sending packet %d\n", currentPacket);
    sendData(m_filePackets[currentPacket].encode());
    while(currentPacket < nPackets - 1){
        if(receiveData()){
            currentPacket++;
            fprintf(stdout, "Sending packet %d\n", currentPacket);
            sendData(m_filePackets[currentPacket].encode());
        } else {
            fprintf(stdout, "Sending packet %d\n", currentPacket);
            sendData(m_filePackets[currentPacket].encode());
        }
    }
*/
    uint16_t ack;
    std::cout << "nPackets: " << nPackets << std::endl;
    while (m_basePacket < nPackets) {
        //std::cout << "base packet: " << m_basePacket << std::endl;
        //std::cout << "next packet: " << m_nextPacket << std::endl;    
        // if next packet is not within window, wait for window to shift right
        if (m_nextPacket >= m_basePacket + m_cwnd) {
            std::cout << "waiting for window to shift\n";
            // wait to receive acknowledgement
            while (!receiveData()) {
                continue;
            }
            ack = receiveAck();
            fprintf(stdout, "Receiving packet %d\n", ack);

            // if first packet in window is acked
            if (m_filePackets.at(m_basePacket).isAcked()) {
                // move window forward
                m_basePacket++;
                m_baseSeq = (m_baseSeq + PACKET_DATA_SIZE) % MAX_SEQ;
            }
            continue;
        }
        fprintf(stdout, "Sending packet %d\n", 
                m_filePackets.at(m_nextPacket).getHeader().fields[SEQ]);

        bool sent = sendNextPacket();
        if (!sent)  {
            fprintf(stderr, "send error\n");
        };
/*
        // repeatedly send next packet until we hit the end of window.
        while (m_nextPacket < m_basePacket + m_cwnd) {
            fprintf(stdout, "Sending packet %d\n", 
                        m_filePackets.at(m_nextPacket).getHeader().fields[SEQ]);
            bool sent = sendNextPacket();
            if (!sent) {
                fprintf(stderr, "send error\n");
                return false;
            }
            else {
                m_nextPacket++;
            }
        }
*/
        if (receiveData()) {
            ack = receiveAck();
            fprintf(stdout, "Receiving packet %d\n", ack);
        }

        // if base packet is acked, move window forward
        if (m_filePackets.at(m_basePacket).isAcked()) {
            m_basePacket++;
            m_baseSeq = (m_baseSeq + PACKET_DATA_SIZE) % MAX_SEQ;
        }

        /*
        // Receive all ack packets
        while (receiveData()) {
            ack = receiveAck();
            fprintf(stdout, "Receiving packet %d\n", ack);

            // if base packet is acked, move window forward
            if (m_filePackets.at(m_basePacket).isAcked()) {
                m_basePacket++;
                m_baseSeq = (m_baseSeq + PACKET_DATA_SIZE) % MAX_SEQ;
            }
        }
        std::cout << "done receiving acks\n";
        */

        // advance next packet
        m_nextPacket++;
            
        // TODO: how to send the last packet
        //m_nextPacket++;
        //int lastPacketSize = m_bytes % PACKET_DATA_SIZE;
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
    int packet = seq2index(ack) - 1;
    m_filePackets.at(packet).setAcked();

    return ack;
}

int TCP_Server::seq2index(uint16_t seq) {
    int seqOffset = (seq - m_baseSeq);
    int indexOffset = seqOffset / PACKET_DATA_SIZE;
    return m_basePacket + indexOffset;
}

uint16_t TCP_Server::index2seq(int index) {
    int indexOffset = index - m_basePacket;
    uint16_t seqOffset = indexOffset * PACKET_DATA_SIZE;
    return (m_baseSeq + seqOffset) % MAX_SEQ;
}
