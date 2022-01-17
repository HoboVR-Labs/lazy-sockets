cmake_minimum_required(VERSION 3.16)

project(lazy_sockets)

option(LSC_BUILD_TESTS "Build tests for lazy_sockets" OFF)

if (LSC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Get platform.
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(PLATFORM_NAME "unix")

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(PLATFORM_NAME "unix") # cuz we lazy up in here
  message(WARNING "OSX version might not work")

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(PLATFORM_NAME "win")

endif()

include_directories("src")

file(GLOB lazy_sockets_src
	"src/*.h"
	"src/common.cpp"
	"src/soc_${PLATFORM_NAME}.cpp"
)

# Add lib
add_library(lazy_sockets STATIC
	${lazy_sockets_src}
	${lazy_sockets_src_platform}
)



if (${PLATFORM_NAME} MATCHES "win")
    target_compile_options(lazy_sockets PRIVATE -W4 -WX)
	target_compile_definitions(lazy_sockets PUBLIC WIN)
elseif (${PLATFORM_NAME} MATCHES "unix")
    target_compile_options(lazy_sockets PRIVATE -Wall -Wextra -pedantic -Werror)
	target_compile_definitions(lazy_sockets PUBLIC LINUX)
endif()

target_link_libraries(lazy_sockets
	-lpthread
)