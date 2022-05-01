// SPDX-License-Identifier: GPL-2.0-only

// lazy_sockets.h - main library header with prototypes

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __LAZY_SOCKETS
#define __LAZY_SOCKETS

#include "defines.h"  // types, macros, defines, etc.
#include <string>
#include <cstring>
#include <errno.h>

#include <thread>
#include <functional>
#include <memory>


namespace lsc {

// recv flags acceptable for sockets
enum ERecvFlags: int {
	ERecv_none = 0,
	ERecv_nowait = MSG_DONTWAIT
	// TODO: add the rest of the flags
};

// send flags acceptable for sockets
enum ESendFlags: int {
	ESend_none = 0,
	ESend_nowait = MSG_DONTWAIT
	// TODO: add the rest of the flags
};

// current status of the socket descriptor
enum ESockStatus: int {
	EStat_invalid		= 0b0000000,
	EStat_binded		= 0b0000001,
	EStat_connected		= 0b0000010,
	EStat_listening		= 0b0000100,
	EStat_static		= 0b0001000,
	EStat_closed		= 0b0010000,
	EStat_cloned		= 0b0100000,
	EStat_unknown		= 0b1000000

};

// Accept error return
enum EAcceptErrors {
	EAccept_error = LSOCK_ERR,
};

// generate a lcsockaddr_in using :family:, :addr: and :port:
lcsockaddr_in get_inet_addr(int family, const std::string& addr, int port);


////////////////////////////////////////////////////////////////////////////////
// LSocket - main socket class
////////////////////////////////////////////////////////////////////////////////

// basic low level socket class
// has to be a purely inlined class because template bs
template <int FAMILY, int TYPE, int PROTOCOL>
class LSocket {
	lsocket_t _soc_handle;
	int _status = EStat_invalid;

public:
	/////////////////////////////
	// constructors/destructors
	/////////////////////////////

	// default constructor
	inline LSocket() {
		_soc_handle = lsocket(FAMILY, TYPE, PROTOCOL);
	}

	// from raw socket constructor
	inline LSocket(
		lsocket_t other,
		ESockStatus status = EStat_unknown
	): _soc_handle(other), _status(status) {}


	// copy constructor
	inline LSocket(const LSocket<FAMILY, TYPE, PROTOCOL>& other) {
		_soc_handle = other._soc_handle;
		_status = other._status | EStat_cloned;  // if we're a copy, remember that
	}

	// move constructor
	inline LSocket(LSocket<FAMILY, TYPE, PROTOCOL>&& other) {
		_soc_handle = other._soc_handle;
		_status = other._status;  // if we're moved, no cloned flag
	}

	// destructor
	inline ~LSocket() {
		if (!(_status & EStat_cloned))  // if we're a clone, don't close
			Close();
	}

	/////////////////////////////
	// assign operators
	/////////////////////////////


	// copy assign
	inline LSocket& operator=(const LSocket& other) {
		if (&other != this) { // no self copies
			// yeah we still move on a copy,
			// but only because functionally the code is identical
			*this = std::move(other);
			// except that we mark as cloned at the end
			_status |= EStat_cloned;
		}
		return *this;
	}

	// move assign
	inline LSocket& operator=(LSocket&& other) {
		if (&other != this) { // no self copies
			// cleanup old fd before taking a new one
			if (!(_status & EStat_cloned))  // if we're a clone, don't close
				Close();

			_status = std::move(other._status);
			_soc_handle = std::move(other._soc_handle);
		}
		return *this;	
	}


	/////////////////////////////
	// opening connection logic
	/////////////////////////////


	// claims an address
	// returns 0 on success or -1 on error
	inline int Bind(const std::string& addr, int port) {
		return Bind(get_inet_addr(FAMILY, addr, port));
	}
	// returns 0 on success or -1 on error
	inline int Bind(lcsockaddr_in addr) {
		if (_status) {errno = EISCONN; return -1;}
		_status = EStat_binded;
		return bind(_soc_handle, (struct sockaddr*)&addr, sizeof(addr));
	}  // if you like to do extra work

	// connects to a claimed address
	// returns 0 on success or -1 on error
	inline int Connect(const std::string& addr, int port) {
		return Connect(get_inet_addr(FAMILY, addr, port));
	}

	// returns 0 on success or -1 on error
	inline int Connect(lcsockaddr_in addr)  {
		if (_status) {errno = EISCONN; return -1;}
		_status = EStat_connected;
		return connect(_soc_handle, (struct sockaddr*)&addr, sizeof(addr));
	}  // if you like to do extra work


	/////////////////////////////
	// static socket methods
	/////////////////////////////

	// marks socket as static for accepting incoming connections
	// returns 0 on success or -1 on error
	inline int Listen(int backlog) {
		_status = EStat_static;
		return listen(_soc_handle, backlog);
	}

	// blocks until there is an incoming connection if the socket is static
	// returns a socket descriptor or -1 on error
	inline lsocket_t Accept(
		struct sockaddr *addr = nullptr,
		socklen_t *addrlen = nullptr,
		int flags = 0
	) {
		_status = EStat_listening;
		return accept4(_soc_handle, addr, addrlen, flags);
	}


	/////////////////////////////
	// receive/send logic
	/////////////////////////////

	// receives a :size: bytes into :buffer:
	// returns the number of received bytes or -1 on error
	inline int Recv(void* buff, size_t size, ERecvFlags flags=ERecv_none) {
		// compile time check for windows stupidity
		#ifdef WIN
		return recv(_soc_handle, (char*)buff, (int)size, flags);
		#elif defined(LINUX)
		return recv(_soc_handle, buff, size, flags);
		#else
		#error "unsupported platform"
		#endif // #ifdef WIN
	}

	// sends a :size: bytes from :buffer:
	// returns the number of sent bytes or -1 on error
	inline int Send(const void* buff, size_t size, ESendFlags flags=ESend_none) {
		// compile time check for windows stupidity
		#ifdef WIN
		return send(_soc_handle, (const char*)buff, (int)size, flags);
		#elif defined(LINUX)
		return send(_soc_handle, buff, size, flags);
		#else
		#error "unsupported platform"
		#endif // #ifdef WIN
	}


	/////////////////////////////
	// closing connection logic
	/////////////////////////////


	// close currently open connection, returns error code
	// returns 0 on success or -1 on error
	inline int Close()  {
		_status = EStat_closed;
		return close(_soc_handle);
	}


	/////////////////////////////
	// utility
	/////////////////////////////

	inline int Ioctl(unsigned long request, void* argp) {
		return ioctl(_soc_handle, request, argp);
	}


	// returns the current status of the socket
	inline ESockStatus GetStatus() {
		return (ESockStatus)_status;
	}

	// returns the raw socket handle, use this at your own risk!
	inline lsocket_t GetHandle() {
		return _soc_handle;
	}

	// returns the socket family
	inline static constexpr int GetFamily() {
		return FAMILY;
	}

	// returns the socket type
	inline static constexpr int GetType() {
		return TYPE;
	}

	// returns the socket protocol
	inline static constexpr int GetProtocol() {
		return PROTOCOL;
	}

}; // class LSocket {

}  // namespace lsc


// include utility interfaces
#include "receivers.h"

#endif // #ifndef __LAZY_SOCKETS