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
    // Store whether we have received 3 Duplicate ACKs
    bool m_tri_dups = false;
    ssize_t m_num_acks = 0;

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
  bool isAcked() const { return m_acked; }
  bool isSent() const { return m_sent; }
  int getLength() const { return m_data.size() + HEADER_SIZE; }
  bool gotThreeDups() const { return m_tri_dups; }

  // Auxillary Methods
  uint8_t* encode();
  bool hasTimedOut();
  void startTimer() { gettimeofday(&m_time_sent, nullptr); }

  // Mutators
  bool setData(char* data, int data_size = PACKET_SIZE);
  void setAcked(); 
  void setSent();
};
