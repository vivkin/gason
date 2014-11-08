cmake_minimum_required(VERSION 3.0.0)
project(gason VERSION 1.0.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

option(WITH_SSE "Use SSE 4.2 extensions" OFF)
if(WITH_SSE)
    add_compile_options(-msse4)
endif()

add_compile_options(-Wall -Wextra)

add_library(gason STATIC src/gason.cpp)
link_libraries(gason)
add_executable(test-suite src/test-suite.cpp)
add_executable(gasonpp src/pretty-print.cpp)
add_executable(benchmark src/benchmark.cpp)
target_include_directories(benchmark PRIVATE shootout/rapidjson/include)
