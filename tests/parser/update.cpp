#include "ast.hpp"
#include "parser.hpp"

#include <cstdlib>
#include <format>
#include <fstream>
#include <iostream>
#include <memory_resource>

void printFailure(const std::filesystem::path &path, std::string_view message) {
  std::cerr << path.filename().stem() << ": " << message << '\n';
}

int main() {
  std::pmr::monotonic_buffer_resource allocator{};
  std::filesystem::path sourceDir =
      std::filesystem::path(__FILE__).parent_path();
  std::filesystem::path inputPath = sourceDir / "input";
  for (const auto &entry : std::filesystem::directory_iterator{inputPath}) {
    if (!entry.is_regular_file()) {
      continue;
    }
    std::ifstream inputFile{entry.path()};
    std::string input{(std::istreambuf_iterator<char>(inputFile)),
                      std::istreambuf_iterator<char>()};
    std::filesystem::path expectedFilePath{
        sourceDir / std::filesystem::path{"expected"} / entry.path().stem()};
    expectedFilePath += ".json";
    std::ofstream expectedFile{expectedFilePath};
    Scanner scanner{input, allocator};
    std::pmr::monotonic_buffer_resource allocator{};
    Result<ast::Node<>*> result{Parser::parse(scanner, allocator)};
    if (!result) {
      printFailure(entry.path(), std::format("Unhandled parser error: {}\n",
                                             result.error().message));
      continue;
    }

    if (!result) {
      printFailure(entry.path(), "Expected valid AST, got nullptr");
    }

    ast::Json json((*result)->toJson());
    expectedFile << json.dump(2);
  }
}
