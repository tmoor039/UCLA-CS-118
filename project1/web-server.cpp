#include <iostream>
#include <vector>
#include <string>
using namespace std;

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/socket.h>
#include <netdb.h>

#include "http.h"

#define BUF_SIZE 1000

#define HOST_NAME 1
#define PORT 2 
#define FILE_DIR 3

vector<string> parse(string decoded_message){
	vector<string> tokens;
	char delimiters[] = " \r\n";
	char* tokenize = strtok(decoded_message.c_str(), tokens);
	while(tokenize != NULL){
		tokens.push_back(tokenize);
		tokenize = strtok(NULL, delimiters);
	}
	return tokens;
}

vector<string> decode(vector<uint8_t> d_http_req){
	string result = "";
	for(uint8_t byte : http_req){
		result += char(byte);
	}
	return parse(result);
}

void binder(int& sock_fd, struct sockaddr* my_addr, int addr_len){
	if(bind(sock_fd, my_addr, addr_len) == -1){
		perror("Error in binding socket!");
		exit(2);
	}
}

void listener(int& sock_fd){
	if(listen(sock_fd,1) == -1){
		perror("Error initiating listening!");
		exit(3);
	}
}

int acceptor(int& sock_fd, struct sockaddr* cli_addr, int* addrlen){
	int cli_fd = accept(socket_fd, cli_addr, addrlen);
	if(cli_fd == -1){
		perror("Error in accepting request!");
		exit(4);
	}
	return cli_fd;
}

short string_to_short(string input){
	int i = atoi(input.c_str());
	if(i > USHRT_MAX){
		perror("Error, invalid port given!");
		exit(5);
	}
	return (unsigned short)i;
}

int main(int argc, char* argv[]) {
    // Create a socket file descriptor for IPv4 and TCP
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    string hostname, port, filedir;

    // require 0 or 3 arguments
    if (argc != 1 && argc != 4) {
        fprintf(stderr, "Usage: %s [hostname] [port] [file-dir]\n", argv[0]);
        exit(1);
    }
    
    if (argc == 4) {
        hostname = argv[HOST_NAME];
        port = argv[PORT];
        filedir = argv[FILE_DIR];
    } else {  // default arguments
        hostname = "localhost";
        port = "4000";
        filedir = ".";
    }

	URL host_url("", hostname, (unsigned)stoi(port));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(string_to_short(port));
    if(host_url.resolveDomain() == ""){
    	perror("Error resolving domain name!");
    	exit(6);
	}
    addr.sin_addr.s_addr = inet_addr(host_url.resolveDomain());
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));
	
	binder(socket_fd, (struct addrinfo*)& addr, sizeof(addr));
	listener(socket_fd);
	
	struct sockaddr_in clientAddr;
	socklen_t clientAddrSize = sizeof(clientAddr);
	int client_fd = acceptor(socket_fd, (struct sockaddr*)&clientAddr, &clientAddrSize);

	char ipstr[INET_ADDRSTRLEN] = {'\0'};
	inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
	cout << "Accept a connection from: " << ipstr << ":" << ntohs

    // Let socket listen for incoming connections.
    if (listen(sfd, 1) == -1) {
        fprintf(stderr, "Failed to listen on socket %d\n", sfd);
        exit(1);
    }

    freeaddrinfo(result);  

    sockaddr_in clientAddr;
    socklen_t clientAddrSize;
    cfd = accept(sfd, (sockaddr *) &clientAddr, &clientAddrSize);
    if (cfd == -1) {
        fprintf(stderr, "Unable to accept connection\n");
    }
    // receive request message
    int nBytes = recv(sfd, buf, BUF_SIZE, 0);
    if (nBytes < 0) {
        fprintf(stderr, "Failed to receive message\n");
    }
    
    // save request message as vector
    vector<uint8_t> request;
    for (int i = 0; i < nBytes; i++) {
        request.push_back(buf[i]);
    }

    // decode request message
    vector<string> requestParsed = decode(request);

    string URL = requestParsed[1];
    
    // open the web page
    FILE* fp = fopen(URL, "r");
    
    // obtain file size
    fseek (fp , 0 , SEEK_END);
    int fsize = ftell (fp);
    rewind (fp);

    // allocate memory for data from web page and read data.
    char* data;
    data = new char[size];
    fread(data, 1, fsize, fp);    


}
