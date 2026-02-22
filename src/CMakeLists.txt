add_library(lua_compiler_lib
  ast.cpp
  parser.cpp
  scanner.cpp
  bytecode_generator.cpp
  compilation.cpp
  instructions.cpp
  virtual_machine.cpp
  native_functions.cpp
  value.cpp
)

target_include_directories(lua_compiler_lib
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(lua_compiler_lib
  PUBLIC
    nlohmann_json
)

add_executable(lua_compiler main.cpp)
target_link_libraries(lua_compiler PRIVATE lua_compiler_lib)
