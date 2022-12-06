cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(bookkeeper)

set(3RD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/bin)

add_subdirectory(bookkeeper)
add_subdirectory(client)