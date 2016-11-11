#include "UdpServer.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "globals.h"
#include <stdlib.h>

Udp::Udp(int port) 
    : port_(port), sfd_(-1)
{}

UdpServer::UdpServer(int port)                                                     
    : Udp(port)
{                                                                                  
    struct addrinfo hints;                                                         
    struct addrinfo *result, *rp;                                                  
    memset(&hints, 0, sizeof(hints));                                              
    hints.ai_family = AF_UNSPEC;                                                   
    hints.ai_socktype = SOCK_DGRAM; // for UDP                                     
    hints.ai_protocol = IPPROTO_UDP;                                               
    hints.ai_flags = AI_PASSIVE;                                                   
    // Find an address suitable for accepting connections                      

    int ret = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &result);                            
    if (ret != 0) {                                                                
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));                     
    }                                                                              

    int sfd;                                                                       
    for (rp = result; rp != NULL; rp = rp->ai_next) {                              
        sfd = socket(rp->ai_family, rp->ai_socktype,                               
                rp->ai_protocol);                                                          
        if (sfd == -1)                                                             
            continue;                                                              
        ret = bind(sfd, rp->ai_addr, rp->ai_addrlen);                             
        if (ret != -1) {                                                           
            sfd_ = sfd;                                                            
            break;  // success
        }                                                                          
        close(sfd);                                                             
    }                                                                           
    if (rp == NULL) {                                                           
        fprintf(stderr, "Could not bind socket address\n");                     
    }                                                                           
    freeaddrinfo(result);                                                       
}                                                                               

ssize_t Udp::receive_packet() {
    ssize_t nread;
    sockaddr_storage peer_addr;
    socklen_t peer_addr_len = sizeof(sockaddr_storage);
    char buf[PACKET_SIZE];    
    nread = recvfrom(sfd_, buf, PACKET_SIZE, 0,
        (sockaddr *) &peer_addr, &peer_addr_len);
    if (nread == -1)
        fprintf(stderr, "recvfrom error\n");
    std::cout << std::string(buf) << std::endl;
    return nread;
}
                           
