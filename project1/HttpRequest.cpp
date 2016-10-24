#include <vector>
#include <iostream>
using namespace std;

class HttpRequest {
    public:
    void set_url(char const* url);
    void set_method(char const* method);
    vector<uint8_t> encode();
    private:
    //char* remove_null_bytes(char* cStr);
    char const* url;
    char const* method;
};
 
void HttpRequest::set_url(char const* url) {
     this->url = url;
}

void HttpRequest::set_method(char const* method) {
    this->method = method;
}

vector<uint8_t> HttpRequest::encode() {
    // encode method and URL to HTTP Request format
     // using HTTP v1.0, getting rid of '\0' terminators
     vector<uint8_t> wire;
     for (char const* p = method;  *p != '\0'; p++) {
         wire.push_back(*p);
     }
     wire.push_back(' ');
     for (char const *p = url;  *p != '\0'; p++) {
         wire.push_back(*p);
     }
     wire.push_back(' ');
     char const *httpVersion = "HTTP/1.0";
     for (char const *p = httpVersion;  *p != '\0'; p++) {
         wire.push_back(*p);
     }
 
     wire.push_back('\r'); wire.push_back('\n');
     wire.push_back('\r'); wire.push_back('\n');
 
     return wire;
 }
 
 int main(int argc, char* argv[]) {
     HttpRequest request;
     char const *method = "GET";
     request.set_method(method);
     request.set_url(argv[1]);
     vector<uint8_t> wire = request.encode();
     //wire.push_back('\0');
     string wireStr(wire.begin(), wire.end());
     cout << wireStr;
 }
