#include "tcp.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

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
		fprintf(stderr, "Usage: %s <server-host-or-ip> <port-number>\n", argv[0]);
		exit(1);
	}
	uint16_t serverPort;
	if(!strToUInt(argv[2], &serverPort)){
		fprintf(stderr, "Invalid Port! Enter a 2 byte port value.\n");
		exit(1);
	}

	TCP_Client tcpClient(string(argv[1]), serverPort);

	if(!tcpClient.handshake()){
		fprintf(stderr, "TCP Handshake Failure\n");
		exit(1);
	}

  if(!tcpClient.receiveFile()){
    fprintf(stderr, "The client failed to receive the file.\n");
    exit(1);
  }

	return 0;
	
}
