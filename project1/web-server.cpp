#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iterator>
using namespace std;

#include <sys/types.h>
#include <stdio.h>
#include <arpa/inet.h>
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
	size_t msg_size = decoded_message.size();
	string word = "";
	for(int i = 0; i < msg_size; i++){
		if(decoded_message[i] != ' ' && decoded_message[i] != '\r'
		   && decoded_message[i] != '\n'){
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
	for(uint8_t byte : d_http_req){
		result += (char)byte;
	}
	return parse(result);
}

void binder(int& sock_fd, struct addrinfo* my_addr, int addr_len){
	if(::bind(sock_fd, (struct sockaddr*)& *my_addr, addr_len) == -1){
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

int acceptor(int& sock_fd, struct sockaddr* cli_addr, socklen_t* addrlen){
	int cli_fd = accept(sock_fd, cli_addr, addrlen);
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

// Receive data from file descriptor fd and save data in vector.
vector<uint8_t> receive_data(int fd) {
    char buf[BUF_SIZE];
    vector<uint8_t> data;
    bool isEnd = false;
	while(!isEnd) {
        memset(buf, '\0', sizeof(buf));

        int length = recv(fd, buf, BUF_SIZE, 0);
        if (length == -1) {
            perror("recv error\n");
            exit(1);
        }        
        // finished receiving file
        else if (length == 0) {
            break;
        }
        for (int i = 0; i < BUF_SIZE; i++) {
            if (buf[i] == '\0') {
                isEnd = true;
                break;
            }
            data.push_back(buf[i]);
        }
	}
    return data;
} 

string make_fullpath(string file_dir, string req_obj){
	return file_dir + req_obj.substr(0, string::npos);
}

int grab_file_data(vector<uint8_t>& data, string filename){
	ifstream file(filename);
	if(file.fail()){
		perror("Error opening file!");
		return -1;
	}
	ostringstream ss;
	ss << file.rdbuf();
	const string& s = ss.str();
	vector<char> vec(s.begin(), s.end());
	// Use for testing by outputting the contents of file to stdout
	// std::copy(vec.begin(), vec.end(), std::ostream_iterator<char>(std::cout));
	for(char c : vec){
		data.push_back((uint8_t)c);
	}
	return 1;
}

void send_data(int& sock_fd, HttpResponse* resp){
	vector<uint8_t> enc_data = resp->encode();
	size_t data_size = enc_data.size();
	uint8_t* buf = new uint8_t[data_size];
	for(int i = 0; i < data_size; i++){
		buf[i] = enc_data[i];
	}
	if(send(sock_fd, buf, enc_data.size(), 0) == -1){
		perror("Error sending data!");
		exit(7);
	}
	delete[] buf;
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
    addr.sin_addr.s_addr = inet_addr((host_url.resolveDomain()).c_str());
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));
	
	binder(socket_fd, (struct addrinfo*)& addr, sizeof(addr));
	listener(socket_fd);
	
	struct sockaddr_in clientAddr;
	socklen_t clientAddrSize = sizeof(clientAddr);
	int client_fd = acceptor(socket_fd, (struct sockaddr*)&clientAddr, &clientAddrSize);

	char ipstr[INET_ADDRSTRLEN] = {'\0'};
	inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
	cout << "Accept a connection from: " << ipstr << ":" << ntohs(clientAddr.sin_port) << endl;

    vector<uint8_t> requestMessage = receive_data(client_fd);
    vector<string> header = decode(requestMessage);
    string filename = make_fullpath(filedir, header[1]);
    vector<uint8_t> data;
    HttpResponse* response;
	if(grab_file_data(data, filename) == -1){
		response = new HttpResponse(404, data);
	} else {
		response = new HttpResponse(200, data);
	}
    cout << filename << endl;
	send_data(client_fd, response);
}
