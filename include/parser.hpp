#pragma once

#include <format>
#include <initializer_list>
#include <memory_resource>

#include "ast.hpp"
#include "error.hpp"
#include "scanner.hpp"
#include "token.hpp"

enum class ParserErrorCode {
  UnexpectedToken,
  UnassignableValue,
  MalformedNumber,
  MissingClosingDelimiter,
  BreakOutsideLoop,
  BadAllocation,
};
class ParserErrorCategory : public std::error_category {
  const char *name() const noexcept override { return "parser"; }

  std::string message(int ev) const override {
    switch (static_cast<ParserErrorCode>(ev)) {
    case ParserErrorCode::UnexpectedToken:
      return "unexpected token";
    case ParserErrorCode::UnassignableValue:
      return "unassignable value";
    case ParserErrorCode::MalformedNumber:
      return "malformed number";
    case ParserErrorCode::MissingClosingDelimiter:
      return "missing closing delimiter";
    case ParserErrorCode::BreakOutsideLoop:
      return "break outside loop";
    default:
      return "unknown parser error";
    }
  }
};
inline const std::error_category &parserCategory() {
  static ParserErrorCategory instance;
  return instance;
}
inline std::error_code make_error_code(ParserErrorCode e) noexcept {
  return {static_cast<int>(e), parserCategory()};
}
namespace std {
template <> struct is_error_code_enum<ParserErrorCode> : true_type {};
} // namespace std

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

template <typename... T>
Error makeUnexpectedTokenError(const Token &got, T... expected) {
  return {ParserErrorCode::UnexpectedToken,
          makeErrorMessage(
              got, std::format("Unexpected token '{}'. Expected one of: {}",
                               got.data, buildMessage(expected...)))};
}

inline Error makeUnassignableValueError(const Token &token,
                                        const ast::Node<> *value) {
  return {
      ParserErrorCode::UnassignableValue,
      makeErrorMessage(token, std::format("Cannot assign to value of type '{}'",
                                          value->name()))};
}

inline Error makeMalformedNumberError(const Token &token) {
  return {ParserErrorCode::MalformedNumber,
          makeErrorMessage(
              token, std::format("Unable to parse number: '{}'", token.data))};
}

template <typename... T>
Error makeMissingClosingDelimiterError(const Token &token, T... expected) {
  return {
      ParserErrorCode::MissingClosingDelimiter,
      makeErrorMessage(
          token, std::format(
                     "Missing closing delimiter for block. Expected one of: {}",
                     buildMessage(expected...)))};
}

inline Error makeBreakOutsideLoopError(const Token &token) {
  return {ParserErrorCode::BreakOutsideLoop,
          makeErrorMessage(token, "Break statement outside of loop")};
}

class Parser {
public:
  Parser(Scanner &scanner, std::pmr::memory_resource &allocator)
      : m_scanner{scanner}, m_allocator{allocator} {};

  static Result<ast::Node<> *> parse(Scanner &scanner,
                                     std::pmr::memory_resource &allocator);

private:
  std::reference_wrapper<Scanner> m_scanner;
  std::reference_wrapper<std::pmr::memory_resource> m_allocator;

  ast::Node<> *makeNode(ast::Data &&data);
  template <typename T = ast::Node<> *>
  ast::List<T> makeList(std::initializer_list<T> values = {}) {
    return ast::List<T>(values, &m_allocator.get());
  }
  template <typename... T> bool matches(Token::Type type, T... types) {
    return ((type == types) || ...);
  }
  template <typename... T> Result<bool> match(T... types) {
    Result<Token> token{m_scanner.get().peek()};
    if (!token) {
      return std::unexpected{token.error()};
    }
    if (matches(token->type, types...)) {
      if (!(token = m_scanner.get().advance())) {
        return std::unexpected{token.error()};
      };
      return true;
    }
    return false;
  }
  template <typename... T> Result<bool> check(T... types) {
    Result<Token> token{m_scanner.get().peek()};
    if (!token) {
      return std::unexpected{token.error()};
    }
    return matches(token->type, types...);
  }
  template <typename... T> Result<Token> consume(T... types) {
    Result<Token> token{m_scanner.get().peek()};
    if (!token) {
      return std::unexpected{token.error()};
    }
    if (!matches(token->type, types...)) {
      return std::unexpected{makeUnexpectedTokenError(*token, types...)};
    }

    return m_scanner.get().advance();
  }
  template <typename... T> bool assignable(ast::Node<> *node) {
    return node->is<ast::Access, ast::Subscript, ast::Name>();
  }

