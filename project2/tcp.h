#include "globals.h"
#include <sys/types.h>
#include "stdint.h"

class TCP_Packet {
	struct TCP_Header {
		TCP_Header():
			ACK(0), SEQ(0), WIN(PACKET_SIZE), flags(0) {}
		uint16_t ACK, SEQ, WIN, flags;
		// Set Flags with the three LSB's holding the information
		void setFlags(bool A, bool S, bool F) { flags |= F | (S << 1) | (A << 2); }
	} m_header;
	uint8_t m_data[PACKET_SIZE] = {0};

public:
	// Single constructor with optional Data
	TCP_Packet(uint16_t ack, uint16_t seq, uint16_t win, bool f_ack, bool f_seq,
			bool f_fin, uint8_t* data = nullptr);
	// Accessors
	uint8_t* getData() { return m_data; }
	TCP_Header getHeader() { return m_header; }

	// Mutators
	bool setData(uint8_t* data);

};
