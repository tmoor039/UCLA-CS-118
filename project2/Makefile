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


server:
	g++ -std=c++11 -Wall -O2 server.cpp Udp.cpp -o server

client:
	g++ -std=c++11 -Wall -O2 client.cpp Udp.cpp -o client
