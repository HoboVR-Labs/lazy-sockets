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

private:
	lsocket_t _soc_handle;
	int _status;
};

template<int FAM, int TYP, int PROTO, typename T>
// requirecv_len std::invocable<CB&, void*, size_t>  // because compilers are a bitch i can't have concepts right now
// has to be a purely inlined class because template bs
class ThreadedRecvLoop {
public:
	inline ThreadedRecvLoop(
		LSocket<FAM, TYP, PROTO>* soc,
		T* tag,
		std::function<void(void*, size_t)> callback,
		size_t recv_buff_size = 256
	) {
		m_soc = soc;
		m_callback = callback;
		m_tag = tag;
		m_buff_size = (int)recv_buff_size;
		m_is_alive = false;
	}

	inline void Start() {
		m_is_alive = true;
		m_thread = std::make_unique<std::thread>(ThreadedRecvLoop::thread_enter, this);
	}

	inline void Stop() {
		m_is_alive = false;
		m_thread->join();
	}

	inline void ReallocInternalBuffer(size_t new_size) {
		m_buff_size = new_size;
		m_realloc_buff = true;
	}

	inline size_t GetBufferSize() {
		return m_buff_size;
	}


private:
	inline static void thread_enter(ThreadedRecvLoop* self) {
		self->thread_internal();
	}

	inline void thread_internal() {

		char* recv_buff = new char[m_buff_size + sizeof(*m_tag)];
		m_realloc_buff = false;
		int recv_off = 0;
		int recv_len;

		while (m_is_alive) {

			// check for a realloc call
			if (m_realloc_buff){
				recv_buff = (char*)realloc(recv_buff, m_buff_size + sizeof(*m_tag));
				m_realloc_buff = false;
			}

			// check for too small of a buffer
			if (m_buff_size - recv_off <= 0) {
				m_buff_size += recv_off;
				recv_buff = (char*)realloc(recv_buff, m_buff_size + sizeof(*m_tag));
			}

			// receive a partial message
			recv_len = m_soc->Recv(recv_buff + recv_off, m_buff_size - recv_off);

			// check for an error
			if (recv_len < 0) {
				break;
			}

			recv_off += recv_len;

			// now look for a packet end tag
			for (int i=0;  i < recv_off; i++) {
				if (memcmp(recv_buff + i, m_tag, sizeof(*m_tag)) == 0) {
					if (m_callback)
						m_callback(recv_buff, i);

					// after the packet was processed,
					// we need to move the rest of the buffer
					// to the begging to not lose any data
					int off = i + sizeof(*m_tag);
					if (m_buff_size - off > 0) {
						memmove(recv_buff, recv_buff + off, m_buff_size - off);
						memset(recv_buff + i, 0, m_buff_size - i);
					}

					recv_off = 0; // and zero off the offset
					break;
				}
			}

		}


		delete[] recv_buff; // we're done, we can free the buffer
	}

private:
	LSocket<FAM, TYP, PROTO>* m_soc;
	std::function<void(void*, size_t)> m_callback;

	const T* m_tag;

	int m_buff_size;

	std::unique_ptr<std::thread> m_thread;
	bool m_is_alive;
	bool m_realloc_buff; // sync buff alloc trigger
};

}  // namespace ls

#endif // #ifndef __LAZY_SOCKETS