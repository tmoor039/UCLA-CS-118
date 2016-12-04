#include "tcp.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include "globals.h"
#include <cmath>
#include <iterator>
#include <algorithm>
#include <unistd.h>

using namespace std;

bool finishing = false;
bool finished = false;

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
    // Allow to bind to localhost only - Change to INADDR_ANY for any address - ASK TA
    //m_serverInfo.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    // Accept to 0.0.0.0 which is generic for all NIC devices on machine
    inet_pton(AF_INET, "0.0.0.0", &m_serverInfo.sin_addr.s_addr);

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
    m_cwnd = 1.0;
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
    if(sendto(m_sockFD, m_sendBuffer, data_size, 0, (struct sockaddr*)&m_clientInfo, m_cliLen) <= 0){
        return false;
    }
    return true;
}

bool TCP_Server::receiveData() {
    // Clear out old content of receive buffer
    memset(m_recvBuffer, '\0', sizeof(m_recvBuffer));

    // Default recvfrom, that should block
    ssize_t m_recvSize = recvfrom(m_sockFD, m_recvBuffer, MSS, 0, (struct sockaddr*)&m_clientInfo, &m_cliLen);
    if(m_recvSize == -1){
        // We have timed out based on the set timer on the socket
        if(errno == EWOULDBLOCK){
            if (finishing) {
                finished = true;
            }
        }
        return false;
    }
    return true;
}


bool TCP_Server::receiveDataNoWait() {
    // Clear out old content of receive buffer
    memset(m_recvBuffer, '\0', sizeof(m_recvBuffer));

    // MSG_DONTWAIT makes it nonblocking
    ssize_t m_recvSize = recvfrom(m_sockFD, m_recvBuffer, MSS, MSG_DONTWAIT, (struct sockaddr*)&m_clientInfo, &m_cliLen);
    if(m_recvSize <= 0){
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
            return false;
        }
    } else {
        if(setsockopt(m_sockFD, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0){
            return false;
        }
    }
    return true;
}

bool TCP_Server::handshake(){
    // First receive packet from client
    while(!receiveData()) {
        // Just keep trying
        continue;
    }

    // Determine packet just received
    m_packet = new TCP_Packet(m_recvBuffer);
    uint16_t ack = m_packet->getHeader().fields[ACK];
    uint16_t seq = m_packet->getHeader().fields[SEQ];
    fprintf(stdout, "Receiving packet %hu\n", ack);
    delete m_packet;
    m_packet = nullptr;

    // Begin Sending Timeout
    setTimeout(0, RTO * 1000, 1);

    // Begin Receiving Timeout
    setTimeout(0, RTO * 1000, 0);

    // Send SYN-ACK
    srand(time(NULL));
    m_nextSeq = rand() % MAX_SEQ + 1;
    m_baseSeq = m_nextSeq;
    fprintf(stdout, "Sending packet %d %d %d SYN\n", m_nextSeq, (int)m_cwnd * MIN_CWND, (int)m_ssthresh);
    m_packet = new TCP_Packet(m_nextSeq, seq + 1, PACKET_SIZE, 1, 1, 0);
    sendData(m_packet->encode());

    // Retransmit data if timeout
    while(1) {
        while(!receiveData()){
            fprintf(stdout, "Sending packet %d %d %d Retransmission SYN\n", m_nextSeq, (int)m_cwnd * MIN_CWND, (int)m_ssthresh);
            sendData(m_packet->encode());
        }

        // Receive ACK from client
        TCP_Packet* received_packet = new TCP_Packet(m_recvBuffer);
        ack = received_packet->getHeader().fields[ACK];
        uint16_t flags = received_packet->getHeader().fields[FLAGS];
        // Check for FIN from Client
        if (0x0004 & flags) {
            delete m_packet;
            fprintf(stdout, "Receiving packet %hu\n", ack);
            m_nextSeq = ack;
            delete received_packet;
            received_packet = nullptr;
            return true;
        }
    }
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
    // If the file is open grab as much as specified or remaining
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
            TCP_Packet packet(m_nextSeq, 0, (int)m_cwnd*PACKET_DATA_SIZE, 0, 0, 0);
            packet.setData(data, data_size);
            m_filePackets.push_back(packet);
            // Set the next sequence
            m_nextSeq = (m_nextSeq + data_size) % MAX_SEQ;
            // If we just grabbed the remaining chunk, we're done with data
            if(shouldEnd){
                m_file.close();
                break;
            }
        }
        return true;
    }
    return false;
}

bool TCP_Server::removeAcked(int pos){
    if(pos == -1)
        return false;
    // Remove all the elements before the just acked packet since 
    // they have been received on the client regardless
    m_filePackets.erase(m_filePackets.begin(), m_filePackets.size() > (size_t)pos + 1 ? m_filePackets.begin() + pos + 1 : m_filePackets.end());
    return true;
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
            fprintf(stdout, "Sending packet %d %d %d Retransmission\n", m_filePackets.at(pos).getHeader().fields[SEQ], (int)m_cwnd * MIN_CWND, (int)m_ssthresh);
        } else {
            fprintf(stdout, "Sending packet %d %d %d\n", m_filePackets.at(pos).getHeader().fields[SEQ], (int)m_cwnd * MIN_CWND, (int)m_ssthresh);
            m_filePackets.at(pos).setSent();
        }
        return true;
    }
    // either next packet is out of window range or last packet has been sent
    return false;
}

