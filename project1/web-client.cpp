#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define BUF_SIZE 1000

int sock = -1;
int port = 4000;
string address = "";

int main(int argc, char* argv[]) {

    string *urls;

    // require more than 1 argument
    if (argc == 1) {
        fprintf(stderr, "Usage: %s [URL] [URL]...\n", argv[0]);
        exit(1);
    }

    urls = new string[argc - 1];
    for (int i = 1; i < argc; i++) {
      urls[i - 1] = argv[i];
    } 

    char buffer[BUF_SIZE + 1];
    int len, mysocket;
    struct sockaddr_in dest;

    mysocket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    dest.sin_port = htons(port);

    connect(mysocket, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));

    len = recv(mysocket, buffer, BUF_SIZE, 0);

    buffer[len] = '\0';

    printf("Received %s (%d bytes).\n", buffer, len);

    close(mysocket);
    return EXIT_SUCCESS;
}
