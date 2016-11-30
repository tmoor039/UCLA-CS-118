#include "globals.h"
#include <sys/types.h>
#include <vector>
#include <sys/time.h>
#include "stdint.h"

class TCP_Packet {
	struct TCP_Header {
		uint16_t fields[NUM_FIELDS] = {0};
		// Set Flags with the three LSB's holding the information
		void setFlags(bool A, bool S, bool F) { fields[FLAGS] |= F | (S << 1) | (A << 2); }
		void setCWND(uint16_t cwnd) { fields[WIN] = cwnd; }
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
	std::vector<uint8_t> m_data; 
  	uint8_t* m_encoded_packet = nullptr;
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
  // Destructor to remove any heap allocated objects
  ~TCP_Packet();
	// Accessors
  std::vector<uint8_t>* getData() { return &m_data; }
  TCP_Header getHeader() { return m_header; }
  bool isAcked() { return m_acked; }
  bool isSent() { return m_sent; }
  int getLength() { return m_data.size() + HEADER_SIZE; }

  uint8_t* encode();
  bool hasTimedOut();

  // Mutators
  bool setData(char* data, int data_size = PACKET_SIZE);
  void startTimer() { gettimeofday(&m_time_sent, NULL); }
  void setAcked() { m_acked = true; }
  void setSent() { m_sent = true; }
};
