#pragma once

#include <format>
#include <initializer_list>
#include <memory_resource>

#include "ast.hpp"
#include "scanner.hpp"
#include "token.hpp"

namespace {
template <typename... T> std::string buildMessage(T... types) {
  std::string typesString =
      ((std::string(Token::toString(types)) + ", ") + ...);
  if (!typesString.empty())
    typesString.resize(typesString.size() - 2);

  return typesString;
}

template <typename... T> std::string buildNodesString() {
  std::string nodesString;
  ((nodesString += std::string(ast::NodeName<T>::value) + ", "), ...);

  if (!nodesString.empty())
    nodesString.resize(nodesString.size() - 2);

  return nodesString;
}
} // namespace

class ParseError : public std::runtime_error {
public:
  ast::Node *ast{};

  ParseError(Token token, std::string_view message)
      : std::runtime_error{
            std::format("{}:{}: {}", token.line, token.position, message)} {}

  ParseError(const std::runtime_error &error, ast::Node *ast = nullptr)
      : std::runtime_error{error}, ast{ast} {}
};

class UnexpectedToken : public ParseError {
public:
  template <typename... T>
  UnexpectedToken(Token got, T... expected)
      : ParseError{got,
                   std::format("Unexpected token '{}'. Expected one of: {}",
                               got.data, buildMessage(expected...))} {}
};

template <typename... T> class UnexpectedNode : public ParseError {
public:
  explicit UnexpectedNode(ast::Node *got, Token token)
      : ParseError{token,
                   std::format("Unexpected node '{}'. Expected one of: {}",
                               got->name(), buildNodesString<T...>())} {}
};

class MalformedNumber : public ParseError {
public:
  explicit MalformedNumber(Token token)
      : ParseError{token,
                   std::format("Unable to parse number: '{}'", token.data)} {}
};

template <typename... T> class MissingClosingDelimiter : public ParseError {
public:
  explicit MissingClosingDelimiter(Token token, T... expected)
      : ParseError{
            token,
            std::format(
                "Missing closing delimiter for block. Expected one of: {}",
                buildMessage(expected...))} {}
};

template <typename... T> class BreakOutsideLoop : public ParseError {
public:
  explicit BreakOutsideLoop(Token token)
      : ParseError{token, "Break statement outside of loop"} {}
};

class Parser {
public:
  Parser(Scanner &scanner, std::pmr::memory_resource &allocator)
      : m_scanner{scanner}, m_allocator{allocator} {};

  static ast::Node *parse(Scanner &scanner,
                          std::pmr::memory_resource &allocator);

private:
  std::reference_wrapper<Scanner> m_scanner;
  std::reference_wrapper<std::pmr::memory_resource> m_allocator;

  ast::Node *makeNode(ast::Data &&data);
  template <typename T = ast::Node *>
  ast::List<T> makeList(std::initializer_list<T> values = {}) {
    return ast::List<T>(values, &m_allocator.get());
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
      throw UnexpectedToken{token, types...};
    }

