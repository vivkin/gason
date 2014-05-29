cmake_minimum_required(VERSION 2.8.12)
project(gason)

option(SHOOTOUT "Build parser-shootout" OFF)

add_compile_options(-O3 -std=c++11 -fno-rtti -fno-exceptions -Wall -Wextra)

add_executable(gasonpp gason.cpp pretty-print.cpp)
add_executable(test-suite gason.cpp test-suite.cpp)
if(SHOOTOUT)
	add_executable(parser-shootout parser-shootout.cpp
		gason.cpp
		vjson/block_allocator.cpp
		vjson/json.cpp)
endif()
