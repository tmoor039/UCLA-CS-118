#include <iostream>
#include <vector>
#include <string>
using namespace std;

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUF_SIZE 1000

#define HOST_NAME 1
#define PORT 2 
#define FILE_DIR 3

vector<string> parse(string decoded_message){
	vector<string> tokens;
	string word = "";
	for(int i = 0; i < decoded_message.size(); i++){
		if(decoded_message[i] != ' ' && decoded_message[i] != '\r' && decoded_message[i] != '\n'){
			word += decoded_message[i];
		} else {
			tokens.push_back(word);
			word = "";
		}
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

int main(int argc, char* argv[]) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, cfd;  // server file descriptor, client file descriptor
    char buf[BUF_SIZE];
    memset(&buf, 0, sizeof(char));
    
    string hostname;
    string port;
    string filedir;

    // require 0 or 3 arguments
    if (argc != 1 && argc != 4) {
        fprintf(stderr, "Usage: %s [hostname] [port] [file-dir]\n", argv[0]);
        exit(1);
    }
    
    if (argc == 4) {
        hostname = argv[HOST_NAME];
        port = argv[PORT];
        filedir = argv[FILE_DIR];
    }
    else {  // default arguments
        hostname = "localhost";
        port = "4000";
        filedir = ".";
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;  /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;  /* socket type used for TCP */
    hints.ai_flags = AI_PASSIVE;  /* returned socket address is used for 
                                    server */
    hints.ai_flags = 0;  /* socket addresses with any protocol can be 
                            returned by getaddrinfo() */    
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    int s;
    s = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(1);
    }
    
    /* Result is list of addrinfo structures. Try each address until
    we can create socket and bind. */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) {  // socket creation failed
            continue;
        }        
        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;  /* bind successful */
        }
        else {
            close(sfd);
        }
    }
    
    if (rp == NULL) {
        fprintf(stderr, "Failed to bind socket to address\n");
        exit(1);
    }

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
