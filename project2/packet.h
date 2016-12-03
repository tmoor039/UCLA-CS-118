#include "globals.h"
#include <sys/types.h>
#include <vector>
#include <sys/time.h>
#include "stdint.h"
#include <iostream>

using namespace std;

class TCP_Packet {
    // 8 byte header
	struct TCP_Header {
		uint16_t fields[NUM_FIELDS] = {0};
		// Set Flags with the three LSB's holding the information
		void setFlags(bool A, bool S, bool F) { fields[FLAGS] |= F | (S << 1) | (A << 2); }
		void setCWND(uint16_t cwnd) { fields[WIN] = cwnd; }
        // XDR break up ignoring Endianness
		bool encode(uint8_t* enc) {
			// Break 2 byte values by high and low byte fields
			if(enc){
				for(ssize_t i = 0; i < HEADER_SIZE; i+=2){
					enc[i] = fields[i/2] & 0xFF;
					enc[i+1] = fields[i/2] >> 8;
				}
				return true;
			}
			return false;
		}
        // Merge the broken bytes into 2 bytes
		bool decode(uint8_t* dec){
			if(dec){
				for(ssize_t i = 0; i < HEADER_SIZE; i+=2){
					fields[i/2] = (dec[i+1] << 8) | dec[i];
				}
				return true;
			}
			return false;
		}

	} m_header;
    // Stores the variable data
	std::vector<uint8_t> m_data; 
    // Stores the data as encoded stream
    uint8_t* m_encoded_packet = nullptr;
    int m_enc_count = 0;
    // Used to know when packet was sent
	struct timeval m_time_sent;

    // Mark packet as sent and acked as necessary
    // Packets by default arent acked or sent
    bool m_sent = false;
    bool m_acked = false;

public:
	// Single constructor with optional Data
	TCP_Packet(uint16_t seq, uint16_t ack, uint16_t win, bool f_ack, bool f_syn,
			bool f_fin, uint8_t* data = nullptr, ssize_t data_size = PACKET_SIZE);
	// Constructor that decodes data stream into TCP Packet
	TCP_Packet(uint8_t* enc_stream, int enc_size = MSS);
    // Copy constructor
    TCP_Packet(const TCP_Packet& other);
    // Assignment operator
    TCP_Packet& operator=(const TCP_Packet& other);
    // Destructor to remove any heap allocated objects
    ~TCP_Packet();
	// Accessors
    std::vector<uint8_t>* getData() { return &m_data; }
    uint8_t* getEncoded() { return m_encoded_packet; }
    TCP_Header getHeader() { return m_header; }
    bool isAcked() const { return m_acked; }
    bool isSent() const { return m_sent; }
    int getLength() const { return m_data.size() + HEADER_SIZE; }

    // Auxillary Methods
    uint8_t* encode();
    // Figures out if packet timed out based on given RTO
    bool hasTimedOut();
    // Starts the timer based on current time
    void startTimer() { gettimeofday(&m_time_sent, nullptr); }

    // Mutators
    bool setData(char* data, int data_size = PACKET_SIZE);
    void setAcked(); 
    void setSent();
};