  template <bool inLoop, typename... T>
  Result<ast::Node<> *> parseBlock(T... delimiters) {
    ast::List<> statements{makeList()};

    while (true) {
      Result<bool> delimiter{check(delimiters...)};
      if (!delimiter) {
        return std::unexpected{delimiter.error()};
      }
      if (*delimiter) {
        return makeNode(ast::Block{statements});
      }
      auto eof{check(Token::Type::Eof)};
      if (!eof) {
        return std::unexpected{eof.error()};
      }
      if (*eof) {
        return std::unexpected{makeMissingClosingDelimiterError(
            *m_scanner.get().peek(), delimiters...)};
      }
      Result<ast::Node<> *> statement{parseStatement<inLoop>(delimiters...)};
      if (!statement) {
        return std::unexpected{statement.error()};
      }
      statements.push_back(*statement);
      if (auto token = match(Token::Type::Semicolon); !token) {
        return std::unexpected{token.error()};
      }
    }
  }
  Result<ast::Node<> *> parseChunk();
  template <typename... T> Result<ast::Node<> *> parseReturn(T... delimiters) {
    auto delimiter{check(delimiters...)};
    if (!delimiter) {
      return std::unexpected{delimiter.error()};
    }
    auto semicolon{check(Token::Type::Semicolon)};
    if (!semicolon) {
      return std::unexpected{semicolon.error()};
    }
    if (*delimiter || *semicolon) {
      return makeNode(ast::Return{});
    }
    auto values{parseExpressionList()};
    if (!values) {
      return std::unexpected{values.error()};
    }
    return makeNode(ast::Return{*values});
  }

  template <bool inLoop, typename... T>
  Result<ast::Node<> *> parseStatement(T... delimiters) {
    Result<Token> token{m_scanner.get().advance()};
    if (!token) {
      return std::unexpected{token.error()};
    }

    switch (token->type) {
    case Token::Type::Local:
      return parseLocalDeclaration();
    case Token::Type::While:
      return parseWhileLoop();
    case Token::Type::Repeat:
      return parseRepeatLoop();
    case Token::Type::Return:
      return parseReturn(delimiters...);
    case Token::Type::If:
      return parseConditional<inLoop>();
    case Token::Type::For:
      return parseForLoop();
    case Token::Type::Function:
      return parseFunctionDeclaration();
    case Token::Type::Break:
      if constexpr (!inLoop) {
        return std::unexpected{makeBreakOutsideLoopError(*token)};
      }
      return makeNode(ast::Break{});
    default:
      Result<ast::Node<> *> prefix{parsePrefixExpression(*token)};
      if (!prefix) {
        return std::unexpected{prefix.error()};
      }

      auto assign{match(Token::Type::Assign)};
      if (!assign) {
        return std::unexpected{assign.error()};
      }
      if (*assign) {
        if (!assignable(*prefix)) {
          return std::unexpected{makeUnassignableValueError(*token, *prefix)};
        }
        Result<ast::Node<> *> expression{parseExpression()};
        if (!expression) {
          return std::unexpected{expression.error()};
        }
        prefix = makeNode(ast::Assignment{{*prefix}, {*expression}});
      } else {
        auto comma{match(Token::Type::Comma)};
        if (!comma) {
          return std::unexpected{comma.error()};
        }
        if (!*comma) {
          return prefix;
        }
        if (!assignable(*prefix)) {
          return std::unexpected{makeUnassignableValueError(*token, *prefix)};
        }
        auto variables{parseVariableList(*prefix)};
        if (!variables) {
          return std::unexpected{variables.error()};
        }

        if (auto token = consume(Token::Type::Assign); !token) {
          return std::unexpected{token.error()};
        }

        auto values{parseExpressionList()};
        if (!values) {
          return std::unexpected{values.error()};
        }
        prefix = makeNode(ast::Assignment{*variables, *values});
      }

      return prefix;
    }
  }

