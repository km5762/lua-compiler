add_executable(parser_tests
  parser.cpp
)

target_link_libraries(parser_tests
  PRIVATE
    lua_compiler_lib
)

add_test(NAME parsers_tests COMMAND parser_tests)
