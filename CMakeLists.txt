cmake_minimum_required(VERSION 2.8.12)
project(gason)

option(SHOOTOUT "Build parser-shootout" OFF)

add_compile_options(-O3 -std=c++11 -fno-rtti -fno-exceptions -Wall -Wextra)

add_executable(gasonpp src/gason.cpp src/pretty-print.cpp)
add_executable(test-suite src/gason.cpp src/test-suite.cpp)
if(SHOOTOUT)
	add_executable(parser-shootout src/parser-shootout.cpp
		src/gason.cpp
		vjson/block_allocator.cpp
		vjson/json.cpp)
endif()
