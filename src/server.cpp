#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <vector>

using namespace std;
using boost::asio::ip::tcp;

string s;

#define IP_ADDRESS "127.0.0.1"
#define PORT "4025"
#define PASSWORD "user"

//#define BUILD_TPI_EMULATOR

struct elv3_tpi_raw_command {
	boost::array<char, 128> buffer;
	size_t length;
};

struct elv3_tpi_command {
	elv3_tpi_raw_command command;
	tcp::socket* client;
};

#ifdef __gnu_linux__

void Sleep(unsigned int milliseconds) {
	usleep(milliseconds*1000);
}

/* A utility function to reverse a string  */
void reverse(char str[], int length)
{
    int start = 0;
    int end = length -1;
    while (start < end)
    {
        swap(*(str+start), *(str+end));
        start++;
        end--;
    }
}

// Implementation of itoa()
char* itoa(int num, char* str, int base)
{
    int i = 0;
    bool isNegative = false;
 
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
    // In standard itoa(), negative numbers are handled only with 
    // base 10. Otherwise numbers are considered unsigned.
    if (num < 0 && base == 10)
    {
        isNegative = true;
        num = -num;
    }
 
    // Process individual digits
    while (num != 0)
    {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }
 
    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';
 
    str[i] = '\0'; // Append string terminator
 
    // Reverse the string
    reverse(str, i);
 
    return str;
}

#endif

tcp::socket* elv3_tpi_socket;
boost::asio::io_service elv3_tpi_io_service;
vector<tcp::socket*> all_clients;
vector<tcp::socket*> logged_in_clients;
vector<elv3_tpi_command> command_queue;

void read_command(tcp::socket &socket, elv3_tpi_raw_command &cmd, unsigned int timeout_delay = -1) {
	boost::system::error_code error;

	cmd.length = socket.read_some(boost::asio::buffer(cmd.buffer), error);
	if (error == boost::asio::error::eof) {
		cout << "Error [read_command]" << endl << "Error: Connection closed by peer" << endl;
		cmd.buffer.empty();
		return;
	}
	else if (error) {
		//throw boost::system::system_error(error); // Some other error.
		cout << "Error [read_command]" << endl << "Error: " << error.value() << endl;;
		cmd.buffer.empty();
		cmd.length = 0;
		return;
	}

	if(cmd.buffer[cmd.length-2] != '\r' || cmd.buffer[cmd.length-1] != '\n') {
		cout << "Error [read_command]" << endl << "Error: No CR and LF" << endl;
		cout << "buffer: '" << cmd.buffer.data() << "'" << endl;
		cout << "length: " << cmd.length << endl;
		cmd.buffer.empty();
		cmd.length = 0;
		return;
	}
}

void execute_command(elv3_tpi_command command) {
	elv3_tpi_raw_command response;

	boost::system::error_code ignored_error;
	cout << "Relaying command: " << command.command.buffer.data() << endl;
	cout << "Command length: " << command.command.length << endl;
	boost::asio::write(*elv3_tpi_socket, boost::asio::buffer(command.command.buffer, command.command.length), boost::asio::transfer_all(), ignored_error);
	read_command(*elv3_tpi_socket, response);
	boost::asio::write(*command.client, boost::asio::buffer(response.buffer, response.length), boost::asio::transfer_all(), ignored_error);
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
	message.append("\r\n");
	boost::asio::write(socket, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);

	boost::array<char, 128> buf;
	size_t len;
	elv3_tpi_raw_command temp;

	read_command(socket, temp, 10);

	string buffer(temp.buffer.data());
	buffer = buffer.substr(0, temp.length);
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
	message.append("\r\n");
	boost::asio::write(socket, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);

	return login_success;
}

void acceptor_loop() {
	boost::asio::io_service io_service;
#ifdef BUILD_TPI_EMULATOR
	tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 4025));
#endif
#ifndef BUILD_TPI_EMULATOR
	tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 1337));
