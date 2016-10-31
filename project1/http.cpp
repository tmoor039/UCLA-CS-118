#include "http.h"
using namespace std;

// The URL implementation

string URL::resolveDomain() {
  struct addrinfo hints;
  struct addrinfo* res;

  // prepare hints
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // IPv4
  hints.ai_socktype = SOCK_STREAM; // TCP

  // get address
  int status = 0;
  if ((status = getaddrinfo(host.c_str(), to_string(port).c_str(), &hints, &res)) != 0) {
    return "";
  }

  char ipstr[INET_ADDRSTRLEN] = {'\0'};
  for(struct addrinfo* p = res; p != 0; p = p->ai_next) {
    // convert address to IPv4 address
    struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;

    // convert the IP to a string and print it:
    inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
  }
      
  freeaddrinfo(res); // free the linked list

  return ipstr;
}

// The Message implementation - This is the base class

HttpMessage::HttpMessage()
	: m_version("HTTP/1.0"), m_connection("non-persistent")
{}

HttpMessage::HttpMessage(string version)
	: m_version(version)
{
	if(version == "HTTP/1.1" || version == "HTTP/2.0"){
		m_connection = "keep-alive";
	} else if(version == "HTTP/1.0") {
		m_connection = "non-persistent";
	} else {
		m_connection = "invalid";
	}
}

// The Request implementation - Used to make generic HTTP Requests (only supports GET for now)

HttpRequest::HttpRequest(URL url, string method)
	:  m_method(method)
{
	m_url = url;
}
HttpRequest::HttpRequest(URL url, string version, string method)
	: HttpMessage(version), m_method(method)
{
	m_url = url;
	// Keep Alive time for 1.1 - Default to 115s
  if(version == "HTTP/1.1"){
    m_keep_alive = 115;
  } else {
    m_keep_alive = 0;
  }
}

// Encode the data as bytes
vector<uint8_t> HttpRequest::encode(){
	string cr_nl = "\r\n";
	string keep_alive_info = "";
	if(m_version == "HTTP/1.1"){
		keep_alive_info = "Keep-Alive: " + to_string(m_keep_alive) + cr_nl;
	}
	string final_message = m_method + " " + m_url.object + " " + m_version + cr_nl + "Host: " + m_url.host + cr_nl +
						   keep_alive_info + cr_nl;
	vector<uint8_t> wire(final_message.begin(), final_message.end());

	return wire;
}

// Convenience overloading for easy output
ostream& operator<<(ostream& os, const HttpRequest& http_req) {
	os << "Method: " << http_req.m_method << endl
	   << "Version: " << http_req.m_version << endl
	   << "Host: " << http_req.m_url.host << endl
	   << "Object: " << http_req.m_url.object << endl
	   << "Connection-type: " << http_req.m_connection << endl;
	return os;
}

// The Response implementation - Used to make generic HTTP Responses

HttpResponse::HttpResponse()
	: m_status(200), m_data_size(0)
{}

HttpResponse::HttpResponse(int status, vector<uint8_t> data)
	: m_status(status)
{
	m_data = data;
	m_data_size = data.size();
}

HttpResponse::HttpResponse(int status, string version, vector<uint8_t> data)
	: HttpMessage(version), m_status(status)
{
	m_data = data;
	m_data_size = data.size();
}

// Encode the data as bytes
vector<uint8_t> HttpResponse::encode(){
	string cr_nl = "\r\n";
	string final_message =  m_version + " " + to_string(m_status) + " " + get_code(m_status) + cr_nl + m_connection +
		cr_nl + "Content-Length: " + to_string(m_data_size) + cr_nl + cr_nl;
	vector<uint8_t> wire(final_message.begin(), final_message.end());
	wire.insert(wire.end(), m_data.begin(), m_data.end());

	return wire;
}

// Convenience overloading for easy output
ostream& operator<<(ostream& os, const HttpResponse& http_resp){
	os  << "Version: " << http_resp.m_version << endl
		<< "Status: " << http_resp.get_code(http_resp.m_status) << endl
	    << "Connection-type: " << http_resp.m_connection << endl
	    << "Data: ";
	http_resp.print_data();
	return os;
}