// Send the file as per TCP Tahoe
bool TCP_Server::sendFile() {
    // Get the very first chunk
    int ack = 0;
    int seq = 0;
    // Get the very first chunk
    grabChunk();
    while(!m_filePackets.empty()){
        // Window size based on stored packets
        ssize_t win_size = m_filePackets.size();
        // Send everything in the current window
        ssize_t current_window;
        do {
            // Only look through the minimum of these bounds
            current_window = min(min((int)win_size, (int)m_cwnd), (int)(m_window/PACKET_SIZE));
            for(ssize_t i = 0; i < current_window; i++){
                if(m_filePackets.at(i).isSent()){
                    // Retransmission - if its been sent and timed out 
                    // Because the packet is lost, we reset ssthresh and
                    // switch mode back to SS if necessary
                    if(m_filePackets.at(i).hasTimedOut()){
                        m_ssthresh = (m_cwnd*MIN_CWND)/2;
                        m_cwnd = 1.0;
                        m_duplicates_received = 1;
                        m_curr_mode = SS;
                        sendNextPacket(i, true);
                        m_filePackets.at(i).startTimer();
                    }
                } else {
                    // Normal sent - Start the timer after the send
                    sendNextPacket(i, false);
                    m_filePackets.at(i).startTimer();
                }
            }
            // Non-blocking read
        } while (!receiveDataNoWait());

        do {
            // Mark the packet as acked
            ack = receiveAck();

            // Get the ack number for the currently received packet
            TCP_Packet* ackPacket = new TCP_Packet(m_recvBuffer);
            uint16_t ack_field = ackPacket->getHeader().fields[ACK];
            m_window = ackPacket->getHeader().fields[WIN];
            delete ackPacket;
            ackPacket = nullptr;

            // Run Congestion control accordingly
            switch(m_curr_mode){
                case SS: 
                    // If we get duplicate acks retransmit, if the packet is in the window
                    if(runSlowStart(ack_field)){
                        int packet_to_send = ack2index(ack_field);
                        if (packet_to_send >= 0) {
                            sendNextPacket(packet_to_send, true);
                            m_filePackets.at(packet_to_send).startTimer();
                        }
                    }
                    break;
                case CA:
                    // If we get duplicate acks retransmit, if the packet is in the window
                    if(runCongestionAvoidance(ack_field)) {
                        int packet_to_send = ack2index(ack_field);
                        if (packet_to_send >= 0) {
                            sendNextPacket(packet_to_send, true);
                            m_filePackets.at(packet_to_send).startTimer();
                        }
                    }
                    break;
                default:
                    break;
            }
            // If Ack not found, ignore and move on
            if(ack == -1) {
                continue;
            }
            // Retrieve currently acked packet
            int just_acked_index = seq2index(ack);
            TCP_Packet just_acked = m_filePackets.at(just_acked_index);
            seq = just_acked.getHeader().fields[SEQ];
            // Run the congestion control based on current packet state
            // If we just received an Ack for one of the first packets
            // We can move our window forward to the right
        } while (receiveDataNoWait());

        bool shouldGrab = false;
        int greatest_pos = -1;
        // Find the most recently acked packet
        for(ssize_t i = 0; i < (int)m_filePackets.size(); i++){
            if(m_filePackets.at(i).isAcked()) {
                greatest_pos = i;
                shouldGrab = true;
            }
        }

        // Remove all packets from the buffer until said Ack
        if (greatest_pos >= 0) {
            removeAcked(greatest_pos);
        }

        if(shouldGrab && m_cwnd > m_filePackets.size()) {
            // Grab as much as we can
            grabChunk(min((int)(m_window/PACKET_SIZE) - m_filePackets.size(), (int)m_cwnd - m_filePackets.size()));
        }
    }
    setTimeout(0, RTO * 1000, 1);
    setTimeout(0, RTO * 1000, 0);

    // Send FIN
    m_baseSeq = (m_baseSeq + PACKET_DATA_SIZE) % MAX_SEQ;
    fprintf(stdout, "Sending packet %d %d %d FIN\n", ack, (int)m_cwnd * MIN_CWND, (int)m_ssthresh);
    m_packet = new TCP_Packet(ack, seq + 1, PACKET_SIZE, 0, 0, 1);
    sendData(m_packet->encode());
    int counter = 3;

    // Retransmit FIN if timeout
    while(!receiveData()){
        m_cwnd = 1;
        fprintf(stdout, "Sending packet %d %d %d Retransmission FIN\n", ack, (int)m_cwnd * MIN_CWND, (int)m_ssthresh);
        sendData(m_packet->encode());
        if (counter <= 0) {
            return true;
        }
        counter--;
    }
    delete m_packet;
    m_packet = nullptr;

    m_nextSeq = (m_nextSeq + 1) % MAX_SEQ;

    // Receive FIN/ACK from client
    m_packet = new TCP_Packet(m_recvBuffer);
    seq = m_packet->getHeader().fields[SEQ];
    ack = m_packet->getHeader().fields[ACK];
    fprintf(stdout, "Receiving packet %d\n", ack);
    delete m_packet;
    m_packet = nullptr;

    // Send ACK
    m_baseSeq = (m_baseSeq + PACKET_DATA_SIZE) % MAX_SEQ;
    fprintf(stdout, "Sending packet %d %d %d\n", ack, (int)m_cwnd * MIN_CWND, (int)m_ssthresh);
    m_packet = new TCP_Packet(ack, seq + 1, PACKET_SIZE, 1, 0, 0);
    sendData(m_packet->encode());

    TCP_Packet* new_packet = nullptr;
    finishing = true;
    setTimeout(0, 2 * RTO * 1000, 1);
    setTimeout(0, 2 * RTO * 1000, 0);
    // Timed Wait
    while(1){

        // Check if the timer successfully finishes (no more data was received)
        int status = receiveData();
        
        if(finished) {
          //  perror("READ");
        //    cout << errno << endl;
            break;
        }

        if(status) {
           // cout << "FINISH" << endl;
            new_packet = new TCP_Packet(m_recvBuffer);
            ack = new_packet->getHeader().fields[ACK];
            fprintf(stdout, "Receiving packet %d\n", ack);
            delete new_packet;
            new_packet = nullptr;
            fprintf(stdout, "Sending packet %d %d %d Retransmission\n", ack, (int)m_cwnd * MIN_CWND, (int)m_ssthresh);
            sendData(m_packet->encode());
        }
    }
    close(m_sockFD);
    delete m_packet;
    m_packet = nullptr;
    return true;
}