#endif

	for(;;) {
		tcp::socket *socket = new tcp::socket(io_service);
		acceptor.accept(*socket);
		all_clients.push_back(socket);
		Sleep(100);
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
#ifdef BUILD_TPI_EMULATOR
				elv3_tpi_command cmd2;
				cmd2.client = logged_in_clients.at(i);
				read_command(*logged_in_clients.at(i), cmd2.command, 1);
				cout << "Received command2: " << cmd2.command.buffer.data() << endl;
				string temp_string(cmd2.command.buffer.data());
				temp_string = temp_string.substr(0, cmd2.command.length);
				string recv_code = temp_string.substr(0, 3);
				
				// send ack
				boost::system::error_code ignored_error;
				string code = "500";
				string message;
				message.append(code);
				message.append(recv_code);
				message.append(uitos_2_hex(calculate_checksum(message)));
				cout << "Message prepared to write out: " << message.c_str() << endl;
				message.append("\r\n");
				boost::asio::write(*cmd2.client, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);
#endif
			if((*logged_in_clients.at(i)).available() > 0) {
				boost::array<char, 128> buf;
				size_t len;
				elv3_tpi_command cmd;
				cmd.client = logged_in_clients.at(i);

				read_command(*logged_in_clients.at(i), cmd.command, 1);

				//add buf command to queue
				command_queue.push_back(cmd);
				cout << "Received command: " << cmd.command.buffer.data() << endl;
			}
		}
		Sleep(100);
	}
}

void command_loop() {
	for(;;) {
		for(int i = 0; i < command_queue.size(); i++) {
			execute_command(command_queue.at(i));
		}
		command_queue.clear();
		Sleep(100);
	}
}

void connect_elv3() {
	cout << "Connecting to ELV3 TPI..." << endl;
	char key;

	boost::system::error_code error = boost::asio::error::host_not_found;
	
	tcp::resolver resolver(elv3_tpi_io_service);
	tcp::resolver::query query(IP_ADDRESS, PORT);
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::resolver::iterator end;
	elv3_tpi_socket = new tcp::socket(elv3_tpi_io_service);

	while(error && endpoint_iterator != end) {
		(*elv3_tpi_socket).close();
		(*elv3_tpi_socket).connect(*endpoint_iterator++, error);
	}
	if(error) {
		//throw boost::system::system_error(error);
		cout << "Error 1" << endl << "Error: " << error.value();
		cin >> key;
		return;
	}

	cout << "Connected." << endl << "Reading data from buffer..." << endl;

	boost::array<char, 128> buf;
	size_t len;
	elv3_tpi_raw_command temp;

	read_command((*elv3_tpi_socket), temp);
	cout << "RX:" << endl;
	std::cout.write(temp.buffer.data(), temp.length);

	boost::system::error_code ignored_error;
	string message;
	string code = "005";
	string data = PASSWORD;
	message.append(code);
	message.append(data);
	message.append(uitos_2_hex(calculate_checksum(message)));
	cout << "Message prepared to write out: " << message.c_str() << endl;
	message.append("\r\n");
	boost::asio::write((*elv3_tpi_socket), boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);


	read_command((*elv3_tpi_socket), temp);
	cout << "RX:" << endl;
	std::cout.write(temp.buffer.data(), temp.length);
}

int main(int argc, char* argv[]) {
	cout << "Server" << endl;

#ifndef BUILD_TPI_EMULATOR
	connect_elv3();

#ifdef __gnu_linux__
	cout << "Press enter to continue...";
	getline(cin,s);
#endif
#ifndef __gnu_linux__
	system("pause");
#endif
#endif

	boost::thread acceptor_thread(acceptor_loop);
	acceptor_thread.detach();

	boost::thread communicator_thread(communicator_loop);
	communicator_thread.detach();

	boost::thread command_thread(command_loop);
	command_thread.detach();

#ifdef __gnu_linux__
	cout << "Press enter to continue...";
	getline(cin,s);
#endif
#ifndef __gnu_linux__
	system("pause");
#endif

	return 0;
}
