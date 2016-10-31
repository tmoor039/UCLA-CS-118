.PHONY: all web-server web-server-1.1 web-client web-client-1.1 clean
CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
USERID=104436427-704465898-704303603
CLASSES=http.cpp

all: web-server web-server-1.1 web-client web-client-1.1

web-server: 
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) $@.cpp

web-server-1.1:
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) $@.cpp

web-client:
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) $@.cpp

web-client-1.1:
	$(CXX) -o $@ $(CLASSES) $(CXXFLAGS) $@.cpp

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM web-server web-client web-server-1.1 web-client-1.1 *.tar.gz

tarball: clean
	tar -cvf $(USERID).tar.gz *
