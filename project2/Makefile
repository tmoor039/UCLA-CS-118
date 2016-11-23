.PHONY: all clean server client
CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
CLASSES=packet.cpp

all: server client 

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM server client

server:
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) tcp_server.cpp $@.cpp

client:
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) tcp_client.cpp $@.cpp