  template <bool inLoop> Result<ast::Node<> *> parseConditional() {
    Result<ast::Node<> *> condition{parseExpression()};
    if (!condition) {
      return std::unexpected{condition.error()};
    }

    if (auto token = consume(Token::Type::Then); !token) {
      return std::unexpected{token.error()};
    }
    Result<ast::Node<> *> block{parseBlock<inLoop>(
        Token::Type::ElseIf, Token::Type::Else, Token::Type::End)};
    if (!block) {
      return std::unexpected{block.error()};
    }

    Result<ast::Node<> *> conditional{
        makeNode(ast::Conditional{*condition, *block})};
    if (!conditional) {
      return std::unexpected{condition.error()};
    }
    ast::Node<> *current{*conditional};
    while (true) {
      auto elseif{match(Token::Type::ElseIf)};
      if (!elseif) {
        return std::unexpected{elseif.error()};
      }
      if (!*elseif) {
        break;
      }
      Result<ast::Node<> *> alternateCondition{parseExpression()};
      if (!alternateCondition) {
        return std::unexpected{alternateCondition.error()};
      }
      if (auto token = consume(Token::Type::Then); !token) {
        return std::unexpected{token.error()};
      }
      Result<ast::Node<> *> alternateBlock{
          parseBlock<inLoop>(Token::Type::ElseIf, Token::Type::Else)};
      if (!alternateBlock) {
        return std::unexpected{alternateBlock.error()};
      }
      Result<ast::Node<> *> alternate{
          makeNode(ast::Conditional{*alternateCondition, *alternateBlock})};
      if (!alternate) {
        return std::unexpected{alternate.error()};
      }
      ast::Conditional &currentConditional{
          std::get<ast::Conditional>(current->data)};
      currentConditional.alternate = *alternate;
      current = *alternate;
    }

    auto alternate{match(Token::Type::Else)};
    if (!alternate) {
      return std::unexpected{alternate.error()};
    }
    if (*alternate) {
      Result<ast::Node<> *> alternateBlock{
          parseBlock<inLoop>(Token::Type::End)};
      if (!alternateBlock) {
        return std::unexpected{alternateBlock.error()};
      }

      ast::Conditional &currentConditional{
          std::get<ast::Conditional>(current->data)};
      currentConditional.alternate = *alternateBlock;
    }

    if (auto token = consume(Token::Type::End); !token) {
      return std::unexpected{token.error()};
    }

    return conditional;
  }

  Result<ast::Node<> *> parseWhileLoop();
  Result<ast::Node<> *> parseRepeatLoop();
  Result<ast::Node<> *> parseLocalDeclaration();
  Result<ast::Node<> *> parseForLoop();
  Result<ast::Node<> *> parseFunctionDeclaration();
  Result<ast::Node<> *>
  parseFunction(std::optional<std::string_view> first = std::nullopt);

  Result<ast::List<std::string_view>>
  parseParameters(std::optional<std::string_view> first = std::nullopt);
  Result<ast::List<std::string_view>>
  parseNameList(std::optional<std::string_view> first = std::nullopt);
  Result<ast::List<>> parseVariableList(ast::Node<> *first = nullptr);
  Result<ast::List<>> parseExpressionList(ast::Node<> *first = nullptr);
  Result<ast::List<>> parseArguments(ast::Node<> *first = nullptr);
  Result<ast::Node<> *> parseExpression(int precedence = 0);

  int getPrecedence(Token::Type type);
  Result<ast::Node<> *> parseNud(const Token &token);
  Result<ast::Node<> *> parseNumber(const Token &token);
  Result<ast::Node<> *> parsePrefixExpression(const Token &token);
  Result<ast::Node<> *> parseVariable(const Token &token);
  Result<ast::Node<> *> parseLed(const Token &token, ast::Node<> *left);
};
