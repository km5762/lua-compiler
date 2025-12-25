#pragma once

#include <nlohmann/json.hpp>
#include <variant>
#include <vector>

#include "token.hpp"

namespace ast {

struct Node;
template <typename T = Node *> using List = std::pmr::vector<T>;
using Json = nlohmann::ordered_json;

struct Block {
  Node *chunk{};
};
struct Chunk {
  List<> statements{};
  Node *lastStatement{};
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
  std::pair<Node *, Node *> operands{};
};
struct UnaryOperator {
  Token::Type op{};
  Node *operand{};
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
  Node *operand{};
  Node *index{};
};
struct Access {
  Node *operand{};
  std::string_view member{};
};
struct FunctionCall {
  Node *operand{};
  List<> arguments{};
};

using Data = std::variant<std::monostate, Block, Chunk, LocalDeclaration,
                          Assignment, BinaryOperator, UnaryOperator, Return,
                          Break, Number, Name, Subscript, Access, FunctionCall>;

struct Node {
public:
  Data data{};

  Json toJson() const;

private:
  void buildJson(Json &json) const;
};
} // namespace ast
