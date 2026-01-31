#include "compilation.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Expected path to program\n";
    return EXIT_FAILURE;
  }

  std::string path{argv[1]};
  std::ifstream file{path};
  if (!file.is_open()) {
    std::cerr << "Error opening program: " << path << '\n';
    return EXIT_FAILURE;
  }

  std::string program{std::istreambuf_iterator<char>{file},
                      std::istreambuf_iterator<char>{}};
  compilation::run(program);

  return EXIT_SUCCESS;
}
