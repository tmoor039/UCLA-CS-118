#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

class URL{
  public:
	URL()
		: host("localhost"), object(""), port(80)
	{}
	URL(std::string v_object, std::string v_host="localhost", unsigned int v_port=80)
		: host(v_host), object(v_object), port(v_port)
	{}
  std::string resolveDomain();
  std::string url;
	std::string host;
  std::string ip;
	std::string object;
  unsigned short port;
};

// Abstract class for HTTP Messages
// Every HTTP Message should have a version and connection type
class HttpMessage {
protected:
	std::string m_version;
	std::string m_connection;
public:
	HttpMessage();
	HttpMessage(std::string version);
	virtual ~HttpMessage() {};

	// Accessors
	std::string get_version() const { return m_version; }
	std::string get_connection() const { return m_connection; }

	// Mutators
	void set_version(std::string version) { m_version = version; }
	void set_connection(std::string connection) { m_connection = connection; }

	// Virtual methods
	virtual std::vector<uint8_t> encode() = 0;
};

// Every Request should have at the least a url, method and host name
class HttpRequest: HttpMessage {
	std::string m_method;
	URL m_url;
	unsigned int m_keep_alive;
public:
	// Constructors
	HttpRequest(URL url, std::string method="GET");
	HttpRequest(URL url, std::string version, std::string method="GET");
	virtual ~HttpRequest() {};

	// Accessors
	URL get_url() const { return m_url; }
	std::string get_method() const { return m_method; }

	// Mutators
	void set_url(URL url) { m_url = url; }
	void set_method(std::string method) { m_method = method; }

	// Overiding methods
	std::vector<uint8_t> encode() override;

	// Overloading ostream
	friend std::ostream& operator<<(std::ostream& os, const HttpRequest& http_req);
};

// Every Response should have a status code and the data associated with it
class HttpResponse: HttpMessage {
	unsigned int m_status;
	size_t m_data_size;
	std::vector<uint8_t> m_data;
	const std::unordered_map<unsigned int, std::string> m_codes = {
		{200, "OK"},
		{400, "Bad Request"},
		{404, "Not Found"},
		{505, "HTTP Version Not Supported"}
	};
	// Used for data printing
	void print_data() const{
		int counter = 0;
		for(uint8_t d : m_data){
			std::cout << (unsigned int)d << " ";
			if(!(counter%20))
				std::cout << std::endl;
			counter++;
		}
	}
public:
	HttpResponse();
	HttpResponse(int status, std::vector<uint8_t> data);
	HttpResponse(int status, std::string version, std::vector<uint8_t> data);
	virtual ~HttpResponse(){};

	// Accessors
	unsigned int get_status() const { return m_status; }
	std::vector<uint8_t> get_data() const { return m_data; }
	// Get the mapped number to status code message
	std::string get_code(unsigned int status) const { 
		try{
			return m_codes.at(status);
		} catch (...) {
			return "Invalid code";	
		}
	}

	// Mutators
	void set_status(unsigned int status) { m_status = status; }
	void set_data(std::vector<uint8_t> data) { m_data = data; }

	// Overiding methods
	std::vector<uint8_t> encode() override;

	// Overloading ostream
	friend std::ostream& operator<<(std::ostream& os, const HttpResponse& http_resp);
};
