#include <iostream>
#include <stdlib.h>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

int main(int argc, char* argv[]) {
	cout << "Client" << endl;

	boost::asio::io_service io_service;
	tcp::resolver resolver(io_service);
	tcp::resolver::query query("127.0.0.1", "1337");
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::resolver::iterator end;
	tcp::socket socket(io_service);
	boost::system::error_code error = boost::asio::error::host_not_found;

	while(error && endpoint_iterator != end) {
		socket.close();
		socket.connect(*endpoint_iterator++, error);
	}
	if(error) {
		//throw boost::system::system_error(error);
		cout << "Error 1" << endl << "Error: " << error.value();
		return -1;
	}

	boost::array<char, 128> buf;
	size_t len = socket.read_some(boost::asio::buffer(buf), error);
	if (error == boost::asio::error::eof) {
		cout << "Error [read_command]" << endl << "Error: Connection closed by peer" << endl;
		return -1;
	}
	else if (error) {
		//throw boost::system::system_error(error); // Some other error.
		cout << "Error [read_command]" << endl << "Error: " << error.value() << endl;;
		return -1;
	}

	cout << "Messaged RXd:" << endl;
	std::cout.write(buf.data(), len);
	cout << endl;
	
	system("pause");

	return 0;
}
