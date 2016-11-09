#include "UdpServer.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

int UdpServer::get_cfd() {
    return cfd_;
}

UdpServer::UdpServer(char* port)                                                     
    : port_(port), sfd_(-1), cfd_(-1)                                              
{                                                                                  
    struct addrinfo hints;                                                         
    struct addrinfo *result, *rp;                                                  
    memset(&hints, 0, sizeof(hints));                                              
    hints.ai_family = AF_UNSPEC;                                                   
    hints.ai_socktype = SOCK_DGRAM; // for UDP                                     
    hints.ai_protocol = IPPROTO_UDP;                                               
    hints.ai_flags = AI_PASSIVE;                                                   
    // Find an address suitable for accepting connections                      

    int ret = getaddrinfo(NULL, port, &hints, &result);                            
    if (ret != 0) {                                                                
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));                     
    }                                                                              

    int sfd;                                                                       
    for (rp = result; rp != NULL; rp = rp->ai_next) {                              
        sfd = socket(rp->ai_family, rp->ai_socktype,                               
                rp->ai_protocol);                                                          
        if (sfd == -1)                                                             
            continue;                                                              
        ret = bind(sfd_, rp->ai_addr, rp->ai_addrlen);                             
        if (ret != -1) {                                                           
            break;  // success
            sfd_ = sfd;                                                            
        }                                                                          
        close(sfd);                                                             
    }                                                                           
    if (rp == NULL) {                                                           
        fprintf(stderr, "Could not bind socket address\n");                     
    }                                                                           
    freeaddrinfo(result);                                                       

    ret = listen(sfd, SOMAXCONN);                                               
    if (ret == -1) {                                                            
        fprintf(stderr, "Failed to listen on socket\n");                        
    }                                                                           
}                                                                               

void UdpServer::accept_connection() {
    int cfd = accept(sfd_, NULL, NULL);
    if (cfd == -1) {
        fprintf(stderr, "accept error\n");                                      
    }
    else {
        cfd_ = cfd;
    } 
}                           
