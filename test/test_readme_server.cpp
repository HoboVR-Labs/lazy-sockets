#include <iostream>
#include "lazy_sockets.h"

int main()
{
	// lets create a socket
	lsc::LSocket<AF_INET, SOCK_STREAM, 0> binder;

	// bind it
	int res = binder.Bind("0.0.0.0", 6969);
	if (res)
		return -errno;

	// make it a static listener
	res = binder.Listen(1);
	if (res)
		return -errno;

	// and accept a single connection
	res = binder.Accept();
	if (res < 0)
		return -errno;

	// convert that connection from a raw socket to an LScocket and profit
	lsc::LSocket<AF_INET, SOCK_STREAM, 0> client(res, lsc::EStat_connected);

	char receive_buffer[256];
	int ret = 0;
	while (1)
	{
		// receive data
		res = client.Recv(receive_buffer, sizeof(receive_buffer));
		if (res < 0)
		{
			ret = -errno;
			break;
		} // connection closed

		// re send what we received
		res = client.Send(receive_buffer, res);
		if (res < 0)
		{
			ret = -errno;
			break;
		} // connection closed
	}
	// all socked will be closed on class instance destruction
	return ret;
}