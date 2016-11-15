#include "globals.h"
#include <sys/types.h>
#include "stdint.h"

class TCP_Packet {
public:
    // used for sending packets
    TCP_Packet(uint16_t seq, uint16_t ack, uint16_t win, uint16_t flags);
    TCP_Packet(uint16_t seq, uint16_t ack, uint16_t win, uint16_t flags,
        uint8_t* data, int len);

    // used for receiving packets
    TCP_Packet(uint8_t* encStream);

    bool insert_data(uint8_t data[], int len);

    // accessors:
    uint8_t* get_header();
    uint8_t* get_data();
    uint16_t get_seq();
    uint16_t get_ack();
    uint16_t get_win();
    uint16_t get_flags();

private:
    uint8_t m_header[PACKET_HEADER_SIZE];
    uint8_t m_data[PACKET_DATA_SIZE];
}
/*
class TCP_Packet {
    struct TCP_Header {
        uint16_t fields[NUM_FIELDS] = {0};
        // Set Flags with the three LSB's holding the information
        void setFlags(bool A, bool S, bool F) { fields[FLAGS] |= F | (S << 1) | (A << 2); }
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

    public:
    // Single constructor with optional Data
    TCP_Packet(uint16_t seq, uint16_t ack, uint16_t win, bool f_ack, bool f_syn,
            bool f_fin, uint8_t* data = nullptr);
    // Constructor that decodes data stream into TCP Packet
    TCP_Packet(uint8_t* enc_stream);
    // Accessors
    uint8_t* getData() { return m_data; }
    TCP_Header getHeader() { return m_header; }

    uint8_t* encode();

    // Mutators
    bool setData(uint8_t* data);

};
*/
