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
    Scanner scanner{input};
    std::pmr::monotonic_buffer_resource allocator{};
    ast::Node *ast{};
    try {
      ast = Parser::parse(scanner, allocator);
    } catch (const ParseError &error) {
      std::string dump{error.ast->toJson().dump(2)};
      printFailure(entry.path(),
                   std::format("Unhandled parser error: {}\nPartial AST:\n{}\n",
                               error.what(), dump));
      continue;
    }

    if (!ast) {
      printFailure(entry.path(), "Expected valid AST, got nullptr");
    }

    ast::Json json(ast->toJson());
    expectedFile << json.dump(2);
  }
}
