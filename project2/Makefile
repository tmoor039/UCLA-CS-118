.PHONY: all clean server client
CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
CLASSES=packet.cpp

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
	$(CXX) -o $@ tcp_server.cpp packet.cpp $(CXXFLAGS) $@.cpp	

client:
	$(CXX) -o $@ tcp_client.cpp tcp_server.cpp packet.cpp $(CXXFLAGS) $@.cpp	

server2:
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) tcp_server.cpp $@.cpp
client2:
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) tcp_client.cpp $@.cpp
