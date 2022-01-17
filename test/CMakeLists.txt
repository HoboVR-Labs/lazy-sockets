cmake_minimum_required(VERSION 3.16)

project(lazy_socket_test)

include_directories(
	"../src/"
)

add_executable(lsc_test
	test.cpp
)

target_link_libraries(lsc_test
	-lpthread
	lazy_sockets
)