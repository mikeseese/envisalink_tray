#include <iostream>
#include <stdlib.h>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

#define IP_ADDRESS "10.0.0.1"

int main(int argc, char* argv[]) {
	boost::asio::io_service io_service;
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(IP_ADDRESS, "daytime?");

	return 0;
}
