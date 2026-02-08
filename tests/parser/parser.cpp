#include "parser.hpp"
#include "ast.hpp"
#include "token.hpp"

#include <cstdlib>
#include <format>
#include <fstream>
#include <iostream>
#include <memory_resource>
#include <string_view>

namespace {
std::pmr::monotonic_buffer_resource allocator{};
}

void printFailure(const std::filesystem::path &path, std::string_view message) {
  std::cerr << path.filename().stem() << ": " << message << '\n';
}

std::string dumpTokens(std::string_view text) {
  Scanner scanner{text, allocator};
  Token token{*scanner.advance()};
  std::string dump{};

  while (token.type != Token::Type::Eof) {
    dump += std::string{Token::toString(token.type)} + ", ";
    token = *scanner.advance();
  }

  return dump;
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
    if (!inputFile.is_open()) {
      printFailure(entry.path(), "Could not open input file for reading");
    }
    std::string input{(std::istreambuf_iterator<char>(inputFile)),
                      std::istreambuf_iterator<char>()};
    std::filesystem::path expectedFilePath{
        sourceDir / std::filesystem::path{"expected"} / entry.path().stem()};
    expectedFilePath += ".json";
    std::ifstream expectedFile{expectedFilePath};
    if (!expectedFile.is_open()) {
      printFailure(entry.path(), "Could not open expected file for reading");
    }

    std::string expected{(std::istreambuf_iterator<char>(expectedFile)),
                         std::istreambuf_iterator<char>()};

    Scanner scanner{input, allocator};
    Result<ast::Node *> result{Parser::parse(scanner, allocator)};
    if (!result) {
      printFailure(entry.path(),
                   std::format("Unhandled parser error: {}\nToken dump:{}\n\n",
                               result.error().message, dumpTokens(input)));
      exitCode = EXIT_FAILURE;
      continue;
    }

    if (!*result) {
      printFailure(entry.path(), "Expected valid AST, got nullptr");
      exitCode = EXIT_FAILURE;
    }

    ast::Json json((*result)->toJson());
    ast::Json expectedJson(ast::Json::parse(expected));
    if (json != expectedJson) {
      ast::Json diff(ast::Json::diff(expectedJson, json));
      printFailure(
          entry.path(),
          std::format("Unexpected AST dump with diff:{}\n", diff.dump(2)));
      exitCode = EXIT_FAILURE;
    }
  }

  std::filesystem::path failurePath = sourceDir / "failure";
  for (const auto &entry : std::filesystem::directory_iterator{failurePath}) {
    if (!entry.is_regular_file()) {
      continue;
    }
    std::ifstream inputFile{entry.path()};
    std::string input{(std::istreambuf_iterator<char>(inputFile)),
                      std::istreambuf_iterator<char>()};
    Scanner scanner{input, allocator};
    Result<ast::Node *> result{Parser::parse(scanner, allocator)};
    if (result) {
      std::string dump{*result ? (*result)->toJson().dump(2) : ""};
      printFailure(entry.path(),
                   std::format("Expected failure, got AST:\n{}\n", dump));
      exitCode = EXIT_FAILURE;
    }
  }
  exit(exitCode);
}
