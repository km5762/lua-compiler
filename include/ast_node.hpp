#pragma once

#include <nlohmann/json.hpp>
#include <variant>
#include <vector>

#include "token.hpp"

struct AstNode {
 public:
  template <typename T = AstNode *>
  using List = std::pmr::vector<T>;

  struct Block {
    AstNode *chunk{};
  };
  struct Chunk {
    List<> statements{};
    AstNode *lastStatement{};
  };
  struct LocalDeclaration {
    List<std::string_view> names{};
    List<> values{};
  };
  struct Assignment {
    List<> vars{};
    List<> values{};
  };
  struct BinaryOperator {
    Token::Type op{};
    std::pair<AstNode *, AstNode *> operands{};
  };
  struct UnaryOperator {
    Token::Type op{};
    AstNode *operand{};
  };
  struct Return {
    List<> values;
  };
  struct Break {};
  struct Number {
    double value{};
  };
  struct Name {
    std::string_view name{};
  };
  struct Subscript {
    AstNode *operand{};
    AstNode *index{};
  };
  struct Access {
    AstNode *operand{};
    std::string_view member{};
  };
  struct FunctionCall {
    AstNode *operand{};
    List<> arguments{};
  };

  using Data =
      std::variant<std::monostate, Block, Chunk, LocalDeclaration, Assignment,
                   BinaryOperator, UnaryOperator, Return, Break, Number, Name,
                   Subscript, Access, FunctionCall>;

  nlohmann::json toJson() const;
  Data data{};

 private:
  void buildJson(nlohmann::json &json) const;
};
