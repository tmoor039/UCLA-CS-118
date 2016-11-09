#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>

class UdpServer {
public:
    UdpServer(int port);
	void send_file(string& file);
	int accept();

	// accessors:
	int get_cfd();

private:
    std::string addr_;
    int port_;    
    int sfd_;
	int cfd_;
};
endif // UDP_SERVER_H
