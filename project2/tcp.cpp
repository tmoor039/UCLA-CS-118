#include "tcp.h"

TCP_Packet::TCP_Packet(uint16_t seq, uint16_t ack, uint16_t win, bool f_ack, 
		bool f_syn, bool f_fin, uint8_t* data){
	// Set Header Fields
	m_header.fields[SEQ] = seq;
	m_header.fields[ACK] = ack;
	m_header.fields[WIN] = win;
	m_header.setFlags(f_ack, f_syn, f_fin);
	// Set Data
	if(data){
		for(ssize_t i = 0; i < PACKET_SIZE; i++){
			m_data[i] = data[i];
		}
	}
}

// Grab the first 8 bytes and decode them into the header
// Store the rest in data
TCP_Packet::TCP_Packet(uint8_t* enc_stream){
	if(enc_stream){
		m_header.decode(enc_stream);
	}
	for(ssize_t i = 2*NUM_FIELDS; i < MSS; i++){
		m_data[i] = enc_stream[i];
	}
}

// Encode into byte array
uint8_t* TCP_Packet::encode(){
	uint8_t* encoded_packet = new uint8_t[MSS];
	// Header encoding - nullptr returned on error
	if(!m_header.encode(encoded_packet)) { return nullptr; }
	// The first 8 bytes have been encoded, encode the data
	for(ssize_t i = 2*NUM_FIELDS; i < MSS; i++){
		encoded_packet[i] = m_data[i];
	}
	return encoded_packet;
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

