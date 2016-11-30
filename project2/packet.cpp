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
}

// Grab the first 8 bytes and decode them into the header
// Store the rest in data
TCP_Packet::TCP_Packet(uint8_t* enc_stream, int enc_size){
	if(enc_stream){
		m_header.decode(enc_stream);
	}
	// Clear out the data currently stored
	m_data.clear();
	// Store the data data stream in m_data
	for(ssize_t i = HEADER_SIZE; i < enc_size; i++){
		m_data.push_back(enc_stream[i]);
	}
}

TCP_Packet::~TCP_Packet(){
  if(m_encoded_packet){
    delete m_encoded_packet;
  }
}

bool TCP_Packet::hasTimedOut(){
	// Get the current time and a place to store the diff
	struct timeval diff, now;
	gettimeofday(&now, NULL);
	timersub(&m_time_sent, &now, &diff);
	// If the difference is > 500 ms then we timed out
	if((diff.tv_sec*1000) + (diff.tv_usec/1000) > RTO){
		return true;
	}
	return false;
}

// Encode into byte array
uint8_t* TCP_Packet::encode(){
	// Find the current total size of the packet
	ssize_t total_size = m_data.size() + HEADER_SIZE;
	uint8_t* m_encoded_packet = new uint8_t[total_size];
	// Header encoding - nullptr returned on error
	if(!m_header.encode(m_encoded_packet)) { return nullptr; }
	// The first 8 bytes have been encoded, encode the data
	for(ssize_t i = HEADER_SIZE; i < total_size; i++){
		m_encoded_packet[i] = m_data[i-HEADER_SIZE];
	}
	return m_encoded_packet;
}

// Shallow copy the data into the member data
bool TCP_Packet::setData(char* data, int data_size){
	if(data){
		// Clear out the old data
		m_data.clear();
		// Set to new data
		for(ssize_t i = 0; i < data_size; i++){
			m_data.push_back((uint8_t) data[i]);
		}
		return true;
	}
	return false;
}

