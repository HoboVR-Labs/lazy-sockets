// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#include "lazy_sockets.h"


namespace lsc {


lcsockaddr_in get_inet_addr(int family, const std::string& addr, int port) {
	struct sockaddr_in serv_addr;

	// filling out the bind address info
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = family;
	serv_addr.sin_port = htons(port);
	inet_aton(addr.c_str(), &serv_addr.sin_addr);

	return serv_addr;
}

}  // namespace lsc
