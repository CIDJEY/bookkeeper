cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(client)

find_package(OpenSSL)

set(CLIENT_SOURCES
	src/main.cpp)

set(SOURCES
	${CLIENT_SOURCES})

set(EXECUTABLE_NAME client)

add_executable(${EXECUTABLE_NAME} ${SOURCES})

#add_dependencies(${EXECUTABLE_NAME} network)

target_include_directories(${EXECUTABLE_NAME} PRIVATE ${3RD_PARTY_DIR}
													  ${3RD_PARTY_DIR}/asio)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE pthread OpenSSL::SSL OpenSSL::Crypto)

