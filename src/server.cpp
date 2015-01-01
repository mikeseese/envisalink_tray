#include <iostream>
#include <stdlib.h>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <vector>

using namespace std;
using boost::asio::ip::tcp;

#define PASSWORD "user"

struct elv3_command {
	tcp::socket* client;
	string message;
};

vector<tcp::socket*> all_clients;
vector<tcp::socket*> logged_in_clients;
vector<elv3_command> request_queue;

void read_command(tcp::socket &socket, boost::array<char, 128> &buf, size_t &len, unsigned int timeout_delay = -1) {
	boost::system::error_code error;

	len = socket.read_some(boost::asio::buffer(buf), error);
	if (error == boost::asio::error::eof) {
		cout << "Error [read_command]" << endl << "Error: Connection closed by peer" << endl;
		buf.empty();
		return;
	}
	else if (error) {
		//throw boost::system::system_error(error); // Some other error.
		cout << "Error [read_command]" << endl << "Error: " << error.value() << endl;;
		buf.empty();
		len = 0;
		return;
	}

	if(buf[len-2] != '\r' || buf[len-1] != '\n') {
		cout << "Error [read_command]" << endl << "Error: No CR and LF" << endl;
		buf.empty();
		len = 0;
		return;
	}
}

string uitos_2_hex(unsigned int i) {
	char buf[2];
	itoa(i, buf, 16);
	string out(buf);
	return out;
}

unsigned int calculate_checksum(string in) {
	unsigned int sum = 0;
	char buf[2];

	for(int i = 0; i < in.length(); i++) {
		sum += in.at(i);
	}

	return sum & 0xFF;
}

bool check_checksum(string in) {
	string check_checksum = in.substr(in.length()-4,2);
	unsigned int checksum = strtoul(check_checksum.c_str(), NULL, 16);

	unsigned int calculated_checksum = calculate_checksum(in.substr(0,in.length()-4));

	if(calculated_checksum == checksum) {
		return true;
	}
	else {
		return false;
	}
}

bool login_handshake(tcp::socket &socket) {
	bool login_success = false;
	boost::system::error_code ignored_error;
	string message;
	string code = "505";
	string data = "3";
	message.append(code);
	message.append(data);
	message.append(uitos_2_hex(calculate_checksum(message)));
	cout << "Message prepared to write out: " << message.c_str() << endl;
	message.append("\r\n");
	boost::asio::write(socket, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);

	boost::array<char, 128> buf;
	size_t len;

	read_command(socket, buf, len, 10);

	string buffer(buf.data());
	cout << "buffer len: " << buffer.length() << endl;
	if(check_checksum(buffer) && buffer.length() > 7) {
		if(buffer.substr(0,3).compare("005") == 0) {
			if(buffer.substr(3,buffer.length()-7).compare(PASSWORD) == 0) {
				// Successful login
				login_success = true;
			}
		}
	}

	if(login_success) {
		data = "1";
	}
	else {
		data = "0";
	}

	message = "";
	message.append(code);
	message.append(data);
	message.append(uitos_2_hex(calculate_checksum(message)));
	cout << "Message prepared to write out: " << message.c_str() << endl;
	message.append("\r\n");
	boost::asio::write(socket, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);

	return login_success;
}

void acceptor_loop() {
	boost::asio::io_service io_service;
	tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 1337));
	boost::system::error_code ignored_error;
	string message = "whats up bitches";

	for(;;) {
		tcp::socket *socket = new tcp::socket(io_service);
		acceptor.accept(*socket);
		//boost::asio::write(socket, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);
		all_clients.push_back(socket);
	}
}

void communicator_loop() {
	for(;;) {
		// Look for new clients that need to login
		if(all_clients.size() != logged_in_clients.size()) {
			for(int i = 0; i < all_clients.size(); i++) {
				if(!(find(logged_in_clients.begin(), logged_in_clients.end(), all_clients.at(i)) != logged_in_clients.end())) {
					if(login_handshake(*all_clients.at(i))) {
						logged_in_clients.push_back(all_clients.at(i));
					}
					else {
						all_clients.erase(all_clients.begin()+i);
						break; // This breaks the for loop for checking the new clients. This will continue the communicator_loop, 
								//  but other new clients will have to wait until next time
					}
				}
			}
		}

		for(int i = 0; i < logged_in_clients.size(); i++) {
			// See if anyone wants to queue up a command
			boost::array<char, 128> buf;
			size_t len;
			elv3_command command;

			read_command(*logged_in_clients.at(i), buf, len, 1); // 1 second timeout
			buf[len] = '\0'; // null terminated char array
			string buffer(buf.data());

			command.message = buffer;
			command.client = logged_in_clients.at(i);

			request_queue.push_back(command);
		}
		cout << "Number of logged_in_clients: " << logged_in_clients.size() << endl;
	}
}

int main(int argc, char* argv[]) {
	cout << "Server" << endl;

	boost::thread acceptor_thread(acceptor_loop);
	acceptor_thread.detach();

	boost::thread communicator_thread(communicator_loop);
	communicator_thread.detach();
	
	system("pause");

	return 0;
}
