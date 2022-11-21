cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/bin)

set(BOOKKEEPER_SOURCES
	src/main.cpp)

set(SOURCES
	${BOOKKEEPER_SOURCES})

set(EXECUTABLE_NAME bookkeeper)

add_executable(${EXECUTABLE_NAME} ${SOURCES})

target_include_directories(${EXECUTABLE_NAME} PRIVATE ${3RD_PARTY_DIR}/asio)
target_link_libraries(${EXECUTABLE_NAME} pthread)

