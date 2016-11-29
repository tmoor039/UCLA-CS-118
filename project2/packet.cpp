#include "packet.h"

TCP_Packet::TCP_Packet(uint16_t seq, uint16_t ack, uint16_t win, bool f_ack, 
		bool f_syn, bool f_fin, uint8_t* data, ssize_t data_size){
	// Set Header Fields
	m_header.fields[SEQ] = seq;
	m_header.fields[ACK] = ack;
	m_header.fields[WIN] = win;
	m_header.setFlags(f_ack, f_syn, f_fin);
	// Set Data
	if(data){
		for(ssize_t i = 0; i < data_size; i++){
			m_data.push_back(data[i]);
		}
	}
    m_acked = false;
    m_sent = false;
}

// Grab the first 8 bytes and decode them into the header
// Store the rest in data
TCP_Packet::TCP_Packet(uint8_t* enc_stream, int enc_size){
	if(enc_stream){
		m_header.decode(enc_stream);
	}
	for(ssize_t i = 2*NUM_FIELDS; i < enc_size; i++){
		m_data.push_back(enc_stream[i]);
	}
}

TCP_Packet::~TCP_Packet(){
  if(m_encoded_packet){
    delete m_encoded_packet;
  }
}

// Encode into byte array
uint8_t* TCP_Packet::encode(){
	uint8_t* m_encoded_packet = new uint8_t[MSS];
	// Header encoding - nullptr returned on error
	if(!m_header.encode(m_encoded_packet)) { return nullptr; }
	// The first 8 bytes have been encoded, encode the data
	ssize_t header_size = 2*NUM_FIELDS;
	ssize_t total_size = m_data.size() + header_size;
	for(ssize_t i = header_size; i < total_size; i++){
		m_encoded_packet[i] = m_data[i-header_size];
	}
	return m_encoded_packet;
}

// Shallow copy the data into the member data
bool TCP_Packet::setData(char* data, int data_size){
	if(data){
		m_data.clear();
		for(ssize_t i = 0; i < data_size; i++){
			m_data.push_back((uint8_t) data[i]);
		}
		return true;
	}
	return false;
}

