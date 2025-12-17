#pragma once

#include "scanner.hpp"

#include <format>
#include <functional>
#include <memory>
#include <variant>

struct AstNode {
public:
  using Ptr = std::unique_ptr<AstNode>;
  struct Block {
    Ptr chunk{};
  };
  struct Chunk {
    std::vector<Ptr> statements{};
    Ptr lastStatement{};
  };
  struct LocalDeclaration {
    std::vector<std::string_view> names{};
    std::vector<Ptr> values{};
  };
  struct Assignment {
    std::vector<std::string_view> names{};
    std::vector<Ptr> values{};
  };
  struct BinaryOperator {
    Token op{};
    std::pair<Ptr, Ptr> operands{};
  };
  struct Return {
    std::vector<Ptr> values;
  };
  struct Number {
    double value{};
  };
  struct Name {
    std::string_view name{};
  };
  using Data = std::variant<std::monostate, Block, Chunk, LocalDeclaration,
                            Assignment, BinaryOperator, Return, Number, Name>;

  std::string toString() const {
    struct Visitor {
      std::string operator()(const std::monostate &) const { return "Empty"; }
      std::string operator()(const Block &block) const {
        return "Block(" + (block.chunk ? block.chunk->toString() : "null") +
               ")";
      }
      std::string operator()(const Chunk &chunk) const {
        std::string s = "Chunk[";
        for (const auto &stmt : chunk.statements) {
          s += stmt->toString() + ", ";
        }
        if (chunk.lastStatement)
          s += "Last: " + chunk.lastStatement->toString();
        s += "]";
        return s;
      }
      std::string operator()(const LocalDeclaration &decl) const {
        std::string s = "Local(";
        for (size_t i = 0; i < decl.names.size(); ++i) {
          s += std::string(decl.names[i]);
          if (i < decl.values.size())
            s += "=" + decl.values[i]->toString();
          if (i + 1 < decl.names.size())
            s += ", ";
        }
        s += ")";
        return s;
      }
      std::string operator()(const Assignment &assign) const {
        std::string s = "Assign(";
        for (size_t i = 0; i < assign.names.size(); ++i) {
          s += std::string(assign.names[i]);
          if (i < assign.values.size())
            s += "=" + assign.values[i]->toString();
          if (i + 1 < assign.names.size())
            s += ", ";
        }
        s += ")";
        return s;
      }
      std::string operator()(const BinaryOperator &bin) const {
        return "Binary(" + bin.operands.first->toString() + " " +
               std::string(bin.op.data) + " " +
               bin.operands.second->toString() + ")";
      }
      std::string operator()(const Return &ret) const {
        std::string s = "Return(";
        for (size_t i = 0; i < ret.values.size(); ++i) {
          s += ret.values[i]->toString();
          if (i + 1 < ret.values.size())
            s += ", ";
        }
        s += ")";
        return s;
      }
      std::string operator()(const Number &num) const {
        return "Number(" + std::to_string(num.value) + ")";
      }
      std::string operator()(const Name &name) const {
        return "Name(" + std::string(name.name) + ")";
      }
    };

    return std::visit(Visitor{}, data);
  }

  Data data{};
};

class UnexpectedToken : public std::runtime_error {
public:
  template <typename... T>
  UnexpectedToken(Token::Type got, T... expected)
      : std::runtime_error{buildMessage(got, expected...)} {}

private:
  template <typename... T>
  static std::string buildMessage(Token::Type got, T... expected) {
    std::string expectedStr =
        ((std::string(Token::toString(expected)) + ", ") + ...);
    if (!expectedStr.empty())
      expectedStr.resize(expectedStr.size() - 2);

    return std::format("Unexpected token: {}. Expected one of: {}",
                       Token::toString(got), expectedStr);
  }
};
class MalformedNumber : public std::runtime_error {
public:
  explicit MalformedNumber(std::string_view number)
      : std::runtime_error{std::format("Unable to parse number: {}", number)} {}
};

class Parser {
public:
  Parser(Scanner &scanner) : m_scanner{scanner} {};

  static AstNode::Ptr parse(Scanner &scanner);

private:
  std::reference_wrapper<Scanner> m_scanner;

  template <typename... T> bool match(Token::Type type, T... types) {
    return ((type == types) || ...);
  }
  template <typename... T> bool match(T... types) {
    return match(m_scanner.get().advance().type, types...);
  }
  template <typename... T> bool check(T... types) {
    return match(m_scanner.get().peek().type, types...);
  }
  template <typename... T> Token consume(T... types) {
    Token token{m_scanner.get().peek()};

    if (!match(token.type, types...)) {
      std::string expected =
          ((std::string{Token::toString(types)} + ", ") + ...);
      if (!expected.empty())
        expected = expected.substr(0, expected.size() - 2);

      throw UnexpectedToken{token.type, types...};
    }

    return m_scanner.get().advance();
  }
  template <typename... T> Token expect(T... types) {
    Token token{m_scanner.get().peek()};

    if (!match(token.type, types...)) {
      std::string expected =
          ((std::string{Token::toString(types)} + ", ") + ...);
      if (!expected.empty())
        expected = expected.substr(0, expected.size() - 2);

      throw UnexpectedToken{token.type, types...};
    }

    return token;
  }
  bool eof() { return check(Token::Type::Eof); }

  AstNode::Ptr parseBlock();
  AstNode::Ptr parseChunk();
  AstNode::Ptr parseStat();
  AstNode::Ptr parseLastStat();
  AstNode::Ptr parseVar();
  std::vector<std::string_view> parseNameList();
  std::vector<AstNode::Ptr> parseExpList();
  AstNode::Ptr parseExp();
  AstNode::Ptr parseAdditive();
  AstNode::Ptr parsePrimary();
};
