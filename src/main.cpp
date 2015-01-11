#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

#define IP_ADDRESS "127.0.0.1"
#define PORT "1337"
#define PASSWORD "user"

string s;

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

void read_command(tcp::socket &socket, boost::array<char, 128> &buf, size_t &len) {
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
		cout << buf[len-2] << buf[len-1] << endl;
		buf.empty();
		len = 0;
		return;
	}
}

string calculate_checksum(string in) {
	unsigned int sum = 0;
	char buf[2];

	for(int i = 0; i < in.length(); i++) {
		sum += in.at(i);
	}

	itoa(sum & 0xFF, buf, 16);
	string out(buf);
	return out;
}

int main(int argc, char* argv[]) {
	cout << "Initializing..." << endl;

	boost::asio::io_service io_service;
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(IP_ADDRESS, PORT);
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::resolver::iterator end;
	tcp::socket socket(io_service);
	boost::system::error_code error = boost::asio::error::host_not_found;

	cout << "Finished Initializing." << endl;

	cout << "Connecting..." << endl;
	char key;
	while(error && endpoint_iterator != end) {
		socket.close();
		socket.connect(*endpoint_iterator++, error);
	}
	if(error) {
		throw boost::system::system_error(error);
		cout << "Error 1" << endl << "Error: " << error.value();
		cin >> key;
		return -1;
	}

	cout << "Connected." << endl << "Reading data from buffer..." << endl;

	boost::array<char, 128> buf;
	size_t len;

	read_command(socket, buf, len);
	cout << "RX:" << endl;
	std::cout.write(buf.data(), len);

	boost::system::error_code ignored_error;
	string message;
	string code = "005";
	string data = PASSWORD;
	message.append(code);
	message.append(data);
	message.append(calculate_checksum(message));
	cout << "Message prepared to write out: " << message.c_str() << endl;
	message.append("\r\n");
	boost::asio::write(socket, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);


	read_command(socket, buf, len);
	cout << "RX:" << endl;
	std::cout.write(buf.data(), len);

#ifdef __gnu_linux__
	cout << "Press enter to continue...";
	getline(cin,s);
#endif
#ifndef __gnu_linux__
	system("pause");
#endif

	code = "000";
	data = "";
	message.clear();
	message.append(code);
	message.append(data);
	message.append(calculate_checksum(message));
	cout << "Message prepared to write out: " << message.c_str() << endl;
	message.append("\r\n");
	boost::asio::write(socket, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);

#ifdef __gnu_linux__
	cout << "Press enter to continue...";
	getline(cin,s);
#endif
#ifndef __gnu_linux__
	system("pause");
#endif

	return 0;
}
