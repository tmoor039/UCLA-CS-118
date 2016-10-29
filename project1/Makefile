default:
	g++ -std=c++11 http_test.cpp http.cpp -o http
	g++ -std=c++11 web-server.cpp http.cpp -o web-server
	g++ -std=c++11 web-client.cpp http.cpp -o web-client

server:
	g++ -std=c++11 web-server.cpp http.cpp -o web-server

client:
	g++ -std=c++11 web-client.cpp http.cpp -o web-client

clean:
	rm -f http web-server web-client
