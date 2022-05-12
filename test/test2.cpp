// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>


#define NOMINMAX 
#include <iostream>
#include "lazy_sockets.h"
#include <errno.h>

#include <thread>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

using namespace lsc;

using tcp_socket = LSocket<AF_INET, SOCK_STREAM, 0>;

void server_echo(bool* echo_alive) {
	// lets create a socket
	tcp_socket binder;

	// bind it
	int res = binder.Bind("0.0.0.0", 6969);
	std::cout<<res<<std::endl;
	CHECK_MESSAGE(res==0, "Error:", getString(lerrno).c_str());


	// make it a static listener
	CHECK(binder.Listen(1)==0);

	// and accept a single connection
	res = binder.Accept();
	CHECK_MESSAGE(res >= 0, "Error:", getString(lerrno).c_str());

	// convert that connection from a raw socket to an LScocket and profit
	LSocket<AF_INET, SOCK_STREAM, 0> client(res, EStat_connected);

	char buff[256]; // receive buffer

	while (*echo_alive) {
		// receive data
		res = client.Recv(buff, sizeof(buff));
		CHECK(res >= 0);

		// re send what we received
		res = client.Send(buff, res);
		CHECK(res >= 0);
	}

	// all socked will be closed on class instance destruction
}

struct my_tag {
	char a;
	char b;
	char c;
	char d;
};

static constexpr int MSG_SIZE = 1024 * 9;

void call_back(void* buff, size_t len) {

	std::cout << "'...,";

	for (int i=MSG_SIZE / 4  - 20; i < len / 4; i++)
		std::cout << (((int*)buff)[i]) << ',';

	std::cout << "'<- " << len / 4 << "\n";

	// memset(buff, 0, len + sizeof(my_tag));
}



TEST_CASE("Threaded send and receive") {
	bool echo_alive = true;
	std::thread echo_thread(server_echo, &echo_alive);

	// let the server setup first
	std::this_thread::sleep_for(std::chrono::milliseconds(1));


	auto client = std::make_shared<tcp_socket>();

	CHECK(client->Connect("127.0.0.1", 6969)==0);

	my_tag tag = {'\r', '\t', '\n', '\0'};

	ThreadedRecvLoop<AF_INET, SOCK_STREAM, 0, my_tag> recv_loop(client, tag, call_back, 2);

	recv_loop.Start();

	void* msg = malloc(MSG_SIZE + sizeof(my_tag));
	// for (int i=0; i<MSG_SIZE; msg[i]=i, i++);
	int* tmp = (int*)msg;
	for (int i=0; i < MSG_SIZE / 4; tmp[i]=i, i++);

	memcpy((char*)msg + MSG_SIZE, &tag, sizeof(tag));


	// send loop

	for (int i=0; i<10; i++) {
		int res = client->Send(msg, MSG_SIZE + sizeof(my_tag));
		CHECK(res >= 0);
	
		std::this_thread::sleep_for(std::chrono::milliseconds(2)); // let the server setup first
	}


	echo_alive = false;

	// to unblock the recv wait in the server thread
	int res = client->Send(msg, MSG_SIZE + sizeof(my_tag));
	CHECK(res >= 0);

	// releasing ownership of the receiver socket shared_ptr
	// will cause the receiver to exit it's thread
	client.reset();

	recv_loop.Stop(); // call it to verify receiver thread is joined
	echo_thread.join();

	std::cout << "done\n";
}