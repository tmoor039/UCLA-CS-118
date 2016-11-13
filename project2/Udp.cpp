#include "Udp.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "globals.h"
#include <stdlib.h>

UdpPacket::UdpPacket(uint16_t srcPort, uint16_t destPort) {
    memset((uint8_t *) &header_[0], srcPort, sizeof(srcPort));
    memset((uint8_t *) &header_[2], destPort, sizeof(destPort));
    set_checksum();
    memset((uint8_t *) data_, '\0', sizeof(data));
}

void UdpPacket::set_checksum() {
    // NOT LEGIT
    memset((uint8_t *) &header_[6], '\0', 2);
}

bool UdpPacket::insert_data(uint8_t data[], int len) {
    if (len > 1024) {
        fprintf(stderr, "cannot insert %d bytes to packet\n");
        return false;
    }
    memcpy((uint8_t *) &data_, data, len);
    return true;
}

void UdpPacket::set_length(uint16_t length) {
    memset((uint8_t *) &header_[4], length, sizeof(length));
}

Udp::Udp(uint16_t port) 
    : port_(port), sfd_(-1)
{}

void Udp::set_send_buf(std::string data) {
    int i = 0;
    int len = data.length();

    uint8_t packetData[PACKET_DATA_SIZE];
    memset((uint8_t *) data, '\0', sizeof(data));
    while (i < len) {
        UdpPacket packet((uint8_t) 0, (uint8_t) 0);
        // todo
        
        int dist = len - i;
        if (dist >= PACKET_DATA_SIZE) {
            memcpy((uint8_t *) data, (uint8_t *) &data[i], PACKET_DATA_SIZE);  
        else {
            memcpy((uint8_t *) data, (uint8_t *) &data[i], dist);
        }
        sendBuf_.append(packet);
        i++;
    }
}

UdpServer::UdpServer(int port)                                                     
    : Udp(port)
{                                                                                  
    struct addrinfo hints;                                                         
    struct addrinfo *result, *rp;                                                  
    memset(&hints, 0, sizeof(hints));                                              
    hints.ai_family = AF_UNSPEC;                                                   
    hints.ai_socktype = SOCK_DGRAM; // for UDP                                     
    hints.ai_protocol = IPPROTO_UDP;                                               
    //hints.ai_flags = AI_PASSIVE;                                                   
    // Find an address suitable for accepting connections                      

    //int ret = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &result);
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

ssize_t Udp::receive_packet(sockaddr* srcAddr, socklen_t* addrLen) {
    ssize_t nread;
    nread = recvfrom(sfd_, recvPacket_, PACKET_SIZE, 0,
        srcAddr, addrLen);
    if (nread == -1)
        fprintf(stderr, "recvfrom error\n");
    std::cout << std::string(recvPacket_) << std::endl;
    return nread;
}

ssize_t Udp::send_packet(sockaddr* destAddr, socklen_t addrLen) {
    ssize_t nsent;
    nsent = sendto(sfd_, sendPacket_, PACKET_SIZE, 0, destAddr, addrLen);
    if (nsent == -1) {
        fprintf(stderr, "sendto error\n");
    }
    return nsent;
}

ssize_t UdpClient::receive_packet() {
    ssize_t nread;
    nread = recvfrom(sfd_, recvPacket_, PACKET_SIZE, 0, destAddr_, &destAddrLen_);
    if (nread == -1) {
        fprintf(stderr, "recvfrom error\n");
    }
    std::cout << std::string(recvPacket_) << std::endl;
    return nread;
}

ssize_t UdpClient::send_packet() {
    /*ssize_t nsent;
    nsent = sendto(sfd_, sendPacket_, PACKET_SIZE, 0, destAddr_, destAddrLen_);
    if (nsent == -1) {
        fprintf(stderr, "sendto error\n");
    }*/
    
    int len = strlen(sendPacket_);
    if (write(sfd_, sendPacket_, len) != len) {
        fprintf(stderr, "Incomplete write\n");
    }
    return 0;
    //return nsent;
}

UdpClient::UdpClient(std::string serverHost, int port)
    : Udp(port), serverHost_(serverHost) 
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM; // for UDP
    hints.ai_protocol = IPPROTO_UDP;

    int ret = getaddrinfo(serverHost_.c_str(), 
        std::to_string(port).c_str(), &hints, &result);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    }
    int sfd;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
        if (sfd == -1)
            continue;
        ret = connect(sfd, rp->ai_addr, rp->ai_addrlen);
        if (ret != -1) {
            sfd_ = sfd;
            destAddr_ = rp->ai_addr;
            destAddrLen_ = rp->ai_addrlen;
            break;  // success
        }
        close(sfd);
    }
    if (rp == NULL) {
        fprintf(stderr, "Could not connect with socket address\n");
    }
    freeaddrinfo(result);
}
