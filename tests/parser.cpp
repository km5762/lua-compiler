#include "parser.hpp"
#include "ast.hpp"

#include <algorithm>
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
  int exitCode{EXIT_SUCCESS};
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
    std::ifstream expectedFile{expectedFilePath};
    std::string expected{(std::istreambuf_iterator<char>(expectedFile)),
                         std::istreambuf_iterator<char>()};
    std::string stripped{expected};
    stripped.erase(std::remove_if(stripped.begin(), stripped.end(), ::isspace),
                   stripped.end());

    Scanner scanner{input};
    std::pmr::monotonic_buffer_resource allocator{};
    ast::Node *ast{};
    try {
      ast = Parser::parse(scanner, allocator);
    } catch (const ParseError &error) {
      std::string astJson{ast ? ast->toJson().dump(2) : ""};
      printFailure(inputPath,
                   std::format("Unhandled parser error: {}", error.what()));
      exitCode = EXIT_FAILURE;
    }

    if (!ast) {
      printFailure(entry.path(), "Expected valid AST, got nullptr");
      exitCode = EXIT_FAILURE;
    }

    ast::Json json(ast->toJson());
    ast::Json expectedJson(ast::Json::parse(expected));
    if (json != expectedJson) {
      printFailure(
          entry.path(),
          std::format("Unexpected AST dump.\nExpected:\n{}\nGot:\n{}\n",
                      expectedJson.dump(2), json.dump(2)));
      exitCode = EXIT_FAILURE;
    }
  }

  exit(exitCode);
}
