cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/bin)

set(BOOKKEEPER_SOURCES
	src/main.cpp)

set(SOURCES
	${BOOKKEEPER_SOURCES})

add_executable(bookkeeper ${SOURCES})
