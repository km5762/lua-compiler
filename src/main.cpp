#include "scanner.hpp"

#include <iostream>
#include <ostream>

int main(int argc, char *argv[]) {
  std::string_view program = "int x = 42; return x;";

  Scanner scanner{program};

  while (true) {
    Token token = scanner.next();

    if (token.type() == Token::Type::Eof) {
      break;
    }

    std::cout << token << "\n";
  }

  return 0;
}
