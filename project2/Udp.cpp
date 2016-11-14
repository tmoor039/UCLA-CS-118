#include "Udp.h"
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include "globals.h"
#include <stdlib.h>

  Udp::Udp(int port) 
: port_(port), sfd_(-1)
{}

void Udp::set_send_buf(std::string data) {
  memset(sendPacket_, '\0', sizeof(sendPacket_));
  std::strcpy(sendPacket_, data.c_str());
}

  UdpServer::UdpServer(int port)                                                     
: Udp(port)
{                                                                                  
  struct addrinfo hints;                                                         
  struct addrinfo *result, *rp;                                                  
  memset(&hints, 0, sizeof(hints));                                              
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM; // for UDP                                     
  hints.ai_protocol = IPPROTO_UDP;                                               
  // Find an address suitable for accepting connections                      

  int ret = getaddrinfo("localhost", std::to_string(port).c_str(), &hints, &result);

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

void Udp::set_timeout(float sec, float usec) {
  struct timeval tv;
  tv.tv_sec = sec;
  tv.tv_usec = usec;
  if (setsockopt(sfd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    perror("Error");
  }
}

ssize_t Udp::receive_packet(sockaddr* srcAddr, socklen_t* addrLen) {
  ssize_t nread;
  nread = recvfrom(sfd_, recvPacket_, PACKET_SIZE, 0,
      srcAddr, addrLen);
  if (nread == -1) {
    //fprintf(stderr, "recvfrom error\n");
  }
  //std::cout << std::string(recvPacket_) << std::endl;
  return nread;
}

ssize_t Udp::send_packet(sockaddr* destAddr, socklen_t addrLen) {
  ssize_t nsent;
  nsent = sendto(sfd_, sendPacket_, PACKET_SIZE, 0, destAddr, addrLen);
  if (nsent == -1) {
    //fprintf(stderr, "sendto error\n");
  }
  return nsent;
}

ssize_t UdpClient::receive_packet() {
  ssize_t nread;
  nread = recvfrom(sfd_, recvPacket_, PACKET_SIZE, 0, destAddr_, &destAddrLen_);
  if (nread == -1) {
    //fprintf(stderr, "recvfrom error\n");
  }
  //std::cout << std::string(recvPacket_) << std::endl;
  return nread;
}

ssize_t UdpClient::send_packet() {
  /*ssize_t nsent;
    nsent = sendto(sfd_, sendPacket_, PACKET_SIZE, 0, destAddr_, destAddrLen_);
    if (nsent == -1) {
    fprintf(stderr, "sendto error\n");
    }*/

  int len = strlen(sendPacket_);
  if (write(sfd_, sendPacket_, len) != len) {
    fprintf(stderr, "Incomplete write\n");
  }
  return 0;
  //return nsent;
}

  UdpClient::UdpClient(std::string serverHost, int port)
: Udp(port), serverHost_(serverHost) 
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM; // for UDP
  hints.ai_protocol = IPPROTO_UDP;

  int ret = getaddrinfo(serverHost_.c_str(), 
      std::to_string(port).c_str(), &hints, &result);
  if (ret != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
  }
  int sfd;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype,
        rp->ai_protocol);
    if (sfd == -1)
      continue;
    ret = connect(sfd, rp->ai_addr, rp->ai_addrlen);
    if (ret != -1) {
      sfd_ = sfd;
      destAddr_ = rp->ai_addr;
      destAddrLen_ = rp->ai_addrlen;
      break;  // success
    }
    close(sfd);
  }
  if (rp == NULL) {
    fprintf(stderr, "Could not connect with socket address\n");
  }
  freeaddrinfo(result);
}
