#ifndef GLOBALS_H
#define GLOBALS_H

// All numbers are in terms of bytes unless otherwise stated

#define PACKET_SIZE 1024 // Also Window Size
#define MSS 1032 // This is Packet + Header
#define MAX_SEQ 30720 // Reset SEQ back to 0 after exceeding
#define SSTHRESH 15360 // Also start value for receiver window size
#define RTO 500	// In ms

#endif // GLOBALS_H
