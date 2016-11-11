#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>

class Udp {
public:
    Udp() {}
    Udp(int port);
    virtual ~Udp() {}
    ssize_t receive_packet();

    // accessors:
    int get_port();

protected:
    int port_;
    std::string addr_;
    int sfd_;
};


class UdpServer : public Udp {
public:
    UdpServer(int port);
	
	
};
#endif // UDP_SERVER_H
