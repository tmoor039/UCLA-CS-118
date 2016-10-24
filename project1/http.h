#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>

// Abstract class for HTTP Messages
class HttpMessage {
protected:
	std::string m_version;
	std::string m_connection;
public:
	HttpMessage();
	HttpMessage(std::string version);

	// Accessors
	std::string get_version() const { return m_version; }
	std::string get_connection() const { return m_connection; }

	// Mutators
	void set_version(std::string version) { m_version = version; }
	void set_connection(std::string connection) { m_connection = connection; }

	// Virtual methods
	virtual std::vector<uint8_t> encode() = 0;
};

class HttpRequest: HttpMessage {
	std::string m_URL;
	std::string m_method;
	std::string m_host;
public:
	// Constructors
	HttpRequest();
	HttpRequest(std::string url, std::string host="localhost", std::string method="GET");

	// Accessors
	std::string get_url() const { return m_URL; }
	std::string get_method() const { return m_method; }
	std::string get_host() const { return m_host; }

	// Mutators
	void set_url(std::string url) { m_URL = url; }
	void set_method(std::string method) { m_method = method; }
	void set_host(std::string host) { m_host = host; }

	// Overiding methods
	std::vector<uint8_t> encode() override;

	// Overloading ostream
	friend std::ostream& operator<<(std::ostream& os, const HttpRequest& http_req);
};

class HttpResponse: HttpMessage {
	unsigned int m_status;
	std::vector<uint8_t> m_data;
	const std::unordered_map<unsigned int, std::string> m_codes = {
		{200, "OK"},
		{400, "BAD REQUEST"},
		{404, "NOT FOUND"},
		{505, "HTTP VERSION NOT SUPPORTED"}
	};
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

	// Accessors
	unsigned int get_status() const { return m_status; }
	std::vector<uint8_t> get_data() const { return m_data; }
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
