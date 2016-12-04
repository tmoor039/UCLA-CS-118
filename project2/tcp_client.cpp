#include "tcp.h"
#include <iostream>
#include <stdlib.h>
#include <utility>
#include <time.h>
#include <algorithm>
#include <unistd.h>

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
    if(sendto(m_sockFD, m_sendBuffer, data_size, 0, (struct sockaddr*)&m_serverInfo, m_serverLen) <= 0){
        return false;
    }

    return true;
}

bool TCP_Client::receiveData(){

    // Clear out the old content of the receive buffer
    memset(m_recvBuffer, '\0', sizeof(m_recvBuffer));
    m_recvSize = recvfrom(m_sockFD, m_recvBuffer, MSS, 0, (struct sockaddr*)&m_serverInfo, &m_serverLen);
    if(m_recvSize <= 0){
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
            // Detect FIN bit
            if (0x0001 & flags) {
                while(1) {
                    delete m_packet;
                    // Send the FIN/ACK
                    fprintf(stdout, "Sending packet %d FIN\n", (seq + 1) % MAX_SEQ);
                    m_packet = new TCP_Packet(ack, (seq + 1) % MAX_SEQ, PACKET_SIZE, 1, 0, 1);
                    sendData(m_packet->encode());

                    setTimeout(0, RTO * 1000, 1);
                    setTimeout(0, RTO * 1000, 0);

                    // Retransmit in case of timeout
                    int counter = 3;
                    while(!receiveData()){
                        fprintf(stdout, "Sending packet %d Retransmission FIN\n", (seq + 1) % MAX_SEQ);
                        sendData(m_packet->encode());
                        if (counter <= 0) {
                            outputFile.close();
                            close(m_sockFD);
                            return true;
                        }
                        counter--;
                    }
                    delete m_packet;

                    m_packet = new TCP_Packet(m_recvBuffer);
                    seq = m_packet->getHeader().fields[SEQ];
                    flags = m_packet->getHeader().fields[FLAGS];
                    fprintf(stdout, "Receiving packet %hu\n", seq);
                    if(flags == 0x0004) {
                        outputFile.close();
                        close(m_sockFD);
                        return true;
                    }
                }
            }

            // If an expected packet was received
            else if (seq == m_expected_seq) {

                // Write the data to the file
                for(ssize_t i = 0; i < data_size; i++){
                    outputFile << data->at(i);
                }
                while (m_written.size() >= (int)(START_WINDOW/PACKET_SIZE)) {
                    m_written.erase(m_written.begin());
                }
                m_written.push_back(seq);
                m_expected_seq = (m_expected_seq + m_packet->getLength() - HEADER_SIZE) % MAX_SEQ;

                // Check if there are more correctly-ordered packets in the buffer
                bool found = true;
                while (found){
                    found = false;
                    for (size_t i = 0; i < packet_buffer.size(); i++) {
                        if(m_expected_seq == packet_buffer[i]->getHeader().fields[SEQ]) {
                            found = true;
                            m_expected_seq = (m_expected_seq + packet_buffer[i]->getLength() - HEADER_SIZE) % MAX_SEQ;
                            data = packet_buffer[i]->getData();
                            for(ssize_t i = 0; i < (int)data->size(); i++){
                                outputFile << data->at(i);
                                while (m_written.size() >= (int)(START_WINDOW/PACKET_SIZE)) {
                                    m_written.erase(m_written.begin());
                                }
                                m_written.push_back(seq);
                                delete packet_buffer[i];
                                packet_buffer.erase(packet_buffer.begin() + i);
                            }
                        }
                    }
                }
            }

            // If an unexpected packet was received, add it to the buffer
            else {
                bool found = false;
                for (size_t i = 0; i < packet_buffer.size(); i++) {
                    if(seq == packet_buffer[i]->getHeader().fields[SEQ]) {
                        found = true;
                        break;
                    }
                }
                if (!found && find(m_written.begin(), m_written.end(), seq) == m_written.end() && packet_buffer.size() < (int)(START_WINDOW/PACKET_SIZE)) {
                    packet_buffer.push_back(m_packet);
                }
            }

            delete m_packet;
            // Send the ACK
            fprintf(stdout, "Sending packet %d\n", m_expected_seq);
            m_packet = new TCP_Packet(ack, m_expected_seq, START_WINDOW, 1, 0, 0);
            sendData(m_packet->encode());
            delete m_packet;
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
            return false;
        }
    } else {
        if(setsockopt(m_sockFD, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0){
            return false;
        }
    }
    return true;
}

bool TCP_Client::handshake(){

    // Send the very first packet
    fprintf(stdout, "Sending packet 0 SYN\n");

    // Set Sending timeout
    setTimeout(0, RTO * 1000, 1);

    // Set Receiving timeout
    setTimeout(0, RTO * 1000, 0);

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

    m_written.push_back(seq);

    // Send the ACK from the client to begin data transmission
    fprintf(stdout, "Sending packet %d\n", m_expected_seq);
    m_packet = new TCP_Packet(ack, m_expected_seq, PACKET_SIZE, 1, 0, 0);
    sendData(m_packet->encode());

    return true;
}
