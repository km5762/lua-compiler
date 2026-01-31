add_executable(parser_tests
  parser.cpp
)

add_executable(update_tests
  update.cpp
)

target_link_libraries(parser_tests
  PRIVATE
    lua_compiler_lib
)

target_link_libraries(update_tests
  PRIVATE
    lua_compiler_lib
)

add_test(NAME parser_tests COMMAND parser_tests)
