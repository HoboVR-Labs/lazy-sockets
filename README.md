# lazy-sockets

A simple bare bones socket wrapper library for C++.

# Building
This lib is setup for CMake, subdir it if you're using it as a dependency

Build example
```
mkdir -p build && cd build
cmake ..
cmake --build .
```

To build with tests run
```
mkdir -p build && cd build
cmake -DLSC_BUILD_TESTS=ON ..
cmake --build .
```

# TODO
* Add more receive/send loops
* Fix the windows version
* Other daring new feature? :P

# Examples
## echo server

Simple single peer echo server.

```c++
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
```

## client

Simple client.

```c++
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
```
