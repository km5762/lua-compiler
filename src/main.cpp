#include "parser.hpp"
#include "scanner.hpp"

#include <iostream>

int main(int argc, char *argv[]) {
  std::string_view program = R"(local a, b, c = 1, 2 + 3 * 4, -5
  a, b = b ^ 2, a + c
  c = a + b * 2 - 3 ^ 2
  a = -a
  local cmp1 = a < b
  local cmp2 = b >= c
  local cmp3 = a == c
  local cmp4 = a ~= b
  local logic = cmp1 and cmp2 or cmp3
  local concat = a .. b .. c
  local expr = (a + b) * (c - 1) ^ 2 and -c or a + b .. c
  return a, b, c, logic, concat, expr)";

  Scanner scanner{program};
  AstNode::Ptr ast{Parser::parse(scanner)};

  if (ast) {
    std::cout << ast->toJson().dump() << std::endl;
  }
}
