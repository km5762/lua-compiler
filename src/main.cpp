#include "compilation.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Expected path to program\n";
    return EXIT_FAILURE;
  }

  bool disassemble{false};
  std::string path{};
  for (std::size_t i{1}; i < argc; ++i) {
    std::string_view arg{argv[i]};
    if (arg == "--disassemble" || arg == "-d") {
      disassemble = true;
    } else {
      path = arg;
    }
  }

  if (path.empty()) {
    std::cerr << "Expected path to program\n";
    return EXIT_FAILURE;
  }

  std::ifstream file{path};
  if (!file.is_open()) {
    std::cerr << "Error opening program: " << path << '\n';
    return EXIT_FAILURE;
  }

  std::string program{std::istreambuf_iterator<char>{file},
                      std::istreambuf_iterator<char>{}};

  if (disassemble) {
    std::optional<Error> error{compilation::disassemble(program)};
    if (error) {
      std::cerr << error->message << '\n';
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }

  std::optional<Error> error{compilation::run(program)};
  if (error) {
    std::cerr << error->message << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
