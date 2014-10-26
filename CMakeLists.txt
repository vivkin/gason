cmake_minimum_required(VERSION 3.0.0)
project(gason VERSION 1.0.0)

add_compile_options(-std=c++11 -fno-rtti -fno-exceptions -Wall -Wextra -O3 -g)

option(USE_SSE "Use SSE 4.2 extensions" OFF)
if(USE_SSE)
    add_compile_options(-msse4)
endif()

option(SHOOTOUT "Build parser-shootout" OFF)
if(SHOOTOUT)
    if(ANDROID)
        add_library(parser-shootout SHARED
            src/parser-shootout.cpp
            src/gason.cpp
            ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
        target_include_directories(${ANDROID_NDK}/sources/android/native_app_glue)
        target_link_libraries(parser-shootout android log)
    else()
        add_executable(parser-shootout src/parser-shootout.cpp src/gason.cpp)
    endif()
    target_compile_options(parser-shootout PRIVATE -Wno-unused-parameter -Wno-ignored-qualifiers)
endif()

if(NOT CMAKE_CROSSCOMPILING)
    add_executable(gasonpp src/gason.cpp src/pretty-print.cpp)
    add_executable(test-suite src/gason.cpp src/test-suite.cpp)
endif()
