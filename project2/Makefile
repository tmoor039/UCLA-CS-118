.PHONY: all clean server client
CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
CLASSES=packet.cpp Udp.cpp

all: server client server2 client2
new: server2 client2

clean:
	if [ -a server ] ; \
	then \
		rm server ; \
	fi;
	if [ -a client ] ; \
	then \
		rm client ; \
	fi;
	rm -rf *.o *~ *.gch *.swp *.dSYM client2 server2


server:
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) $@.cpp	

client:
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) $@.cpp	

server2:
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) tcp_server.cpp $@.cpp
client2:
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) tcp_client.cpp $@.cpp
