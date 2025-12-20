#pragma once

#include "token.hpp"

#include <nlohmann/json.hpp>

#include <variant>
#include <vector>

struct AstNode {
public:
  using Ptr = AstNode *;
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
    std::vector<Ptr> vars{};
    std::vector<Ptr> values{};
  };
  struct BinaryOperator {
    Token op{};
    std::pair<Ptr, Ptr> operands{};
  };
  struct UnaryOperator {
    Token op{};
    Ptr operand{};
  };
  struct Return {
    std::vector<Ptr> values;
  };
  struct Break {};
  struct Number {
    double value{};
  };
  struct Name {
    std::string_view name{};
  };
  struct Subscript {
    Ptr operand{};
    Ptr index{};
  };
  struct Access {
    Ptr operand{};
    std::string_view member{};
  };
  struct FunctionCall {
    Ptr operand{};
    std::vector<Ptr> arguments{};
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
