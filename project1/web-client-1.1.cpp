#include <iostream>

#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <fcntl.h>
#include <sys/time.h>

#define BUF_SIZE 1000
#define TIMEOUT 10

struct Connection {
  char buffer[BUF_SIZE + 1];
  int len;
  int sfd;
  struct sockaddr_in dest;
};

URL *createURL(std::string urlString) {
  URL *url = new URL;

  url->url = urlString;
  url->port = 80;

  // Find the starting point after '://'
  bool foundFirst = false;
  size_t i = 0;
  for (; i < urlString.length(); i++) {
    if (urlString[i] == '/') {
      if (foundFirst == true) {
        i++;
        break;
      }
      foundFirst = true;
    }
  }
  if (i >= urlString.length()) {
    return NULL;
  }

  // Find host name
  bool noPort = false;
  std::string host = "";
  for (; i < urlString.length(); i++) {
    if (urlString[i] == ':') {
      i++;
      break;
    } else if (urlString[i] == '/') {
      noPort = true;
      break;
    }
    host += urlString[i];
  }
  url->host = host;
  if (i >= urlString.length()) {
    return NULL;
  }

  // Find port number
  if (noPort == false) {
    std::string port = "";
    for (; i < urlString.length(); i++) {
      if (urlString[i] == '/') {
        break;
      }
      port += urlString[i];
    }
    url->port = atoi(port.c_str());
  }

  // Find object path
  url->object = urlString.substr(i, urlString.length() - i);

  // Find ip address of host
  std::string ip = "";
  ip = url->resolveDomain();
  if (ip == "") {
    return NULL;
  }
  url->ip = url->resolveDomain();

  return url;
}

Connection *connectToURLHost(URL *url) {
  Connection *connection = new Connection;

  connection->sfd = socket(AF_INET, SOCK_STREAM, 0);

  connection->dest.sin_family = AF_INET;
  connection->dest.sin_port = htons(url->port);
  connection->dest.sin_addr.s_addr = inet_addr(url->ip.c_str());
  memset(connection->dest.sin_zero, '\0', sizeof(connection->dest.sin_zero));

  if (connect(connection->sfd, (struct sockaddr *) &connection->dest, sizeof(connection->dest)) == -1) {
    perror("connect");
    return NULL;
  }

  return connection;
}

int sendHttpRequest(HttpRequest *request, Connection *connection) {
  std::vector<uint8_t> data = request->encode();
  uint8_t buf[data.size()];

  for (size_t i = 0; i < data.size(); i++) {
    buf[i] = data[i];
  }

  if (send(connection->sfd, &buf, data.size(), 0) == -1) {
    perror("send");
    return 1;
  }

  return 0;
}

int getHttpResponse(Connection *connection, URL *url) {
  uint8_t buf[BUF_SIZE] = {0};
  int count = 0, contentLength = 1000000000;
  bool isData = false, getCode = false, getMessage = false, getLength = false;
  std::string code = "", message = "", word = "";
  
  // Prepare output file
  std::string path = url->object;
  std::string name = path.substr(path.find_last_of("/") + 1);
  if (name.empty()) {
    name = "index.html";
  }
  std::ofstream outputFile;

  fcntl(connection->sfd, F_SETFL, O_NONBLOCK);

  struct timeval start, current;
  gettimeofday(&start, NULL);
  while (true) {
    gettimeofday(&current, NULL);
    if (current.tv_sec * 1000000 + current.tv_usec > start.tv_sec * 1000000 + start.tv_usec + TIMEOUT * 1000000) {
      perror("Connection timed out");
      return 1;
    }
    memset(buf, '\0', sizeof(buf));

    // Read from socket
    int length = recv(connection->sfd, buf, BUF_SIZE, 0);
    if (length == -1) {
      usleep(100000);
      continue;
    } else if (length == 0) {
      outputFile.close();
      return 0;
    }

    for (int i = 0; i < length; i++) {
      if (isData == false) {
        if (buf[i] == '\r' || buf[i] == '\n' || buf[i] == ' ' || buf[i] == '\0') {
          if (word.substr(0, 5) == "HTTP/") {
            getCode = true;
          }
          
          else if (getCode) {
            code = word;
            getCode = false;
            getMessage = true;
          } else if (getMessage) {
            while (buf[i] != '\r' && buf[i] != '\n') {
              word += buf[i];
              i++;
            }
            message = word;
            if (code != "200") {
              std::cout << url->url << ": " << code << " " << message << std::endl;
              return 1;
            }
            outputFile.open(name, std::ios::binary);
            getMessage = false;
          } else if (getLength) {
            contentLength = atoi(word.c_str());
            getLength = false;
          } else if (word == "Content-Length:") {
            getLength = true;
          }
          word = ""; 
        }
         
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
            isData = true;
          } else {
            count = 0;
          }
        }

        // Other non-data contents
        else if (buf[i] != ' ') {
          count = 0;
          word += buf[i];
        }
      }
      
      // Data
      else if (contentLength > 0) {
        outputFile << buf[i];
        contentLength--;
        if (contentLength == 0) {
          std::cout << url->url << ": " << code << " " << message << std::endl;
          outputFile.close();
          return 0;
        }
      }

      else {
        std::cout << url->url << ": " << code << " " << message << std::endl;
        outputFile.close();
        return 0;
      }
    }
  }
}

void closeConnection(Connection *connection) {
  close(connection->sfd);
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        fprintf(stderr, "Usage: %s [URL] [URL]...\n", argv[0]);
        exit(1);
    }
    
    std::unordered_map<std::string, Connection*> connections;
    std::vector<std::pair<Connection*, URL*>> requested;

    for (int i = 1; i < argc; i++) {
      URL *url = createURL(argv[i]);
      if (url == NULL) {
        fprintf(stderr, "Improper URL (%s)\n", argv[1]);
        continue;
      }

      Connection *connection;
      std::string key = url->ip + ":" + std::to_string(url->port);
      int error = 0;
      socklen_t size = sizeof(error);
      if (connections.find(key) == connections.end() || getsockopt(connections[key]->sfd, SOL_SOCKET, SO_ERROR, &error, &size) != 0) {
        connection = connectToURLHost(url);
        if (connection == NULL) {
          delete url;
          continue;
        }
        connections[key] = connection;
      } else {
        connection = connections[key];
      }

      HttpRequest request(*url, "HTTP/1.1", "GET");

      sendHttpRequest(&request, connection);

      getHttpResponse(connection, url);
    }

    for (auto i : connections) {
      int error = 0;
      socklen_t size = sizeof(error);
      if (getsockopt(i.second->sfd, SOL_SOCKET, SO_ERROR, &error, &size) == 0) {
        close(i.second->sfd);
      }
    }
    return EXIT_SUCCESS;
}
