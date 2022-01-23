// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __LAZY_SOCKETS_DEFS
#define __LAZY_SOCKETS_DEFS

#define __LAZY_SOCKETS_VERSION "0.1.4"
#define __LAZY_SOCKETS_BUILD 1642888800  // build date 2022 01 23

// platform defined types
#ifdef LINUX

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include <arpa/inet.h>


namespace lsc {

typedef struct sockaddr_in lcsockaddr_in;

typedef int lsocket_t;

}  // namespace lsc

#elif defined(WIN)

#include <winsock2.h>
#include <stdio.h>
#include <windows.h>

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

namespace lsc {

typedef struct sockaddr_in lcsockaddr_in;

typedef SOCKET lsocket_t;

}  // namespace lsc

#else

#error "unsupported platform"

#endif // LINUX/WIN check

#endif // #ifndef __LAZY_SOCKETS_DEFS