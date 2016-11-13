#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include "globals.h"

class UdpPacket {
public:
    UdpPacket() {}
    UdpPacket(uint16_t srcPort, uint16_t destPort);
    bool insert_data(uint8_t data[]);

private:
    void set_checksum();
    // todo
    
    uint8_t header_[8];
    uint8_t data_[1024];
}

class Udp {
public:
    Udp() {}
    Udp(int port);
    
    void set_send_buf(std::string data);

    virtual ~Udp() {}
    virtual ssize_t receive_packet(sockaddr* srcAddr, socklen_t* addrLen);
    virtual ssize_t receive_packet() { return NULL; }
    // receive packet from socket and returns number of bytes received.
    // srcAddr and addrLen get filled with addr info from source.

    virtual ssize_t send_packet(sockaddr* destAddr, socklen_t addrLen);
    virtual ssize_t send_packet() { return NULL; }
    // accessors:
    int get_port();
    
protected:
    int port_;
    std::string addr_;
    std::string otherAddr_;
    int sfd_;
    char recvPacket_[PACKET_SIZE];
    char sendPacket_[PACKET_SIZE];
};

class UdpServer : public Udp {
public:
    UdpServer(int port);
};

class UdpClient : public Udp {
public:
    UdpClient(std::string serverHost, int port);
    virtual ssize_t send_packet();
    virtual ssize_t receive_packet();
private:
    std::string serverHost_;
    sockaddr* destAddr_;
    socklen_t destAddrLen_;
};
#endif // UDP_SERVER_H
