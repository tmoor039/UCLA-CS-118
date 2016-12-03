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

// Abstract base TCP class
class TCP {
protected:
    // Port
	uint16_t m_port;
    // Packet used for setup and teardown
	TCP_Packet* m_packet;
    // Intermediary buffers for sending and receiving
	uint8_t m_recvBuffer[MSS];
	uint8_t m_sendBuffer[MSS];
    // Status for early construction error handling
	bool m_status = true;
    // Size of the data read from the socket
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
class TCP_Server: public TCP {
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
    float m_cwnd;
    // The sequence number of the first usable but not yet sent packet.
    // In units of bytes.
    uint16_t m_nextSeq;
    // The sequence number of the first packet sent but not yet acked.
    // In units of bytes.
    uint16_t m_baseSeq;
    // The size of the file
    ssize_t m_bytes;
    // Store the ssthresh of the window for the server
    // Note that ssthresh is in bytes, and our m_cwnd is in number of packets
    // Conversions are done in the functions
    ssize_t m_ssthresh = SSTHRESH;
    
	// Current mode for the congestion control
	uint8_t m_curr_mode = SS;
	
	// Private CongestionControl methods
	// Run Slow Start Algorithm
	bool runSlowStart(uint16_t ack);

	// Run Congestion Avoidance algorithm
	// Return true if we want to Fast Retransmit
	bool runCongestionAvoidance(uint16_t ack);

    // Last ack number received
    uint16_t m_last_ack = 0;

    // Number of duplicate acks received
    int m_duplicates_received = 0;

    // Initial Window value
    uint16_t m_window = START_WINDOW;

public:
	TCP_Server(uint16_t port, std::string filename);
	~TCP_Server();

	bool handshake() override;
	bool sendData(uint8_t* data, ssize_t data_size = MSS) override;
	bool receiveData() override;
	bool receiveDataNoWait();
	bool setTimeout(float sec, float usec, bool flag) override;

	// Grab chunks from the file and push to m_filePackets.
	// By default only get 1 chunk
	bool grabChunk(ssize_t num_chunks=1);

	// Remove acked packets from m_filePackets and return number of removed
	// packets
	bool removeAcked(int pos);

	// send packet corresponding to m_nextSeq if it is within send window
	bool sendNextPacket(ssize_t pos, bool resend);

	// call break_file before using this function
	bool sendFile();

	// Accessors
	int getSocketFD() const { return m_sockFD; }
	std::string getFilename() const { return m_filename; }

	// Mutators
	void setFilename(std::string filename) { m_filename = filename; }

	// returns index of the file packets based on sequence number
	int seq2index(uint16_t seq);

	// returns index of the file that should be sent based on the received ack number
	int ack2index(uint16_t ack);

	// Marks the corresponding file packet as marked and returns the ack.
	// Call after reading data to receive buffer
	int receiveAck();
};

// TCP Client
class TCP_Client: public TCP {
    // Destination IP
	std::string m_serverHost;
    // Destination IP info
	struct sockaddr_in m_serverInfo;
	socklen_t m_serverLen = sizeof(m_serverInfo);
    // Socket File Descriptor
	int m_sockFD;
    // Expected sequence used based on previous + data size
	uint16_t m_expected_seq;
    // Buffer to hold written packets, to prevent re-writes
    std::vector<uint16_t> m_written;

public:
	TCP_Client(std::string serverHost, uint16_t port);

	bool handshake() override;
	bool sendData(uint8_t* data, ssize_t data_size = MSS) override;
	bool receiveData() override;
    // Receive the file through out of order buffering
	bool receiveFile();
	bool setTimeout(float sec, float usec, bool flag) override;

	// Accessors
	int getSocketFD() const { return m_sockFD; }
	std::string getServerHost() const { return m_serverHost; }

};

#endif // TCP_H
