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

#define BUF_SIZE 1000

struct Connection {
  char buffer[BUF_SIZE + 1];
  int len;
  int sfd;
  struct sockaddr_in dest;
};

URL *createURL(std::string urlString) {
  URL *url = new URL;

  // Find the starting point after '://'
  bool foundFirst = false;
  int i = 0;
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
  std::string host = "";
  for (; i < urlString.length(); i++) {
    if (urlString[i] == ':') {
      i++;
      break;
    }
    host += urlString[i];
  }
  url->host = host;
  if (i >= urlString.length()) {
    return NULL;
  }

  // Find port number
  std::string port = "";
  for (; i < urlString.length(); i++) {
    if (urlString[i] == '/') {
      break;
    }
    port += urlString[i];
  }
  url->port = atoi(port.c_str());

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

  for (int i = 0; i < data.size(); i++) {
    buf[i] = data[i];
  }

  if (send(connection->sfd, &buf, data.size(), 0) == -1) {
    perror("send");
    return 1;
  }

  return 0;
}

HttpResponse *getHttpResponse(Connection *connection) {
  bool isEnd = false;
  std::string input;
  uint8_t buf[BUF_SIZE] = {0};
  std::stringstream ss;
  std::vector<uint8_t> data;
  
  while (!isEnd) {
    memset(buf, '\0', sizeof(buf));

    int length = recv(connection->sfd, buf, BUF_SIZE, 0);
    if (length == -1) {
      perror("recv");
      return NULL;
    } else if (length == 0) {
      return NULL;      
    }
    for (int i = 0; i < BUF_SIZE; i++) {
      if (buf[i] == '\0') {
        break;
      }
      data.push_back(buf[i]);
    }
    ss << buf << std::endl;
    std::cout << "echo: ";
    std::cout << buf << std::endl;

    if (ss.str() == "close\n")
      break;

    ss.str("");
  }

  return NULL;
}

void closeConnection(Connection *connection) {
  close(connection->sfd);
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        fprintf(stderr, "Usage: %s [URL] [URL]...\n", argv[0]);
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
      URL *url = createURL(argv[i]);
      if (url == NULL) {
        fprintf(stderr, "Improper URL (%s)\n", argv[1]);
        exit(1);
      }

      Connection *connection = connectToURLHost(url);
      if (connection == NULL) {
        exit(1);
      }

      HttpRequest request(*url);

      int status;
      status = sendHttpRequest(&request, connection);
      if (status != 0) {
        exit(1);
      }

      HttpResponse *response = getHttpResponse(connection);
      if (response == NULL) {
        exit(1);
      }

      closeConnection(connection);
    } 

    return EXIT_SUCCESS;
}
