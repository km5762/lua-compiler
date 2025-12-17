#include "parser.hpp"

#include <iostream>

int main(int argc, char *argv[]) {
  std::string_view program = R"(
    local x = 10
    local y = 5
    local result = x - y - 2
    return result
  )";

  Scanner scanner{program};

  while (true) {
    AstNode::Ptr token = Parser::parse(scanner);

    if (token) {
      std::cout << token->toString();
    }

    return 0;
  }
}
