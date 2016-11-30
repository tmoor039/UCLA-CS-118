#ifndef TCP_H
#define TCP_H

#include "globals.h"
#include "packet.h"
#include <fstream>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <vector>
#include "congestion_control.h"

// Abstract base TCP class
class TCP {

protected:
	uint16_t m_port;
	TCP_Packet* m_packet;
	uint8_t m_recvBuffer[MSS];
	uint8_t m_sendBuffer[MSS];
	bool m_status = true;
  int m_recvSize;

public:
	TCP(uint16_t port): m_port(port) {};
	virtual ~TCP(){
		if(m_packet){
			delete m_packet;
		}
	}
	// Pure virtual methods
	virtual bool handshake() = 0;
	virtual bool sendData(uint8_t* data, ssize_t data_size = MSS) = 0;
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
	// File name
	std::string m_filename;
	// File Stream
	std::ifstream m_file;
	// Socket information
	struct sockaddr_in m_serverInfo, m_clientInfo;
	socklen_t m_cliLen = sizeof(m_clientInfo);
	// Server Socket FD
	int m_sockFD;
	// CWND worth of packets from the file
    std::vector<TCP_Packet> m_filePackets;
    // index for the oldest packet that has not yet been acked.
    int m_basePacket;
    // index for the next packet within the window that is ready to be sent.
    int m_nextPacket;
    // in units of PACKET_SIZE
    int m_cwnd;
    // The sequence number of the first usable but not yet sent packet.
    // In units of bytes.
    uint16_t m_nextSeq;
    // The sequence number of the first packet sent but not yet acked.
    // In units of bytes.
    uint16_t m_baseSeq;
    // The size of the file
    ssize_t m_bytes;
    CCMode m_CCMode;

    // Keeps track of the CC algorithm and the congestion window.
    // CongestionControl* CC; 
    
public:
	TCP_Server(uint16_t port, std::string filename);

	bool handshake() override;
	bool sendData(uint8_t* data, ssize_t data_size = MSS) override;
	bool receiveData() override;
	bool setTimeout(float sec, float usec, bool flag) override;

	// Grab chunks from the file and push to m_filePackets.
	// By default only get 1 chunk
	bool grabChunk(ssize_t num_chunks=1);
	
	// Remove acked packets from m_filePackets and return number of removed
	// packets
	ssize_t removeAcked();

    // send packet corresponding to m_nextSeq if it is within send window
    bool sendNextPacket();

    // call break_file before using this function
    bool sendFile();

	// Accessors
	int getSocketFD() const { return m_sockFD; }
	std::string getFilename() const { return m_filename; }

	// Mutators
	void setFilename(std::string filename) { m_filename = filename; }
    void setCCMode(CCMode ccmode) { m_CCMode = ccmode; }

    // returns index of the file packets based on sequence number
    int seq2index(uint16_t seq);

    // returns sequence number based on index of file packets
    uint16_t index2seq(int index);

    // Marks the corresponding file packet as marked and returns the ack.
    // Call after reading data to receive buffer
    uint16_t receiveAck();
    
    // Test function
    bool testWrite();
};

// TCP Client
class TCP_Client: TCP {
	std::string m_serverHost;
	struct sockaddr_in m_serverInfo;
	socklen_t m_serverLen = sizeof(m_serverInfo);
	int m_sockFD;
    uint16_t m_expected_seq;

public:
	TCP_Client(std::string serverHost, uint16_t port);

	bool handshake() override;
	bool sendData(uint8_t* data, ssize_t data_size = MSS) override;
	bool receiveData() override;
    bool receiveFile();
	bool setTimeout(float sec, float usec, bool flag) override;

	// Accessors
	int getSocketFD() const { return m_sockFD; }
	std::string getServerHost() const { return m_serverHost; }

};

#endif // TCP_H
