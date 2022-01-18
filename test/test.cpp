// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>


#include <iostream>
#include "lazy_sockets.h"
#include <errno.h>

#include <thread>

#define ASSERT_RES(res) \
	if ((res)) { \
		throw std::runtime_error("assert on line: " + std::to_string(__LINE__) + ", reason: " + std::to_string(errno)); \
	} \
	else (void)0


void server() {
	lsc::LSocket<AF_INET, SOCK_STREAM, 0> binder;

	int res = binder.Bind("0.0.0.0", 6969);
	ASSERT_RES(res);

	res = binder.Listen(2);
	ASSERT_RES(res);

	// we can ditch the binder now and create the client socket

	res = binder.Accept();
	ASSERT_RES(res < 0);

	lsc::LSocket<AF_INET, SOCK_STREAM, 0> client1(res, lsc::EStat_connected);

	res = binder.Accept();
	ASSERT_RES(res < 0);

	lsc::LSocket<AF_INET, SOCK_STREAM, 0> client2(res, lsc::EStat_connected);

	char msg[] = "hello there :P";

	res = client1.Send(msg, strlen(msg));
	ASSERT_RES(res < 0);

	std::cout << "binder sent: " << msg << '\n';

	res = client2.Send(msg, strlen(msg));
	ASSERT_RES(res < 0);

	std::cout << "binder sent: " << msg << '\n';

	char recv_buff[256];

	res = client1.Recv(recv_buff, sizeof(recv_buff));
	ASSERT_RES(res < 0);

	std::cout << "binder got: " << recv_buff << '\n';

	res = client2.Recv(recv_buff, sizeof(recv_buff));
	ASSERT_RES(res < 0);

	std::cout << "binder got: " << recv_buff << '\n';
}

void client(int id, std::string msg) {
	lsc::LSocket<AF_INET, SOCK_STREAM, 0> conn;

	int res = conn.Connect("127.0.0.1", 6969);
	ASSERT_RES(res);

	char conn_recv[256];

	res = conn.Recv(conn_recv, sizeof(conn_recv));
	ASSERT_RES(res < 0);

	std::cout << "conn" << id << " got: " << conn_recv << '\n';

	res = conn.Send(msg.c_str(), msg.size());
	ASSERT_RES(res < 0);

	std::cout << "conn" << id << " sent: " << msg << '\n';
}

int main() {
	
	std::thread a(server);
	std::this_thread::sleep_for(std::chrono::milliseconds(1)); // let the server setup first
	std::thread b(client, 1, "lmao hi");
	std::thread c(client, 2, "well hello there");

	a.join();
	b.join();
	c.join();

	return 0;
}