cmake_minimum_required(VERSION 3.16)
project(c89_compiler VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
file(GLOB SOURCES "src/*.cpp")

add_executable(c89_compiler ${SOURCES})

