#ifndef TCP_H
#define TCP_H

#include "globals.h"
#include "packet.h"
#include <string.h>
#include <string>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <vector>

class TCP_Packet;

// Abstract base TCP class
class TCP {
protected:
	uint16_t m_port;
    //    TCP_Packet* m_packet;
    std::vector<TCP_Packet> m_recvBuf;
    std::vector<TCP_Packet> m_sendBuf;
    // uint8_t m_recvBuffer[MSS];
	// uint8_t m_sendBuffer[MSS];
	bool m_status = true;
	int m_sockFD;
    uint8_t* unpack(TCP_Packet packet);
    // converts packet to uint8_t[PACKET_SIZE]

    sockaddr* m_destAddr;
    socklen_t m_destAddrLen;

public:
	TCP(uint16_t port) 
        : m_port(port), m_destAddr(NULL), m_destAddrLen(0) {};

	virtual ~TCP(){};

	// Pure virtual methods
	virtual bool handshake() = 0;
	//virtual bool sendData(uint8_t* data) = 0;
	//virtual bool receiveData() = 0;
	//virtual bool setTimeout(float sec, float usec, bool flag) = 0;

    bool add_send_data(uint8_t* data, int len);
    virtual bool send_data(sockaddr* srcAddr = NULL, 
        socklen_t addrLen = 0) = 0;

    bool recv_data(sockaddr* srcAddr = NULL, socklen_t* addrLen = NULL);
    // add data to send buffer

    void set_dest_addr(sockaddr* destAddr, socklen_t addrLen);
    

	//// Accessors
	//uint16_t getPort() const { return m_port; }
	//uint8_t* getReceivedPacket() { return m_recvBuffer; }
	//uint8_t* getSentPacket() { return m_sendBuffer; }
	//bool getStatus() const { return m_status; }
};

// TCP Server
class TCP_Server: TCP {
	std::string m_filename;
	//struct sockaddr_in m_serverInfo, m_clientInfo;
	//socklen_t m_cliLen = sizeof(m_clientInfo);

    //uint16_t m_seq;


public:
	TCP_Server(uint16_t port);
    bool send_data(sockaddr* destAddr = NULL, socklen_t addrLen = 0) override;
	bool handshake() override;
	//bool sendData(uint8_t* data) override;
	//bool receiveData() override;
    //bool setTimeout(float sec, float usec, bool flag) override;

	// Break file into chunks
	// void breakFile(std::string filename);

	// Accessors
    //int getSocketFD() const { return m_sockFD; }
    //std::string getFilename() const { return m_filename; }

	// Mutators
	//void setFilename(std::string filename) { m_filename = filename; }

};

// TCP Client
class TCP_Client: TCP {
	std::string m_serverHost;
	//struct sockaddr_in m_serverInfo;
	//socklen_t m_serverLen = sizeof(m_serverInfo);

    sockaddr* m_destAddr;
    socklen_t m_destAddrLen;

public:
	TCP_Client(std::string serverHost, uint16_t port);

    //bool recv_data(sockaddr* srcAddr = NULL, socklen_t* addrLen = NULL) override;
    bool send_data(sockaddr* srcAddr = NULL, socklen_t addrLen = 0) override;
	bool handshake() override;
	//bool sendData(uint8_t* data) override;
	//bool receiveData() override;
	//bool setTimeout(float sec, float usec, bool flag) override;

	// Accessors
	//int getSocketFD() const { return m_sockFD; }
	std::string getServerHost() const { return m_serverHost; }

};

#endif // TCP_H