    return m_scanner.get().advance();
  }
  template <typename... T> void expect(ast::Node *node, Token token) {
    if (!node->is<T...>()) {
      throw UnexpectedNode<T...>(node, token);
    }
  }

  bool eof() { return check(Token::Type::Eof); }

  template <bool inLoop, typename... T> ast::Node *parseBlock(T... delimiters) {
    ast::List<> statements{makeList()};

    while (true) {
      if (check(delimiters...)) {
        return makeNode(ast::Block{statements});
      }
      if (check(Token::Type::Eof)) {
        throw MissingClosingDelimiter{m_scanner.get().peek(), delimiters...};
      }
      try {
        statements.push_back(parseStatement<inLoop>(delimiters...));
        match(Token::Type::Semicolon);
      } catch (const ParseError &error) {
        throw ParseError{error, makeNode(ast::Block{statements})};
      }
    }
  }
  ast::Node *parseChunk();
  template <typename... T> ast::Node *parseReturn(T... delimiters) {
    if (check(delimiters...) || check(Token::Type::Semicolon)) {
      return makeNode(ast::Return{});
    }
    return makeNode(ast::Return{parseExpressionList()});
  }

  template <bool inLoop, typename... T>
  ast::Node *parseStatement(T... delimiters) {
    Token token{m_scanner.get().advance()};
    switch (token.type) {
    case Token::Type::Local: {
      ast::List<std::string_view> nameList{parseNameList()};
      ast::List<> expressionList{};

      if (match(Token::Type::Assign)) {
        expressionList = parseExpressionList();
      }

      return makeNode(ast::LocalDeclaration{nameList, expressionList});
    }
    case Token::Type::While: {
      ast::Node *condition{parseExpression()};
      consume(Token::Type::Do);
      ast::Node *block{parseBlock<true>(Token::Type::End)};
      consume(Token::Type::End);
      return makeNode(ast::WhileLoop{condition, block});
    }
    case Token::Type::Repeat: {
      ast::Node *block{parseBlock<true>(Token::Type::Until)};
      consume(Token::Type::Until);
      ast::Node *condition{parseExpression()};
      return makeNode(ast::RepeatLoop{condition, block});
    }
    case Token::Type::Return:
      return parseReturn(delimiters...);
    case Token::Type::If: {
      return parseConditional<inLoop>();
    }
    case Token::Type::Break:
      if constexpr (!inLoop) {
        throw BreakOutsideLoop(token);
      }
      return makeNode(ast::Break{});
    default: {
      ast::Node *prefix{parsePrefixExpression(token)};

      if (match(Token::Type::Assign)) {
        expect<ast::Access, ast::Subscript, ast::Name>(prefix, token);
        prefix = makeNode(ast::Assignment{{prefix}, {parseExpression()}});
      } else if (match(Token::Type::Comma)) {
        expect<ast::Access, ast::Subscript, ast::Name>(prefix, token);
        ast::List<> variables{parseVariableList(prefix)};

        consume(Token::Type::Assign);
        prefix = makeNode(ast::Assignment{variables, parseExpressionList()});
      }

      return prefix;
    }
    }
  }

  template <bool inLoop> ast::Node *parseConditional() {
    ast::Node *condition{parseExpression()};
    consume(Token::Type::Then);
    ast::Node *block{parseBlock<inLoop>(Token::Type::ElseIf, Token::Type::Else,
                                        Token::Type::End)};

    ast::Node *conditional{makeNode(ast::Conditional{condition, block})};
    ast::Node *current{conditional};
    while (match(Token::Type::ElseIf)) {
      ast::Node *alternateCondition{parseExpression()};
      consume(Token::Type::Then);
      ast::Node *alternateBlock{
          parseBlock<inLoop>(Token::Type::ElseIf, Token::Type::Else)};
      ast::Node *alternate{
          makeNode(ast::Conditional{alternateCondition, alternateBlock})};
      ast::Conditional &currentConditional{
          std::get<ast::Conditional>(current->data)};
      currentConditional.alternate = alternate;
      current = alternate;
    }

    if (match(Token::Type::Else)) {
      ast::Node *alternateBlock{parseBlock<inLoop>(Token::Type::End)};
      ast::Conditional &currentConditional{
          std::get<ast::Conditional>(current->data)};
      currentConditional.alternate = alternateBlock;
    }

    consume(Token::Type::End);

    return conditional;
  }

  ast::List<std::string_view> parseNameList();
  ast::List<> parseVariableList(ast::Node *first = nullptr);
  ast::List<> parseExpressionList(ast::Node *first = nullptr);
  ast::List<> parseArguments(ast::Node *first = nullptr);
  ast::Node *parseExpression(int precedence = 0);

  int getPrecedence(Token::Type type);
  ast::Node *parseNud(Token token);
  ast::Node *parseNumber(Token token);
  ast::Node *parsePrefixExpression(Token token);
  ast::Node *parseVariable(Token token);
  ast::Node *parseLed(Token token, ast::Node *left);
};
