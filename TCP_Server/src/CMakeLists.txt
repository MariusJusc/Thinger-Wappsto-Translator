cmake_minimum_required(VERSION 2.8.9)
project(tcp_server)

set(SOURCE_FILES src/main.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -0g")

add_executable(tcp_server ${SOURCE_FILES})
