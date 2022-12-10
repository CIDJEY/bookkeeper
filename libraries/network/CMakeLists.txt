cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(network)

find_package(OpenSSL)

set(NETWORK_SOURCES
		src/channel.cpp
		src/delegate_socket.cpp)

set(SOURCES
	${NETWORK_SOURCES})

set(EXECUTABLE_NAME ${PROJECT_NAME})

add_library(${EXECUTABLE_NAME} ${SOURCES})

target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)

target_include_directories(${EXECUTABLE_NAME} PRIVATE ${3RD_PARTY_DIR}
													  ${3RD_PARTY_DIR}/asio)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE pthread OpenSSL::SSL OpenSSL::Crypto)