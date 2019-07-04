cmake_minimum_required(VERSION 2.8.9)
project(tcp_server)

set(CMAKE_CXX_FLAGS "-g")
set(SOURCE_FILES src/main.cpp)

add_executable(tcp_server ${SOURCE_FILES})
