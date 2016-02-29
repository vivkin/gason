cmake_minimum_required(VERSION 2.8.12)
project(gason)
include(GNUInstallDirs)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

add_compile_options(-Wall -Wextra)

option(BUILD_SHARED_LIBS "build as shared library" ON)
option(BUILD_STATIC_LIBS "build as static library" OFF)
option(BUILD_TEST_AND_BENCHMARK "build test and benchmark" ON)

if(BUILD_SHARED_LIBS)
	add_library(gason SHARED src/gason.cpp)

	set_target_properties(gason PROPERTIES
		COMPILE_DEFINITIONS "GASON_EXPORT"
		VERSION "${GENERIC_LIB_VERSION}"
		SOVERSION "${GENERIC_LIB_SOVERSION}")

	install(TARGETS gason
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if(BUILD_STATIC_LIBS)
	add_library(gason_static STATIC src/gason.cpp)
	set_target_properties(gason_static PROPERTIES OUTPUT_NAME gason)

	install(TARGETS gason_static
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if (BUILD_TEST_AND_BENCHMARK)
	if(BUILD_SHARED_LIBS)
	link_libraries(gason)
	else()
	link_libraries(gason_static)
	endif()

	add_executable(test-suite src/test-suite.cpp)
	add_executable(gasonpp src/pretty-print.cpp)
	add_executable(benchmark src/benchmark.cpp)
	target_include_directories(benchmark PRIVATE rapidjson/include)
endif()
