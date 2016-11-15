#include "packet.h"
#include <iostream>


TCP_Packet::TCP_Packet(uint16_t seq, uint16_t ack, uint16_t win, 
    uint16_t flags) {

    memcpy(&m_header[0], (uint8_t *) &seq, 2);
    memcpy(&m_header[2], (uint8_t *) &ack, 2);
    memcpy(&m_header[4], (uint8_t *) &win, 2);
    memcpy(&m_header[6], (uint8_t *) &flags, 2);    

    memset(m_data, '\0', PACKET_DATA_SIZE);
}

TCP_Packet::TCP_Packet(uint16_t seq, uint16_t ack, uint16_t win, 
    uint16_t flags, uint8_t* data, int len) {

    memcpy(&m_header[0], (uint8_t *) &seq, 2);
    memcpy(&m_header[2], (uint8_t *) &ack, 2);
    memcpy(&m_header[4], (uint8_t *) &win, 2);
    memcpy(&m_header[6], (uint8_t *) &flags, 2);    

    memset(m_data, '\0', PACKET_DATA_SIZE);
    insert_data(data, len);
}

TCP_Packet::TCP_Packet(uint8_t* packetData) {
    memcpy(m_header, packetData, PACKET_HEADER_SIZE);
    memset(m_data, '\0', PACKET_DATA_SIZE);
    memcpy(m_data, &packetData[PACKET_HEADER_SIZE], PACKET_DATA_SIZE);
}

bool TCP_Packet::insert_data(uint8_t data[], len) {
    if (len > PACKET_DATA_SIZE) {
        fprintf(stderr, "cannot insert %d bytes to packet\n");
        return false;
    }
    memcpy(m_data, data, len);
    return true;
}


// accessors:
uint8_t* TCP_Packet::get_header() {
    return m_header;
}

uint8_t* TCP:Packet::get_data() {
    return m_data;
}

uint16_t TCP_Packet::get_seq() {
    uint16_t seq;
    seq = (m_header[0] << 8) | (m_header[1]);
    return seq;
}

uint16_t TCP_Packet::get_ack() {
    uint16_t ack;
    ack = (m_header[2] << 8) | (m_header[3]);
    return ack;
}

uint16_t TCP_Packet::get_win() {
    uint16_t win;
    win = (m_header[4] << 8) | (m_header[5]);
    return win;
}

uint16_t TCP_Packet::get_flags() {
    uint16_t flags;
    flags = (m_header[6] << 8) | (m_header[7]);
    return flags;
}

/*
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
*/
