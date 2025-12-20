#include "parser.hpp"
#include "scanner.hpp"

#include <iostream>
#include <memory_resource>

int main(int argc, char *argv[]) {
  std::string_view program = R"(
local a, b, c = 1, 2 + 3 * 4, -(5 ^ 2);
local x, y;
x = a + b
y = x % 3
local flag = a < b and b ~= c or not y
local z = a .. b .. c
local p = (a + b) * c
obj.value = arr[1 + 2] * obj.other
result = obj.inner.field[10].leaf
foo(a, b, c)
bar((a + b) * c, not flag)
obj:method(a, b + c)
obj:getInner().compute(a ^ b, -c)
    break;
  )";

  Scanner scanner{program};
  std::pmr::monotonic_buffer_resource allocator{1024};
  AstNode::Ptr ast{Parser::parse(scanner, allocator)};

  if (ast) {
    std::cout << ast->toJson().dump() << std::endl;
  }
}
