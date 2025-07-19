cmake_minimum_required(VERSION 3.20.0)

project(rflx-test VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finput-charset=UTF-8 -fexec-charset=UTF-8 -freflection -fexpansion-statements")

add_executable(rflx-test test/main.cpp test/test.cpp)
target_include_directories(rflx-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
