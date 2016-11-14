#include "tcp.h"

TCP_Packet::TCP_Packet(uint16_t ack, uint16_t seq, uint16_t win, bool f_ack, 
		bool f_seq, bool f_fin, uint8_t* data){
	// Set Header Fields
	m_header.ACK = ack;
	m_header.SEQ = seq;
	m_header.WIN = win;
	m_header.setFlags(f_ack, f_seq, f_fin);
	// Set Data
	if(data){
		for(ssize_t i = 0; i < PACKET_SIZE; i++){
			m_data[i] = data[i];
		}
	}
}

// Shallow copy the data into the member data
bool TCP_Packet::setData(uint8_t* data){
	if(data){
		for(ssize_t i = 0; i < PACKET_SIZE; i++){
			m_data[i] = data[i];
		}
		return true;
	}
	return false;
}

