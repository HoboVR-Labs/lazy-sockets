// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __LAZY_SOCKETS_DEFS
#define __LAZY_SOCKETS_DEFS

#define __LAZY_SOCKETS_VERSION "0.1.4"
#define __LAZY_SOCKETS_BUILD 1642888800  // build date 2022 01 23

#include <errno.h>

// platform defined types
#ifdef LINUX

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include <arpa/inet.h>

// pass through for the socket() call
#define lsocket socket

namespace lsc {

typedef struct sockaddr_in lcsockaddr_in;

typedef int lsocket_t;

}  // namespace lsc

#elif defined(WIN)

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <windows.h>

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

// cuz on windows you can't have nice things
#define MSG_DONTWAIT 0
#define socklen_t int

namespace lsc {

typedef struct sockaddr_in lcsockaddr_in;

typedef SOCKET lsocket_t;

static bool __winsock2_wsa_initialized = false;

// cuz on windows sockets require wsaStartup, call WSACleanup() yourself later, idc
inline lsocket_t lsocket(int family, int type, int protocol) {
	if (!__winsock2_wsa_initialized) {
		WSADATA wsaData;
		int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (res != NO_ERROR) {
			errno = res;
			return INVALID_SOCKET;
		}
		__winsock2_wsa_initialized = true;
	}
	return socket(family, type, protocol);
}

// cuz on windows you can't have accept with flags
inline lsocket_t accept4(lsocket_t soc, struct sockaddr* addr, socklen_t* addrlen, int flags) {
	(void)flags;
	return accept(soc, addr, addrlen);
}

// cuz on windows close() doesn't exist
#define close closesocket

}  // namespace lsc

#else

#error "unsupported platform"

#endif // LINUX/WIN check

#endif // #ifndef __LAZY_SOCKETS_DEFS