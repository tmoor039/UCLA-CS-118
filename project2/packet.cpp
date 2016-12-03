#include "packet.h"
#include <cstring>
#include <iostream>

using namespace std;

TCP_Packet::TCP_Packet(uint16_t seq, uint16_t ack, uint16_t win, bool f_ack, 
		bool f_syn, bool f_fin, uint8_t* data, ssize_t data_size)
{
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
TCP_Packet::TCP_Packet(uint8_t* enc_stream, int enc_size)
{
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

// Copy constructor to make deep copies
TCP_Packet::TCP_Packet(const TCP_Packet& other)
    : m_data(other.m_data), m_enc_count(other.m_enc_count), m_sent(other.m_sent), m_acked(other.m_acked)
    
{
    m_header = other.m_header;
    m_time_sent = other.m_time_sent;
    m_encoded_packet = new uint8_t[m_enc_count];
    // Deep copy of encoded packet
    for(ssize_t i = 0; i < m_enc_count; i++){
        m_encoded_packet[i] = other.m_encoded_packet[i];
    }
}

// Assignment operator to make deep copies
TCP_Packet& TCP_Packet::operator=(const TCP_Packet& other){
    if(this != &other){
        m_header = other.m_header;
        m_data = other.m_data;
        m_time_sent = other.m_time_sent;
        m_sent = other.m_sent;
        m_acked = other.m_acked;
        m_enc_count = other.m_enc_count;
        // Do the deep copy of encoded data
        if(other.m_encoded_packet){
            if(m_encoded_packet) { 
                delete[] m_encoded_packet;
                m_encoded_packet = nullptr;
            }
            m_encoded_packet = new uint8_t[m_enc_count];
            for(ssize_t i = 0; i < m_enc_count; i++){
                m_encoded_packet[i] = other.m_encoded_packet[i];
            }
        }
    }
    return *this;
}

TCP_Packet::~TCP_Packet(){
    // Delete encoded data
    if(m_encoded_packet){
        delete [] m_encoded_packet;
        m_encoded_packet = nullptr;
    }
}

bool TCP_Packet::hasTimedOut(){
    // Get the current time and a place to store the diff
    struct timeval now;
    gettimeofday(&now, nullptr);
    // If the difference is > 500 ms then we timed out
    if(((now.tv_sec*1000) + (now.tv_usec/1000)) - ((m_time_sent.tv_sec*1000) + (m_time_sent.tv_usec/1000)) > RTO){
        m_time_sent = now;
        return true;
    }
    return false;
}

// Encode into byte array
uint8_t* TCP_Packet::encode(){
    // Find the current total size of the packet
    ssize_t total_size = m_data.size() + HEADER_SIZE;
    m_enc_count = total_size;
    m_encoded_packet = new uint8_t[m_enc_count];
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

void TCP_Packet::setAcked(){
    m_acked = true;
}

void TCP_Packet::setSent() {
    m_sent = true;
}
