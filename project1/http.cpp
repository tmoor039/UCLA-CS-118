#include "http.h"
using namespace std;

// The Message implementation - This is the base class

HttpMessage::HttpMessage()
	: m_version("HTTP/1.0"), m_connection("non-persistent")
{}

HttpMessage::HttpMessage(string version)
	: m_version(version)
{
	if(version == "HTTP/1.1" || version == "HTTP/2.0"){
		m_connection = "persistent";
	} else if(version == "HTTP/1.0") {
		m_connection = "non-persistent";
	} else {
		m_connection = "invalid";
	}
}

// The Request implementation - Used to make generic HTTP Requests (only supports GET for now)

HttpRequest::HttpRequest()
	: m_URL(""), m_method("GET"), m_host("localhost")
{}

HttpRequest::HttpRequest(string url, string host, string method)
	: m_URL(url), m_method(method), m_host(host)
{}

// Encode the data as bytes
vector<uint8_t> HttpRequest::encode(){
	string cr_nl = "\r\n";
	string final_message = m_method + " " + m_URL + " " + m_version + cr_nl +
						   m_host + cr_nl + m_connection + cr_nl + cr_nl;
	vector<uint8_t> wire(final_message.begin(), final_message.end());

	return wire;
}

// Convenience overloading for easy output
ostream& operator<<(ostream& os, const HttpRequest& http_req) {
	os << "Method: " << http_req.m_method << endl
	   << "URL: " << http_req.m_URL << endl
	   << "Version: " << http_req.m_version << endl
	   << "Host: " << http_req.m_host << endl
	   << "Connection-type: " << http_req.m_connection << endl;
	return os;
}

// The Response implementation - Used to make generic HTTP Responses

HttpResponse::HttpResponse()
	: m_status(200)
{}

HttpResponse::HttpResponse(int status, vector<uint8_t> data)
	: m_status(status)
{
	m_data = data;
}

// Encode the data as bytes
vector<uint8_t> HttpResponse::encode(){
	string cr_nl = "\r\n";
	string final_message = m_version + to_string(m_status) + cr_nl + m_connection +
						   cr_nl + cr_nl;
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
