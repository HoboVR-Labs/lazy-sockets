cmake_minimum_required(VERSION 3.16)

project(lazy_socket_test)

include_directories(
	"../src/"
)

add_executable(lsc_test
	test.cpp
)

add_executable(lsc_test2
	test2.cpp
)

target_link_libraries(lsc_test
	-lpthread
	lazy_sockets
)

target_link_libraries(lsc_test2
	-lpthread
	lazy_sockets
)