// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>


#define NOMINMAX 
#include <iostream>
#include "lazy_sockets.h"
#include <errno.h>

#include <thread>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"


void server() {
	lsc::LSocket<AF_INET, SOCK_STREAM, 0> binder;

	int res = binder.Bind("0.0.0.0", 6968);
	CHECK(!res);

	res = binder.Listen(2);
	CHECK(!res);

	// we can ditch the binder now and create the client socket

	res = binder.Accept();
	CHECK(res>=0);

	lsc::LSocket<AF_INET, SOCK_STREAM, 0> client1(res, lsc::EStat_connected);

	res = binder.Accept();
	CHECK(res>=0);

	lsc::LSocket<AF_INET, SOCK_STREAM, 0> client2(res, lsc::EStat_connected);

	char msg[] = "hello there :P";

	res = client1.Send(msg, strlen(msg));
	CHECK(res>=0);

	std::cout << "binder sent: " << msg << '\n';

	res = client2.Send(msg, strlen(msg));
	CHECK(res>=0);

	std::cout << "binder sent: " << msg << '\n';

	char recv_buff[256];

	res = client1.Recv(recv_buff, sizeof(recv_buff));
	CHECK(res>=0);

	std::cout << "binder got: " << recv_buff << '\n';

	res = client2.Recv(recv_buff, sizeof(recv_buff));
	CHECK(res>=0);

	std::cout << "binder got: " << recv_buff << '\n';
}

void client(int id, std::string msg) {
	lsc::LSocket<AF_INET, SOCK_STREAM, 0> conn;

	int res = conn.Connect("127.0.0.1", 6968);
	CHECK(!res);

	char conn_recv[256];

	res = conn.Recv(conn_recv, sizeof(conn_recv));
	CHECK(res>=0);

	std::cout << "conn" << id << " got: " << conn_recv << '\n';

	res = conn.Send(msg.c_str(), msg.size());
	CHECK(res>=0);

	std::cout << "conn" << id << " sent: " << msg << '\n';
}

TEST_CASE("Simple server and two clients") {
	
	std::thread a(server);
	std::this_thread::sleep_for(std::chrono::milliseconds(1)); // let the server setup first
	std::thread b(client, 1, "lmao hi");
	std::thread c(client, 2, "well hello there");

	a.join();
	b.join();
	c.join();

}