#include "globals.h"
#include "packet.h"
#include <string.h>
#include <string>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <vector>

// Abstract base TCP class
class TCP {

protected:
	uint16_t m_port;
	TCP_Packet* m_packet;
	uint8_t m_recvBuffer[MSS];
	uint8_t m_sendBuffer[MSS];
	bool m_status = true;

public:
	TCP(uint16_t port): m_port(port) {};
	virtual ~TCP(){
		if(m_packet){
			delete m_packet;
		}
	}
	// Pure virtual methods
	virtual bool handshake() = 0;
	virtual bool sendData(uint8_t* data) = 0;
	virtual bool receiveData() = 0;
	virtual bool setTimeout(float sec, float usec, bool flag) = 0;
	
	// Accessors
	uint16_t getPort() const { return m_port; }
	uint8_t* getReceivedPacket() { return m_recvBuffer; }
	uint8_t* getSentPacket() { return m_sendBuffer; }
	bool getStatus() const { return m_status; }
};

// TCP Server
class TCP_Server: TCP {
	std::string m_filename;
	struct sockaddr_in m_serverInfo, m_clientInfo;
	socklen_t m_cliLen = sizeof(m_clientInfo);
	int m_sockFD;
    std::vector<TCP_Packet> m_filePackets;
    uint16_t m_sendBase;
    // index for the oldest packet that has not yet been acked.

    int m_nextPacket;
    // index for the next packet within the window that is ready to be sent.

    uint16_t m_cwnd;
    // starts at 1024. The number of packets that can be pushed through the
    // socket simultaneously.

    uint16_t m_nextSeq;
    // The sequence number of the first usable but not yet sent packet.

public:
	TCP_Server(uint16_t port);

	bool handshake() override;
	bool sendData(uint8_t* data) override;
	bool receiveData() override;
	bool setTimeout(float sec, float usec, bool flag) override;

	bool breakFile();
    // Break file into packets. The file packets are in m_filePackets.

    bool send_next_packet();
    // send packet corresponding to m_nextSeq if it is within send window

    void send_file();
    // call break_file before using this function

	// Accessors
	int getSocketFD() const { return m_sockFD; }
	std::string getFilename() const { return m_filename; }

	// Mutators
	bool setFilename(std::string filename) { m_filename = filename; }

};

// TCP Client
class TCP_Client: TCP {
	std::string m_serverHost;
	struct sockaddr_in m_serverInfo;
	socklen_t m_serverLen = sizeof(m_serverInfo);
	int m_sockFD;

public:
	TCP_Client(std::string serverHost, uint16_t port);

	bool handshake() override;
	bool sendData(uint8_t* data) override;
	bool receiveData() override;
	bool setTimeout(float sec, float usec, bool flag) override;

	// Accessors
	int getSocketFD() const { return m_sockFD; }
	std::string getServerHost() const { return m_serverHost; }

};

