// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#include "lazy_sockets.h"


namespace lsc {

template<int FAMILY, int TYPE, int PROTOCOL, typename T>
int receive_until_tag(LScoket<FAMILY, TYPE, PROTOCOL> soc, char* buff, size_t size, size_t& out_offset, T* tag) {
	// receives a message until an end tag is reached

	size_t i = 0;

	for (;;) {
		// Check if we have a complete message
		for( ; i <= out_offset-sizeof(T); i++ ) {
			if(memcmp(buff + i, tag, sizeof(T)) == 0) {
				return i + 3; // return length of message
			}
		}

		int n = soc.Recv(buff + out_offset, size - out_offset);

		if( n < 0 ) {
			return -errno; // operation failed!
		}
		out_offset += n;
	};
}

}  // namespace lsc
