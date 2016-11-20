#include "globals.h"
#include <sys/types.h>
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
				for(ssize_t i = 0; i < 2 * NUM_FIELDS; i+=2){
					enc[i] = fields[i/2] & 0xFF;
					enc[i+1] = fields[i/2] >> 8;
				}
				return true;
			}
			return false;
		}
		bool decode(uint8_t* dec){
			if(dec){
				for(ssize_t i = 0; i < 2*NUM_FIELDS; i+=2){
					fields[i/2] = (dec[i+1] << 8) | dec[i];
				}
				return true;
			}
			return false;
		}

	} m_header;
	uint8_t m_data[PACKET_SIZE] = {0};

    // mark packet as sent and acked as necessary
    bool m_sent;
    bool m_acked;

public:
	// Single constructor with optional Data
	TCP_Packet(uint16_t seq, uint16_t ack, uint16_t win, bool f_ack, bool f_syn,
			bool f_fin, uint8_t* data = nullptr);
	// Constructor that decodes data stream into TCP Packet
	TCP_Packet(uint8_t* enc_stream);
	// Accessors
	uint8_t* getData() { return m_data; }
	TCP_Header getHeader() { return m_header; }
    bool is_acked() { return m_acked; }
    bool is_sent() { return m_sent; }

	uint8_t* encode();

	// Mutators
	bool setData(char* data);
    void set_acked() { m_acked = true; }
    void set_sent() { m_sent = true; }
};
