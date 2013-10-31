cmake_minimum_required(VERSION 2.8)
project(gason)

option(BUILD_X86 "Build for x86 instruction set" OFF)
option(SHOOTOUT "Create parser-shootout" OFF)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Default type of build - Debug" FORCE)
endif()

foreach(lang C CXX)
	set(CMAKE_${lang}_FLAGS "-Wall -Wextra -march=corei7 -msse4")
	set(CMAKE_${lang}_FLAGS_DEBUG "-g")
	set(CMAKE_${lang}_FLAGS_RELEASE "-O3 -DNDEBUG")
	set(CMAKE_${lang}_FLAGS_MINSIZEREL "-Os -DNDEBUG")
	set(CMAKE_${lang}_FLAGS_RELWITHDEBINFO "${CMAKE_${lang}_FLAGS_RELEASE} -g")
	if("${CMAKE_${lang}_COMPILER_ID}" STREQUAL "Clang")
		set(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -ferror-limit=4")
	endif()
	if(BUILD_X86)
		set(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -m32")
	endif()
endforeach()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

add_executable(gason-pp gason.cpp pretty-print.cpp)
if(SHOOTOUT)
	execute_process(
		COMMAND hg clone https://code.google.com/p/vjson/
		COMMAND git clone https://github.com/chadaustin/sajson.git
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	add_executable(json-parser-shootout gason.cpp parser-shootout.cpp vjson/block_allocator.cpp vjson/json.cpp)
endif()
