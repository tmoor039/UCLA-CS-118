#include "tcp.h"
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

using namespace std;

bool strToUInt(const char* str, uint16_t* res){
	long int val = strtol(str, NULL, 10);
	if(errno == ERANGE || val > UINT16_MAX || val <0)
		return false;
	*res = (uint16_t)val;
	return true;
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <port-number> <file-name>\n", argv[0]);
		exit(1);
	}
	uint16_t port;
	if(!strToUInt(argv[1], &port)){
		fprintf(stderr, "Invalid Port! Enter a 2 byte port value.\n");
		exit(1);
	}
	string filename = argv[2];

	TCP_Server tcpServer(port);
	tcpServer.setFilename(filename);

	if (tcpServer.handshake()){
		fprintf(stderr, "The TCP handshake failed\n");
		exit(1);
	}

	return 0;
}
