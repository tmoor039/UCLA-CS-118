#include "http.h"
#include <sstream>
#include <fstream>
#include <iterator>
#include <thread>
#include <chrono>
using namespace std;

#include <sys/stat.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/time.h>


#define BUF_SIZE 1000

#define HOST_NAME 1
#define PORT 2 
#define FILE_DIR 3

//Global Timeout limit
unsigned short TIMEOUT = 0;

vector<string> parse(string decoded_message){
	vector<string> tokens;
	size_t msg_size = decoded_message.size();
	string word = "";
	for(size_t i = 0; i < msg_size; i++){
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

bool binder(int& sock_fd, struct addrinfo* my_addr, int addr_len){
	if(::bind(sock_fd, (struct sockaddr*)& *my_addr, addr_len) == -1){
		perror("Error in binding socket!");
		return false;
	}
	return true;
}

bool listener(int& sock_fd){
	if(listen(sock_fd,1) == -1){
		perror("Error initiating listening!");
		return false;
	}
	return true;
}

unsigned short string_to_short(string input){
	int i = atoi(input.c_str());
	if(i > USHRT_MAX){
		perror("Error, invalid port given!");
		exit(1);
	}
	return (unsigned short)i;
}

bool bad_request(vector<string>& request) {
	if (request[0].compare("GET") != 0) {
		return true;
	}   
	else if (request[2].compare("HTTP/1.0") != 0 && request[2].compare("HTTP/1.1") != 0) {
		return true;
	}
	return false;
}

// Receive data from file descriptor fd and save data in vector.
bool receive_data(int fd, vector<uint8_t>& data) {
	char buf[BUF_SIZE];
	bool isEnd = false;
  int count = 0;
	while(!isEnd) {
		memset(buf, '\0', sizeof(buf));

		int length = recv(fd, buf, BUF_SIZE, 0x20000);
		if (length == -1) {
			usleep(10000);
      continue;
		}        
		// finished receiving file
		else if (length == 0) {
      break;
    }
    for (int i = 0; i < length; i++) {
      // Carriage return
      if (buf[i] == '\r') {
        if (count == 2) {
          count = 3;
        } else {
          count = 1;
        }
      }

      // Newline
      else if (buf[i] == '\n') {
        if (count == 1) {
          count = 2;
        } else if (count == 3) {
          return true;
        } else {
          count = 0;
        }
      }

      else {
        count = 0;
      }

      data.push_back(buf[i]);
    }
  }
  return true;
} 

string make_fullpath(string file_dir, string req_obj){
  if (!file_dir.empty() and file_dir[file_dir.length() - 1] != '/') {
    file_dir += '/';
  } 
  if(req_obj == "/"){
    req_obj = "/index.html";
  }
  return file_dir + req_obj.substr(1, string::npos);
}

bool grab_file_data(vector<uint8_t>& data, string filename){
  struct stat info;
  if(stat(filename.c_str(), &info) == 0){
    if(info.st_mode & S_IFDIR){
      perror("Can't send a directory!");
      return false;
    }
  }

  ifstream file(filename, ios::binary);
  if(file.fail()){
    perror("Error opening file!");
    return false;
  }
  file.unsetf(ios::skipws);

  streampos file_size;
  file.seekg(0, ios::end);
  file_size = file.tellg();
  file.seekg(0, ios::beg);

  data.reserve(file_size);

  data.insert(data.begin(), istream_iterator<uint8_t>(file), istream_iterator<uint8_t>());

  return true;
}

int send_data(int& sock_fd, HttpResponse* resp){
  vector<uint8_t> enc_data = resp->encode();
  size_t data_size = enc_data.size();
  uint8_t* buf = new uint8_t[data_size];
  for(size_t i = 0; i < data_size; i++){
    buf[i] = enc_data[i];
  }
  if(send(sock_fd, buf, data_size, 0) == -1){
    perror("Error sending data!");
    return -1;
  }
  delete[] buf;
  return 1;
}

int main(int argc, char* argv[]) {

  string hostname, my_port, filedir;

  // require 0 or 3 arguments
  if (argc != 1 && argc != 4) {
    fprintf(stderr, "Usage: %s [hostname] [port] [file-dir]\n", argv[0]);
    return -1;
  }

  if (argc == 4) {
    hostname = argv[HOST_NAME];
    my_port = argv[PORT];
    filedir = argv[FILE_DIR];
  } else {  // default arguments
    hostname = "localhost";
    my_port = "4000";
    filedir = ".";
  }

  // Asynchronous server
  fd_set readFds;
  fd_set errFds;
  fd_set watchFds;
  FD_ZERO(&readFds);
  FD_ZERO(&errFds);
  FD_ZERO(&watchFds);
  // Create a socket file descriptor for IPv4 and TCP
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  int max_sock_fd = socket_fd;

  // Put the socket in the socket set
  FD_SET(socket_fd, &watchFds);

  int reconnect = 1;
  if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reconnect, sizeof(int)) == -1) {
    perror("Error making socket reusable!");
    return -1;
  }

  URL host_url("", hostname, (unsigned)stoi(my_port));

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(string_to_short(my_port));
  if(host_url.resolveDomain() == ""){
    perror("Error resolving domain name!");
    return -1;
  }
  addr.sin_addr.s_addr = inet_addr((host_url.resolveDomain()).c_str());
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  if(!binder(socket_fd, (struct addrinfo*)& addr, sizeof(addr))) { return -1; }
  if(!listener(socket_fd)) { return -1; }

  struct timeval tiv;
  // Store the client's IP
  char ipstr[INET_ADDRSTRLEN] = {'\0'};
  // Store the client's port
  unsigned short port = 0;
  while(1){
    // Set up watcher
    int n_ready_fds = 0;
    readFds = watchFds;
    errFds = watchFds;
    tiv.tv_sec = 3;
    tiv.tv_usec = 0;

    if((n_ready_fds = select(max_sock_fd + 1, &readFds, NULL, &errFds, &tiv)) == -1){
      perror("Error in select");
      return -1;
    }
    if(n_ready_fds == 0){
      cout << "No data received for " + to_string(tiv.tv_sec) + " seconds!" << endl;
    } else {
      for(int fd = 0; fd <= max_sock_fd; fd++){
        // Get a socket for reading
        if(FD_ISSET(fd, &readFds)) {
          // This is the socket used for listening
          if(fd == socket_fd){
            struct sockaddr_in clientAddr;
            socklen_t clientAddrSize = sizeof(clientAddr);
            int clientFd = accept(socket_fd, (struct sockaddr*)& clientAddr, &clientAddrSize);
            if(clientFd == -1) { continue; }
            port = ntohs(clientAddr.sin_port);
            inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
            cout << "Accepted a connection from: " << ipstr << ":" << port << endl;

            // Update max socket fd
            if(max_sock_fd < clientFd) { max_sock_fd = clientFd; }

            // Add the socket to the socket set
            FD_SET(clientFd, &watchFds);
          } else {
            // Normal socket stuff
            int curr_cli_fd = fd;
            struct timeval start, current;
            gettimeofday(&start, NULL);
            current = start;
            do{
              vector<uint8_t> requestMessage;
              // Get the HTTP Receivequest
              if(!receive_data(curr_cli_fd, requestMessage)) { continue; }
              // Decode the Request
              vector<string> header = decode(requestMessage);
              if (header.empty()) {
                break;
              }
              string filename = make_fullpath(filedir, header[1]);
              // Check for timeout
              if(TIMEOUT == 0) { TIMEOUT = string_to_short(header[8]); }
              vector<uint8_t> data;
              bool good_get = true;
              HttpResponse* response;
              // Check for a bad request
              if(bad_request(header)){
                response = new HttpResponse(400, "HTTP/1.1", data);
                good_get = false;
              }
              // Try to get the data requested
              else if (!grab_file_data(data, filename)){
                response = new HttpResponse(404, "HTTP/1.1", data);
                good_get = false;
              } else {
                response = new HttpResponse(200, "HTTP/1.1", data);
              }
              // Try to send the data
              int send_status = send_data(curr_cli_fd, response);
              if(send_status == 1 && good_get){
                cout << "Sent the file: " << filename << " to " << ipstr << ":" << port << endl;
              } 
              // If the send was bad, just retry after 1 second
              else if (send_status == -1){
                this_thread::sleep_for(chrono::seconds(1));
                gettimeofday(&current, NULL);
                continue;
              } else {
                cerr << "Error sending file: " << filename << " to " << ipstr << ":" << port << endl; 
              }
              delete response;
              gettimeofday(&current, NULL);
            } while(start.tv_sec * 1000000 + start.tv_usec + TIMEOUT * 1000000 > current.tv_sec * 1000000 + current.tv_usec);
            cout << "Closed the connection with: " << ipstr << ":" << port << endl;
            close(curr_cli_fd);
            FD_CLR(curr_cli_fd, &watchFds);
          }
        }
      }
    }
  }
  return 0;
}
