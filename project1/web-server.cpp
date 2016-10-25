#include <iostream>
using namespace std;

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUF_SIZE 1000

int main(int argc, char* argv[]) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, cfd;  // server file descriptor, client file descriptor
    char buf[BUF_SIZE];
    
    string hostname;
    string port;
    string filedir;

    // require 1 or 3 arguments
    if (argc != 1 && argc != 4) {
        fprintf(stderr, "Usage: %s [hostname] [port] [file-dir]\n", argv[0]);
        exit(1);
    }
    
    if (argc == 4) {
        hostname = argv[1];
        port = argv[2];
        filedir = argv[3];
    }
    else {  // default arguments
        hostname = "localhost";
        port = "4000";
        filedir = "/tmp";
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
    s = getaddrinfo(hostname, port, &hints, &result);
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
    socketlen_t clientAddrSize;
    cfd = accept(sfd, (sockaddr_in *) &clientAddr, &clientAddrSize);
    if (clientSfd == -1) {
        fprintf(stderr, "Unable to accept connection\n");
    }

    
}
