#pragma once

#include "ast_node.hpp"
#include "scanner.hpp"

#include <format>
#include <memory_resource>

class ParseError : public std::runtime_error {
public:
  ParseError(Token token, std::string_view message)
      : std::runtime_error{
            std::format("{}:{}: {}", token.line, token.position, message)} {}
};
class UnexpectedToken : public ParseError {
public:
  template <typename... T>
  UnexpectedToken(Token got, T... expected)
      : ParseError{got, buildMessage(got, expected...)} {}

private:
  template <typename... T>
  static std::string buildMessage(Token got, T... expected) {
    std::string expectedStr =
        ((std::string(Token::toString(expected)) + ", ") + ...);
    if (!expectedStr.empty())
      expectedStr.resize(expectedStr.size() - 2);

    return std::format("Unexpected token '{}'. Expected one of: {}", got.data,
                       expectedStr);
  }
};
class MalformedNumber : public ParseError {
public:
  explicit MalformedNumber(Token token)
      : ParseError{token,
                   std::format("Unable to parse number: '{}'", token.data)} {}
};

class Parser {
public:
  Parser(Scanner &scanner, std::pmr::memory_resource &allocator)
      : m_scanner{scanner}, m_allocator{allocator} {};

  static AstNode::Ptr parse(Scanner &scanner,
                            std::pmr::memory_resource &allocator);

private:
  std::reference_wrapper<Scanner> m_scanner;
  std::reference_wrapper<std::pmr::memory_resource> m_allocator;

  template <typename T, typename... Args> T *allocate(Args &&...args) {
    void *buffer{m_allocator.get().allocate(sizeof(T), alignof(T))};
    if (!buffer) {
      return nullptr;
    }

    T *ptr{new (buffer) T{args...}};
    return ptr;
  }

  template <typename... T> bool matches(Token::Type type, T... types) {
    return ((type == types) || ...);
  }
  template <typename... T> bool match(T... types) {
    Token token{m_scanner.get().peek()};
    if (matches(token.type, types...)) {
      m_scanner.get().advance();
      return true;
    }
    return false;
  }
  template <typename... T> bool check(T... types) {
    return matches(m_scanner.get().peek().type, types...);
  }
  template <typename... T> Token consume(T... types) {
    Token token{m_scanner.get().peek()};

    if (!matches(token.type, types...)) {
      std::string expected =
          ((std::string{Token::toString(types)} + ", ") + ...);
      if (!expected.empty())
        expected = expected.substr(0, expected.size() - 2);

      throw UnexpectedToken{token, types...};
    }

    return m_scanner.get().advance();
  }
  template <typename... T> Token expect(T... types) {
    Token token{m_scanner.get().peek()};

    if (!matches(token.type, types...)) {
      std::string expected =
          ((std::string{Token::toString(types)} + ", ") + ...);
      if (!expected.empty())
        expected = expected.substr(0, expected.size() - 2);

      throw UnexpectedToken{token, types...};
    }

    return token;
  }
  bool eof() { return check(Token::Type::Eof); }

  AstNode::Ptr parseBlock();
  AstNode::Ptr parseChunk();
  AstNode::Ptr parseStat();
  AstNode::Ptr parseLastStat();
  std::vector<std::string_view> parseNameList();
  std::string_view parseName() { return consume(Token::Type::Name).data; }
  std::vector<AstNode::Ptr>
  parseVarList(std::optional<AstNode::Ptr> initial = std::nullopt);
  std::vector<AstNode::Ptr>
  parseExpList(std::optional<AstNode::Ptr> initial = std::nullopt);
  std::vector<AstNode::Ptr> parseArguments();
  AstNode::Ptr parseExp();
  template <auto ParseFunc>
  auto
  parseList(std::optional<std::invoke_result_t<decltype(ParseFunc), Parser *>>
                initial = std::nullopt) {
    using T = std::invoke_result_t<decltype(ParseFunc), Parser *>;

    std::vector<T> list;

    if (initial) {
      list.emplace_back(std::move(*initial));
    } else {
      list.emplace_back((this->*ParseFunc)());
    }

    while (match(Token::Type::Comma)) {
      list.emplace_back((this->*ParseFunc)());
    }

    return list;
  }
  template <AstNode::Ptr (Parser::*ParseFunc)(), typename... U>
  AstNode::Ptr parseLeftBinaryOperation(U... types) {
    AstNode::Ptr left{(this->*ParseFunc)()};

    while (check(types...)) {
      Token token{m_scanner.get().advance()};
      AstNode::Ptr right{(this->*ParseFunc)()};
      left = allocate<AstNode>(
          AstNode::BinaryOperator{token, {std::move(left), std::move(right)}});
    }

    return left;
  }
  template <AstNode::Ptr (Parser::*ParseFunc)(), typename... U>
  AstNode::Ptr parseRightBinaryOperation(U... types) {
    AstNode::Ptr left{(this->*ParseFunc)()};

    if (check(types...)) {
      Token token{m_scanner.get().advance()};
      AstNode::Ptr right{parseRightBinaryOperation<ParseFunc>(types...)};
      left = allocate<AstNode>(
          AstNode::BinaryOperator{token, {std::move(left), std::move(right)}});
    }

    return left;
  }
  AstNode::Ptr parseOr();
  AstNode::Ptr parseAnd();
  AstNode::Ptr parseComparison();
  AstNode::Ptr parseConcatenation();
  AstNode::Ptr parseAdditive();
  AstNode::Ptr parseMultiplicative();
  AstNode::Ptr parseUnary();
  AstNode::Ptr parsePower();
  AstNode::Ptr parsePrimary();
  AstNode::Ptr parsePrefixExp();
};
