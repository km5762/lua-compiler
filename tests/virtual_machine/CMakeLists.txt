function(add_lua_test testname)
  add_test(NAME ${testname}
    COMMAND $<TARGET_FILE:lua_compiler> "${CMAKE_CURRENT_SOURCE_DIR}/${testname}.lua")
endfunction()

set(LUA_TESTS
  print
  binary_operators
  foo_bar
)

foreach(t IN LISTS LUA_TESTS)
  add_lua_test(${t})
endforeach()

