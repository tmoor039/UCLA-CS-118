.PHONY: all clean simple-tcp-server simple-tcp-client
CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
CLASSES=packet.cpp

all: simple-tcp-server simple-tcp-client 

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM simple-tcp-server simple-tcp-client

simple-tcp-server:
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) tcp_server.cpp server.cpp

simple-tcp-client:
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) tcp_client.cpp client.cpp

dist:
	tar -zcvf 704465898-104436427-704303603.tar.gz Makefile globals.h report.pdf tcp.h Vagrantfile client.cpp packet.cpp tcp_client.cpp packet.h server.cpp tcp_server.cpp
