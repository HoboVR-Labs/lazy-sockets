// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#include "lazy_sockets.h"


namespace lsc {

lcsockaddr_in get_inet_addr(int family, std::string addr, int port) {
	struct sockaddr_in serv_addr;

	// filling out the bind address info
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = (short)family;
	serv_addr.sin_port = htons((short)port);
	// serv_addr.sin_addr.s_addr = inet_addr(addr.c_str());
	inet_pton(family, addr.c_str(), &serv_addr.sin_addr);

	return serv_addr;
}

}  // namespace lsc
