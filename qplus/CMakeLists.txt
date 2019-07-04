cmake_minimum_required(VERSION 2.8)

project(qplus)

# The version number.
set (MAJOR_VERSION 0)
set (MINOR_VERSION 1)
set (PATCH_VERSION 1)

message(STATUS "Current project dir: ${CMAKE_CURRENT_SOURCE_DIR}")

if (NOT EXISTS "${CMAKE_SOURCE_DIR}/src/${PROJECT_NAME}.conf")
    message(FATAL_ERROR "Can NOT find configuration file: ${PROJECT_NAME}.conf in path: ${CMAKE_SOURCE_DIR}/src")
endif()

set(CMAKE_INSTALL_PREFIX "/opt")
set (QPLUS_PATH    "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}")
set (QPLUS_CONF    "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/${PROJECT_NAME}.conf")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/configuration.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/configuration.hpp")

message(STATUS "Configuring ${PROJECT_NAME} ${SH_MAJOR}.${SH_MINOR}.${SH_PATCH}")
message(STATUS "${PROJECT_NAME} path: ${QPLUS_PATH}")

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

add_definitions(-D_GLIBCXX_USE_C99)
set(CMAKE_CXX_FLAGS "-g --std=c++11 -Wno-psabi")

find_package(EV REQUIRED) 
find_package(OpenSSL REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    )

link_directories(${CMAKE_BINARY_DIR}/lib)

add_executable(${PROJECT_NAME} 
    ${CMAKE_SOURCE_DIR}/src/sslclient.cpp
    ${CMAKE_SOURCE_DIR}/src/localsocket.cpp
    ${CMAKE_SOURCE_DIR}/src/client.cpp
    ${CMAKE_SOURCE_DIR}/src/state.cpp
    ${CMAKE_SOURCE_DIR}/src/value.cpp
    ${CMAKE_SOURCE_DIR}/src/status.cpp
    ${CMAKE_SOURCE_DIR}/src/device.cpp
    ${CMAKE_SOURCE_DIR}/src/network.cpp
    ${CMAKE_SOURCE_DIR}/src/root.cpp
    ${CMAKE_SOURCE_DIR}/src/path.cpp
    ${CMAKE_SOURCE_DIR}/src/serializer.cpp
    ${CMAKE_SOURCE_DIR}/src/dataup.cpp
    ${CMAKE_SOURCE_DIR}/src/util.cpp
    ${CMAKE_SOURCE_DIR}/src/ventilator.cpp
    ${CMAKE_SOURCE_DIR}/src/backendsocket.cpp
    )

target_link_libraries(${PROJECT_NAME} 
    ${LIBEV_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    pthread
    )


install (
	FILES "${CMAKE_SOURCE_DIR}/src/${PROJECT_NAME}.conf" 
	DESTINATION "${QPLUS_PATH}"
)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${QPLUS_PATH}
)

install(DIRECTORY DESTINATION "${QPLUS_PATH}/certs")
install(DIRECTORY DESTINATION "${QPLUS_PATH}/network")
install(DIRECTORY DESTINATION "${QPLUS_PATH}/logs")
