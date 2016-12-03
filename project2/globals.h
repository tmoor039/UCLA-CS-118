#ifndef GLOBALS_H
#define GLOBALS_H

// All numbers are in terms of bytes unless otherwise stated
#define PACKET_SIZE 1024 // Also Window Size
#define PACKET_DATA_SIZE 1024
#define HEADER_SIZE 8
#define MSS 1032 // This is Packet + Header
#define MAX_SEQ 30720 // Reset SEQ back to 0 after exceeding
#define SSTHRESH 15360 // Also start value for receiver window size
#define START_WINDOW 15360 // Also start value for receiver window size
#define RTO 500 // In ms
#define RECEIVED_FILE_NAME "received.data"
// Macros for TCP Header
#define SEQ 0
#define ACK 1
#define WIN 2
#define FLAGS 3
#define NUM_FIELDS 4
// Macros for congestion control
#define SS 0
#define CA 1

#define MIN_CWND 1024

#endif // GLOBALS_H
