cmake_minimum_required(VERSION 3.16)
project(lua_compiler VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)

