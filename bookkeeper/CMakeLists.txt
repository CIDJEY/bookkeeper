cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(bookkeeper-main)

find_package(OpenSSL)

set(BOOKKEEPER_SOURCES
	src/main.cpp)

set(SOURCES
	${BOOKKEEPER_SOURCES})

set(EXECUTABLE_NAME bookkeeper)

add_executable(${EXECUTABLE_NAME} ${SOURCES})

target_include_directories(${EXECUTABLE_NAME} PRIVATE ${3RD_PARTY_DIR})

target_link_libraries(${EXECUTABLE_NAME} PRIVATE pthread OpenSSL::SSL OpenSSL::Crypto)

