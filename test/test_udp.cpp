#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "lazy_sockets.h"

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/socket.h>
#endif

int send_udp() {
    std::cout <<"send_udp start." << '\n';
    auto addr = "127.0.0.1";
    auto port = 58649;

    auto dest_addr = lsc::get_inet_addr(AF_INET, "127.0.0.1", port);

    lsc::LSocket<AF_INET, SOCK_DGRAM, IPPROTO_UDP> udp_sock;

	char receive_buff[1024];

	int ret = 0;
    int i=1000;// Must be much greater than recv_udp's count. This is UDP, so packets are dropped.
    std::cout <<"send_udp sending." << '\n';
	while (i) {
		ret = udp_sock.SendTo("hi", sizeof("hi"), dest_addr, sizeof(dest_addr), lsc::ESend_none);
        CHECK_MESSAGE(ret>=0, "Error:", lsc::getString(lerrno).c_str());
        i-=1;
	}
    std::cout <<"send_udp sent all messages." << '\n';

	return ret;
}

int recv_udp(){
    std::cout <<"recv_udp start." << '\n';
    lsc::LSocket<AF_INET, SOCK_DGRAM, IPPROTO_UDP> udp_sock;

    auto addr = "0.0.0.0";
    auto port = 58649;

    int res = udp_sock.Bind(addr, port);
    CHECK(!res);
    if (res) return -errno;

    //get sock name
    /*
    getsockname(ReceivingSocket, (SOCKADDR *)&ReceiverAddr, (int *)sizeof(ReceiverAddr));
    printf(Server: Receiving IP(s) used: %s\n, inet_ntoa(ReceiverAddr.sin_addr));
    printf(Server: Receiving port used: %d\n, htons(ReceiverAddr.sin_port));
    printf(Server: I\'m ready to receive a datagram...\n);
    */

	char receive_buff[1024]; 

	int ret = 0;
 
    sockaddr_in from;
    size_t from_size = sizeof(from);

    int i=10;
	while (i) {
		// recv data
        //std::cout <<"Receiving" << '\n';
		int res = udp_sock.RecvFrom(receive_buff, 1024, from, from_size, lsc::ERecv_none);
        CHECK_MESSAGE(res>=0, "Error:", lsc::getString(lerrno).c_str());
        std::cout <<"got: " << receive_buff << '\n';
        CHECK_MESSAGE(strcmp(receive_buff, "hi")==0, "Error:, received ", receive_buff, " instead of 'hi'.");
        char* addr_str = inet_ntoa(from.sin_addr);
        CHECK_MESSAGE(strcmp(addr_str, "127.0.0.1")==0, "Error:, got address ", addr_str, " instead of '127.0.0.1'.");
        i-=1;
	}

	return ret;
}

TEST_CASE("testing the factorial function") {
    std::cout <<"Starting test" << '\n';
	std::thread a(send_udp);
	std::thread b(recv_udp);
	a.join();
	b.join();
}
