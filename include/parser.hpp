#pragma once

#include <format>
#include <initializer_list>
#include <memory_resource>

#include "ast_node.hpp"
#include "scanner.hpp"

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
    if (!expectedStr.empty()) expectedStr.resize(expectedStr.size() - 2);

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

  static AstNode *parse(Scanner &scanner, std::pmr::memory_resource &allocator);

 private:
  std::reference_wrapper<Scanner> m_scanner;
  std::reference_wrapper<std::pmr::memory_resource> m_allocator;

  AstNode *makeNode(AstNode::Data &&data);
  template <typename T = AstNode *>
  AstNode::List<T> makeList(std::initializer_list<T> values = {}) {
    return AstNode::List<T>(values, &m_allocator.get());
  }

  template <typename... T>
  bool matches(Token::Type type, T... types) {
    return ((type == types) || ...);
  }
  template <typename... T>
  bool match(T... types) {
    Token token{m_scanner.get().peek()};
    if (matches(token.type, types...)) {
      m_scanner.get().advance();
      return true;
    }
    return false;
  }
  template <typename... T>
  bool check(T... types) {
    return matches(m_scanner.get().peek().type, types...);
  }
  template <typename... T>
  Token consume(T... types) {
    Token token{m_scanner.get().peek()};

    if (!matches(token.type, types...)) {
      std::string expected =
          ((std::string{Token::toString(types)} + ", ") + ...);
      if (!expected.empty()) expected = expected.substr(0, expected.size() - 2);

      throw UnexpectedToken{token, types...};
    }

    return m_scanner.get().advance();
  }
  template <typename... T>
  Token expect(T... types) {
    Token token{m_scanner.get().peek()};

    if (!matches(token.type, types...)) {
      std::string expected =
          ((std::string{Token::toString(types)} + ", ") + ...);
      if (!expected.empty()) expected = expected.substr(0, expected.size() - 2);

      throw UnexpectedToken{token, types...};
    }

    return token;
  }
  bool eof() { return check(Token::Type::Eof); }

  AstNode *parseBlock();
  AstNode *parseChunk();
  AstNode *parseStat();
  AstNode *parseLastStat();
  AstNode::List<std::string_view> parseNameList();
  AstNode::List<> parseVarList(AstNode *first = nullptr);
  AstNode::List<> parseExpList(AstNode *first = nullptr);
  AstNode::List<> parseArguments(AstNode *first = nullptr);
  AstNode *parseExp(int precedence = 0);
  template <auto ParseFunc>
  auto parseList(
      std::optional<std::invoke_result_t<decltype(ParseFunc), Parser *>> first =
          nullptr) {
    using T = std::invoke_result_t<decltype(ParseFunc), Parser *>;

    std::vector<T> list;

    if (first) {
      list.emplace_back(std::move(*first));
    } else {
      list.emplace_back((this->*ParseFunc)());
    }

    while (match(Token::Type::Comma)) {
      list.emplace_back((this->*ParseFunc)());
    }

    return list;
  }

  int getPrecedence(Token::Type type);
  AstNode *parseNud(Token token);
  AstNode *parseNumber(Token token);
  AstNode *parsePrefixExpression(Token token);
  AstNode *parseLed(Token token, AstNode *left);
};
