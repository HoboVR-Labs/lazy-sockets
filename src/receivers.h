#ifndef __LAZY_SOCKET_RECEIVERS
#define __LAZY_SOCKET_RECEIVERS

// include parent header
#include "lazy_sockets.h"


///////////////////////////////////////////////////////////////////////////////
// ThreadedRecvLoop - a threaded receive loop, which will receive
//			 until in encounters a :tag: after wich the callback will be called
///////////////////////////////////////////////////////////////////////////////


// has to be a purely inlined class because template bs
template<int FAM, int TYP, int PROTO, typename T>
// requires std::invocable<CB&, void*, size_t>  // because compilers are a bitch i can't have concepts right now
class ThreadedRecvLoop {
public:
	inline ThreadedRecvLoop(
		std::shared_ptr<LSocket<FAM, TYP, PROTO>> soc,
		const T& tag,
		std::function<void(void*, size_t)> callback,
		size_t recv_buff_size = 256
	): m_soc(soc), m_callback(callback), m_tag(tag) {
		m_buff_size = (int)recv_buff_size;
	}

	inline ~ThreadedRecvLoop() {
		Stop();
	}

	inline void Start() {
		m_is_alive = true;
		m_thread = std::make_unique<std::thread>(ThreadedRecvLoop::thread_enter, this);
	}

	inline bool IsAlive() {
		return m_is_alive;
	}

	inline void Stop() {
		m_is_alive = false;
		if (m_thread)
			if (m_thread->joinable())
				m_thread->join();
	}

	inline void ReallocInternalBuffer(size_t new_size) {
		m_buff_size = (int)new_size;
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

		char* recv_buff = new char[m_buff_size + sizeof(m_tag)];
		int recv_off = 0;
		int recv_len;

		while (m_is_alive) {

			// early stopping,
			// if we're the only owner we need to stop,
			// original socket owner either released ownership
			// OR we were given an improperly constructed socket
			if (m_soc.use_count() == 1)
				break;

			// check for a realloc call
			if (m_realloc_buff){
				recv_buff = (char*)realloc(recv_buff, m_buff_size + sizeof(m_tag));
				m_realloc_buff = false;
			}

			// check for too small of a buffer
			if (m_buff_size - recv_off <= 0) {
				m_buff_size += recv_off;
				recv_buff = (char*)realloc(recv_buff, m_buff_size + sizeof(m_tag));
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
				if (memcmp(recv_buff + i, &m_tag, sizeof(m_tag)) == 0) {
					if (m_callback)
						m_callback(recv_buff, i);

					// after the packet was processed,
					// we need to move the rest of the buffer
					// to the begging to not lose any data
					int off = i + sizeof(m_tag);
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

		// this can be used as a signal by other threads,
		// so we need to reset it here
		m_is_alive = false;
	}

private:
	std::shared_ptr<LSocket<FAM, TYP, PROTO>> m_soc;
	std::function<void(void*, size_t)> m_callback;

	const T& m_tag;

	int m_buff_size;

	std::unique_ptr<std::thread> m_thread = nullptr;
	bool m_is_alive;
	bool m_realloc_buff; // sync buff alloc trigger
}; // class ThreadedRecvLoop 

#endif // #ifndef __LAZY_SOCKET_RECEIVERS