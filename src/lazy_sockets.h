// SPDX-License-Identifier: GPL-2.0-only

// lazy_sockets.h - main library header with prototypes

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __LAZY_SOCKETS
#define __LAZY_SOCKETS

#include "defines.h"  // types, macros, defines, etc.
#include <string>
#include <cstring>



namespace lsc {

// recv flags acceptable for sockets
enum ERecvFlags: int {
	ERecv_none = 0,
	ERecv_nowait = 0x40
	// TODO: add the rest of the flags
};

// send flags acceptable for sockets
enum ESendFlags: int {
	ESend_none = 0,
	ESend_nowait = 0x40
	// TODO: add the rest of the flags
};

// current status of the socket descriptor
enum ESockStatus {
	EStat_invalid		= 0b000000,
	EStat_binded		= 0b000001,
	EStat_connected		= 0b000010,
	EStat_listening		= 0b000100,
	EStat_closed		= 0b001000,
	EStat_cloned		= 0b010000,
	EStat_unknown		= 0b100000

};

// generate an inet socket address using family, string and port
lcsockaddr_in get_inet_addr(int family, std::string addr, int port);

// basic low level socket class
template <int FAMILY, int TYPE, int PROTOCOL>
class LScoket {
public:
	inline LScoket() {
		_status = EStat_invalid;
		_soc_handle = socket(FAMILY, TYPE, PROTOCOL);
	}
	inline ~LScoket() {
		Close();
	}
	inline LScoket(const LScoket<FAMILY, TYPE, PROTOCOL>& other) {
		_soc_handle = other.GetHandle();
		_status = other.GetStatus() | EStat_cloned;
	}
	inline LScoket(lsocket_t other, ESockStatus status) {
		_soc_handle = other;
		_status = status | EStat_cloned;
	}
	inline LScoket(lsocket_t other, int family) {
		_soc_handle = other;
		_status = EStat_unknown | EStat_cloned;
	}


	// opening connection logic


	// claims an address
	inline int Bind(std::string addr, int port) {
		return Bind(get_inet_addr(FAMILY, addr, port));
	}
	inline int Bind(lcsockaddr_in addr) {
		_status = EStat_binded;
		return bind(_soc_handle, (struct sockaddr*)&addr, sizeof(addr));
	}  // if you like to do extra work

	// connects to a claimed address
	inline int Connect(std::string addr, int port) {
		return Connect(get_inet_addr(FAMILY, addr, port));
	}
	inline int Connect(lcsockaddr_in addr)  {
		_status = EStat_connected;
		return connect(_soc_handle, (struct sockaddr*)&addr, sizeof(addr));
	}  // if you like to do extra work

	// static socket methods

	// marks socket as static for accepting incoming connections
	inline int Listen(int backlog) {
		return listen(_soc_handle, backlog);
	}

	// blocks until there is an incoming connection if the socket is static
	// returns a socket descriptor
	inline lsocket_t Accept(
		struct sockaddr *addr = nullptr,
		socklen_t *addrlen = nullptr
	) {
		_status = EStat_listening;
		return accept(_soc_handle, addr, addrlen);
	}


	// receive/send logic

	// receives a :size: bytes into :buffer:
	// returns the number of received bytes
	inline int Recv(void* buff, size_t size, ERecvFlags flags=ERecv_none) {
		return recv(_soc_handle, buff, size, flags);
	}

	// sends a :size: bytes from :buffer:
	// returns the number of sent bytes
	inline int Send(const void* buff, size_t size, ESendFlags flags=ESend_none) {
		return send(_soc_handle, buff, size, flags);
	}


	// closing connection logic


	// close currently open connection, returns error code
	inline int Close()  {
		_status = EStat_closed;
		return close(_soc_handle);
	}


	// utility


	// returns the current status of the socket
	inline ESockStatus GetStatus() {
		return _status;
	}

	// returns the raw socket handle, use this at your own risk!
	inline lsocket_t GetHandle() {
		return _soc_handle;
	}

	// returns the socket family, use this at your own risk!
	inline static constexpr int GetFamily() {
		return FAMILY;
	}

private:
	lsocket_t _soc_handle;
	int _status;
};

// receives into :buff: until
// either a :tag: is encountered or received more than :size:
// returns number of bytes filled in :buff:
template<int FAMILY, int TYPE, int PROTOCOL, typename T>
int receive_until_tag(LScoket<FAMILY, TYPE, PROTOCOL> soc, char* buff, size_t size, T* tag);

// template<class Callback, typename >
// class RecvLoop {
// public:
// 	RecvLoop(LScoket, Callback cb);

// private:

// };

}  // namespace ls

#endif // #ifndef __LAZY_SOCKETS