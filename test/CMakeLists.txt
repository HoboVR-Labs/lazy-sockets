cmake_minimum_required(VERSION 3.16)

project(lazy_socket_test)

include(CTest)
enable_testing()

include_directories(
	"../src/"
)

add_executable(lsc_test
	test.cpp
)

add_executable(lsc_test2
	test2.cpp doctest.h
)

add_executable(driver_socket_mock
	driver_socket_mock.cpp doctest.h
	timer.cpp timer.h packets.h
)

add_executable(udp_test
	test_udp.cpp doctest.h
)

target_link_libraries(lsc_test
	-lpthread
	lazy_sockets
)

target_link_libraries(lsc_test2
	-lpthread
	lazy_sockets
)

target_link_libraries(driver_socket_mock
	-lpthread
	lazy_sockets
)

target_link_libraries(udp_test
	-lpthread
	lazy_sockets
)

set(ENV{CTEST_OUTPUT_ON_FAILURE} 1)
target_compile_features(lsc_test2 PRIVATE cxx_std_17)
target_compile_features(driver_socket_mock PRIVATE cxx_std_17)

add_test(NAME test1 COMMAND lsc_test)
add_test(NAME test2 COMMAND lsc_test2)
add_test(NAME test3 COMMAND driver_socket_mock)
add_test(NAME test4 COMMAND udp_test)