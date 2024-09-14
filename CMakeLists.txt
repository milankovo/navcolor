cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(navcolor)

# Plugin 1
set(CMAKE_CXX_STANDARD 17)

include($ENV{IDASDK}/ida-cmake/common.cmake)

set(PLUGIN_NAME              navcolor)
set(PLUGIN_SOURCES           navcolor.cpp)
set(PLUGIN_RUN_ARGS          "-t -z10000") # Debug messages for the debugger
generate()
disable_ida_warnings(navcolor)
