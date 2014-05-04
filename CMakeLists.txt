cmake_minimum_required(VERSION 2.8.12)
project(gason)

option(SHOOTOUT "Build parser-shootout" OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_compile_options(-stdlib=libc++)
endif()
add_compile_options(-std=c++11 -fno-rtti -fno-exceptions -Wall -Wextra)

add_executable(gason-pp gason.cpp pretty-print.cpp)
add_executable(gason-test-suite gason.cpp test-suite.cpp)
if(SHOOTOUT)
	add_executable(json-parser-shootout parser-shootout.cpp
		gason.cpp
		vjson/block_allocator.cpp
		vjson/json.cpp)
endif()