int TCP_Server::receiveAck() {
    TCP_Packet* ackPacket = new TCP_Packet(m_recvBuffer);
    uint16_t ack = ackPacket->getHeader().fields[ACK];
    m_window = ackPacket->getHeader().fields[WIN];
    delete ackPacket;
    ackPacket = nullptr;

    fprintf(stdout, "Receiving packet %d\n", ack);

    // Mark packet as acked
    int packet = seq2index(ack);
    // Couldn't find the packet
    if(packet < 0){ 
        return -1;
    }
    m_filePackets.at(packet).setAcked();
    return ack;
}

int TCP_Server::seq2index(uint16_t seq) {
    // Run a linear scan within the window to find corresponding
    ssize_t packet_buffer_size = m_filePackets.size();
    for(ssize_t i = 0; i < min((int)(m_window/PACKET_SIZE), (int)packet_buffer_size); i++){
        if((m_filePackets.at(i).getHeader().fields[SEQ] + m_filePackets.at(i).getData()->size()) % MAX_SEQ == seq){
            return i;
        }
    }
    // Couldn't find packet with given sequence number
    return -1;
}

int TCP_Server::ack2index(uint16_t ack) {
    // Run a linear scan within the window to find corresponding
    // packet to ack number
    ssize_t packet_buffer_size = m_filePackets.size();
    for(ssize_t i = 0; i < min(min((int)m_cwnd, (int)(m_window/PACKET_SIZE)), (int)packet_buffer_size); i++){
        if(m_filePackets.at(i).getHeader().fields[SEQ] == ack){
            return i;
        }
    }
    // Couldn't find packet with given sequence number
    return -1;
}

// Pre-Condition - mode is Slow Start
bool TCP_Server::runSlowStart(uint16_t ack){

    // New Ack - Multiplicative Increase of Congestion Window
    if (ack != m_last_ack) {
        m_last_ack = ack;
        m_duplicates_received = 1;
        m_cwnd++;
        // State Change to Congestion Avoidance
        if(m_cwnd >= (m_ssthresh/MIN_CWND)){
            m_curr_mode = CA;
        }
        return false;
    }

    // Duplicate - Increment number of duplicates
    if (ack == m_last_ack) {
        m_duplicates_received++;
    }

    // 3 Duplicates - Reset ssthresh and Congestion Window, and return true
    // to fast retransmit
    if(m_duplicates_received >= 4){
        m_ssthresh = (m_cwnd*MIN_CWND)/2;
        m_cwnd = max(1.0, (double)(m_ssthresh/MIN_CWND));
        return true;
    }

    return false;
}
// Pre-Condition - mode is Congestion Avoidance
bool TCP_Server::runCongestionAvoidance(uint16_t ack){
    // New Ack - Additive Increase of Congestion Window
    if (ack != m_last_ack) {
        m_last_ack = ack;
        m_duplicates_received = 1;
        m_cwnd = m_cwnd + (1.0/m_cwnd);
        return false;
    }
    // Duplicate - Increment number of duplicates
    if (ack == m_last_ack) {
        m_duplicates_received++;
    }
    // 3 Duplicates - Reset ssthresh and Congestion Window, and return true
    // to fast retransmit
    if(m_duplicates_received >= 4){
        m_ssthresh = (m_cwnd*MIN_CWND)/2;
        m_cwnd = max(1.0, (double)(m_ssthresh/MIN_CWND));
        return true;
    }

    return false;
}
