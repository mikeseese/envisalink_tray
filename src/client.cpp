#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

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

#ifdef __gnu_linux__
	pause();
#endif
#ifndef __gnu_linux__
	system("pause");
#endif

	return 0;
}
