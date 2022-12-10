cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(bookkeeper-main)

find_package(OpenSSL)

set(BOOKKEEPER_SOURCES
	src/main.cpp)

set(SOURCES
	${BOOKKEEPER_SOURCES})

set(EXECUTABLE_NAME bookkeeper)

add_executable(${EXECUTABLE_NAME} ${SOURCES})

#add_dependencies(${EXECUTABLE_NAME} network)

target_include_directories(${EXECUTABLE_NAME} PRIVATE include
													  ${LIBRARY_DIR}/network/include
													  ${3RD_PARTY_DIR}
													  ${3RD_PARTY_DIR}/asio)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE pthread OpenSSL::SSL OpenSSL::Crypto)

