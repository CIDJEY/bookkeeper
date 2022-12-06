cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(client)

set(CLIENT_SOURCES
	src/main.cpp)

set(SOURCES
	${CLIENT_SOURCES})

set(EXECUTABLE_NAME client)

add_executable(${EXECUTABLE_NAME} ${SOURCES})

target_include_directories(${EXECUTABLE_NAME} PRIVATE ${3RD_PARTY_DIR}
													  ${3RD_PARTY_DIR}/asio)

