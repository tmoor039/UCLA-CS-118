#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>

class UdpServer {
public:
    UdpServer(char* port);
	//void send_file(std::string& file);
	void accept_connection();
    ssize_t receive_packet();

	// accessors:
	int get_cfd();

private:
    std::string addr_;
    char* port_;    
    int sfd_;
	int cfd_;
};
#endif // UDP_SERVER_H
