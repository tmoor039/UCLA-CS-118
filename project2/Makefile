.PHONY: all clean server client
CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
CLASSES=packet.cpp Udp.cpp

all: server client

clean:
	if [ -a server ] ; \
	then \
		rm server ; \
	fi;
	if [ -a client ] ; \
	then \
		rm client ; \
	fi;
	rm -rf *.o *~ *.gch *.swp *.dSYM 


server:
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) $@.cpp	

client:
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) $@.cpp	
