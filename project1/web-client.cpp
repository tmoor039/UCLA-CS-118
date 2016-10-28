#include <iostream>
using namespace std;

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
#include <netdb.h>

#define BUF_SIZE 1000

struct Connection {
  char buffer[BUF_SIZE + 1];
  int len;
  int sfd;
  struct sockaddr_in dest;
};

URL *createURL(string urlString) {
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
  string host = "";
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
  string port = "";
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
  in_addr *ip = (in_addr *) gethostbyname(url->host.c_str())->h_addr;
  url->ip = inet_ntoa(* ip);

  return url;
}

Connection connectToURLHost(URL *url) {
  Connection connection;

  connection.sfd = socket(AF_INET, SOCK_STREAM, 0);

  memset(&connection.dest, 0, sizeof(connection.dest));

  connection.dest.sin_family = AF_INET;
  connection.dest.sin_addr.s_addr = inet_addr(url->ip.c_str());
  connection.dest.sin_port = htons(url->port);

  connect(connection.sfd, (struct sockaddr *) &connection.dest, sizeof(struct sockaddr_in));
    
  connection.len = recv(connection.sfd, connection.buffer, BUF_SIZE, 0);
  connection.buffer[connection.len] = '\0';
  printf("Received %s (%d bytes).\n", connection.buffer, connection.len);

  return connection;
}

void sendHttpRequest(HttpRequest &request, Connection &connection) {
}

void getHttpResponse(Connection &connection) {
}

void closeConnection(Connection &connection) {
  close(connection.sfd);
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
      Connection connection = connectToURLHost(url);
      HttpRequest request(*url);
      sendHttpRequest(request, connection);
      getHttpResponse(connection);
      closeConnection(connection);
    } 

    return EXIT_SUCCESS;
}
