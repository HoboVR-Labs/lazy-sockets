#include <iostream>
#include "lazy_sockets.h"

int main()
{
	// lets create a socket
	lsc::LSocket<AF_INET, SOCK_STREAM, 0> client;
	// connect it
	int res = client.Connect("127.0.0.1", 6969);
	if (res)
		return -errno;
	char buff[256]; // receive buffer
	// send buffers
	char msg1[] = "this is bullshit";
	char msg2[] = "i did not hit her";
	char msg3[] = "i did not";
	char msg4[] = "oh hi mark";
	// and send a bunch of stuff
	res = client.Send(msg1, sizeof(msg1));
	if (res)
		return -errno;
	res = client.Recv(buff, sizeof(buff));
	if (res)
		return -errno;
	std::cout << '"' << buff << "\"\n";
	res = client.Send(msg2, sizeof(msg2));
	if (res)
		return -errno;
	res = client.Recv(buff, sizeof(buff));
	if (res)
		return -errno;
	std::cout << '"' << buff << "\"\n";
	res = client.Send(msg3, sizeof(msg3));
	if (res)
		return -errno;
	res = client.Recv(buff, sizeof(buff));
	if (res)
		return -errno;
	std::cout << '"' << buff << "\"\n";
	res = client.Send(msg4, sizeof(msg4));
	if (res)
		return -errno;
	res = client.Recv(buff, sizeof(buff));
	if (res)
		return -errno;
	std::cout << '"' << buff << "\"\n";
	client.Close(); // can be explicitly closed
	std::cout << "kthxbye\n";
	return 0;
}