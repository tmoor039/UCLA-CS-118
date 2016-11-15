#include "tcp.h"
#include <iostream>
#include <stdlib.h>
#include <vector>

using namespace std;

TCP_Server::TCP_Server(uint16_t port)
    : TCP(port)
{
    struct addrinfo hints;                                                           
    struct addrinfo *result, *rp;                                                    
    memset(&hints, 0, sizeof(hints));                                                
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM; // for UDP
    hints.ai_protocol = IPPROTO_UDP; 
    // Find an address suitable for accepting connections

    int ret = getaddrinfo("localhost", std::to_string(port).c_str(), &hints, &result);

    if (ret != 0) { 
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    }

    int sfd;
    for (rp = result; rp != NULL; rp = rp->ai_next) {                                
        sfd = socket(rp->ai_family, rp->ai_socktype,  
                rp->ai_protocol); 
        if (sfd == -1)  
            continue;  
        ret = bind(sfd, rp->ai_addr, rp->ai_addrlen); 
        if (ret != -1) {  
            sfd_ = sfd;                                                                  
            break;  // success
        }                                                                              
        close(sfd);
    }                                                                                
    if (rp == NULL) {
        fprintf(stderr, "Could not bind socket address\n");
    }
    freeaddrinfo(result);
}

bool TCP_Server::add_send_data(uint8_t* data, int len) {
    int i = 0;
    while (len > 0) {
        uint16_t flags = 0;
        // todo

        TCP_Packet packet(m_seq, 0, 0, flags);
        if (len >= PACKET_DATA_SIZE) {
            packet.insert_data(&data[i], PACKET_DATA_SIZE);
            i += PACKET_DATA_SIZE;
            len -= PACKET_DATA_SIZE;
        }
        else {
            packet.insert_data(&data[i], len);
            i += len;
            len -= len;
        }
    }

    return true;
}

bool TCP_Server::send_data() {
    
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
    uint16_t newSeq = rand() % MAX_SEQ + 1;
    fprintf(stdout, "Sending packet %d %d %d SYN\n", newSeq, PACKET_SIZE, SSTHRESH);
    m_packet = new TCP_Packet(newSeq, seq + 1, PACKET_SIZE, 1, 1, 0);
    sendData(m_packet->encode());

    // Retransmit data if timeout
    while(!receiveData()){
        fprintf(stdout, "Sending packet %d %d %d Retransmission SYN\n", seq + 1, PACKET_SIZE, SSTHRESH);
        sendData(m_packet->encode());
    }
    delete m_packet;

    // Receive ACK from client
    m_packet = new TCP_Packet(m_recvBuffer);
    ack = m_packet->getHeader().fields[ACK];
    fprintf(stdout, "Receiving packet %hu\n", ack);
    // Delete packet or not?
    return true;
}
