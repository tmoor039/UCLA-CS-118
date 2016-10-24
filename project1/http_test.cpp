#include "http.h"
using namespace std;

// Convenience method to print out bytes
void print_byte_data(vector<uint8_t> data){
	int counter = 0;
	for(auto d : data) {
		cout << (unsigned int)d << " ";
		if(!(counter%20))
			cout << "\n";
		counter++;
	}
	cout << "\n";
}

int main(int argc, char** argv){
	// Some very basic argument parsing to test the request and response
	if(argc < 2) {
		cout << "bleep\n";
		cout << "Usage: [request | response] {url, host}\n";
		return -1;
	}
	if(string(argv[1]) == "request"){
		if(argc < 3){ cout << "Please enter a supporting URL for the request!\n"; return -1; }
		if(argc == 4 && string(argv[3]) == "localhost"){
			HttpRequest request((string(argv[2])), (string(argv[3])));
			print_byte_data(request.encode());
			cout << request;
		} else {
			HttpRequest request((string(argv[2])));
			print_byte_data(request.encode());
			cout << request;
		}
	} else if(string(argv[1]) == "response"){
		HttpResponse response;
		print_byte_data(response.encode());
		cout << response;
	} else {
		cout << "bloop\n";
		cout << "Usage: [request | response] {url, host}\n";
		return -1;
	}
	return 0;
}
