cmake_minimum_required(VERSION 3.16)
project(lua_compiler VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY "https://github.com/nlohmann/json.git"
  GIT_TAG "v3.12.0"
)

FetchContent_MakeAvailable(nlohmann_json)

include_directories(include)
file(GLOB SOURCES "src/*.cpp")

add_executable(lua_compiler ${SOURCES})
target_link_libraries(lua_compiler nlohmann_json)

