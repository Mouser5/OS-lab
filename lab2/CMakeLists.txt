cmake_minimum_required(VERSION 3.10)
project(LinuxIPC_Chat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

file(GLOB CONN_SOURCES "conn_*.cpp")

foreach(SOURCE_FILE ${CONN_SOURCES})
    get_filename_component(FILENAME ${SOURCE_FILE} NAME_WE)
    string(REGEX REPLACE "conn_" "" TYPE_CODE ${FILENAME})
    set(TARGET_NAME "host_${TYPE_CODE}")
    add_executable(${TARGET_NAME} host.cpp ${SOURCE_FILE})
    target_link_libraries(${TARGET_NAME} rt)
    message(STATUS "Generated target: ${TARGET_NAME} using ${SOURCE_FILE}")
endforeach()