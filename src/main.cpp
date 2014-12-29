#include <iostream>
#include <stdlib.h>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

#define IP_ADDRESS "10.0.0.193"
#define PORT "4025"
#define PASSWORD "EatHealthy1"

int main(int argc, char* argv[]) {
	cout << "Initializing..." << endl;

	/*boost::asio::io_service io_service;
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(IP_ADDRESS, PORT);
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::resolver::iterator end;
	tcp::socket socket(io_service);
	boost::system::error_code error = boost::asio::error::host_not_found;

	cout << "Finished Initializing." << endl;

	cout << "Connecting..." << endl;
	while(error && endpoint_iterator != end) {
		socket.close();
		socket.connect(*endpoint_iterator++, error);
	}
	if(error) {
		throw boost::system::system_error(error);
	}

	cout << "Connected." << endl << "Reading data from buffer..." << endl;

	for (;;)
	{
		boost::array<char, 128> buf;
		boost::system::error_code error;

		size_t len = socket.read_some(boost::asio::buffer(buf), error);
		if (error == boost::asio::error::eof)
			break; // Connection closed cleanly by peer.
		else if (error)
			throw boost::system::system_error(error); // Some other error.

		std::cout.write(buf.data(), len);
	}

	cout << "Finished." << endl << "Press any key to close... ";
	
	char key;
	cin >> key;*/

	return 0;
}
