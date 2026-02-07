#pragma once

#include <nlohmann/json.hpp>
#include <variant>
#include <vector>

#include "token.hpp"

namespace ast {
template <typename T> struct NodeName;
#define DECLARE_NODE_NAME(type)                                                \
  struct type;                                                                 \
  template <> struct NodeName<type> {                                          \
    static constexpr std::string_view value = #type;                           \
  }

DECLARE_NODE_NAME(Block);
DECLARE_NODE_NAME(Chunk);
DECLARE_NODE_NAME(LocalDeclaration);
DECLARE_NODE_NAME(Function);
DECLARE_NODE_NAME(Assignment);
DECLARE_NODE_NAME(BinaryOperator);
DECLARE_NODE_NAME(UnaryOperator);
DECLARE_NODE_NAME(Return);
DECLARE_NODE_NAME(Break);
DECLARE_NODE_NAME(Number);
DECLARE_NODE_NAME(Boolean);
DECLARE_NODE_NAME(Name);
DECLARE_NODE_NAME(Subscript);
DECLARE_NODE_NAME(Access);
DECLARE_NODE_NAME(FunctionCall);
DECLARE_NODE_NAME(WhileLoop);
DECLARE_NODE_NAME(RepeatLoop);
DECLARE_NODE_NAME(NumericForLoop);
DECLARE_NODE_NAME(GenericForLoop);
DECLARE_NODE_NAME(Conditional);
DECLARE_NODE_NAME(String);
DECLARE_NODE_NAME(Nil);

#undef DECLARE_NODE_NAME

using Data = std::variant<std::monostate, Block, Chunk, LocalDeclaration,
                          Function, Assignment, BinaryOperator, UnaryOperator,
                          Return, Break, Number, Boolean, Name, Subscript,
                          Access, FunctionCall, WhileLoop, NumericForLoop,
                          GenericForLoop, RepeatLoop, Conditional, String, Nil>;
using LValue = std::variant<Name, Access, Subscript>;

using Json = nlohmann::ordered_json;
struct JsonVisitor {
  Json &json;
  void operator()(const std::monostate &node);
  void operator()(const Block &node);
  void operator()(const Chunk &node);
  void operator()(const LocalDeclaration &node);
  void operator()(const Function &node);
  void operator()(const Assignment &node);
  void operator()(const BinaryOperator &node);
  void operator()(const UnaryOperator &node);
  void operator()(const Return &node);
  void operator()(const Break &node);
  void operator()(const Number &node);
  void operator()(const Boolean &node);
  void operator()(const Name &node);
  void operator()(const Subscript &node);
  void operator()(const Access &node);
  void operator()(const FunctionCall &node);
  void operator()(const WhileLoop &node);
  void operator()(const RepeatLoop &node);
  void operator()(const Conditional &node);
  void operator()(const NumericForLoop &node);
  void operator()(const GenericForLoop &node);
  void operator()(const String &node);
  void operator()(const Nil &node);
};

template <typename V = Data> struct Node {
public:
  V data{};

  Json toJson() const {
    if (std::holds_alternative<std::monostate>(data)) {
      return nullptr;
    }

    Json json{};
    buildJson(json);
    return json;
  }
  template <typename... T> bool is() const {
    return (std::holds_alternative<T>(data) || ...);
  }

  std::string_view name() const {
    return std::visit(
        [](auto const &value) -> std::string_view {
          using T = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<T, std::monostate>)
            return "Empty";
          else
            return NodeName<T>::value;
        },
        data);
  }

private:
  void buildJson(Json &json) const { std::visit(JsonVisitor{json}, data); }
};

template <typename T = Node<> *> using List = std::pmr::vector<T>;
using Json = nlohmann::ordered_json;

struct Name {
  std::string_view name{};
};
struct Subscript {
  Node<> *operand{};
  Node<> *index{};
};
struct Access {
  Node<> *operand{};
  std::string_view member{};
};

using LValue = std::variant<Name, Access, Subscript>;

struct Block {
  List<> statements{};
};
struct Chunk {
  Node<> *block{};
};
struct LocalDeclaration {
  List<std::string_view> names{};
  List<> values{};
};
struct Function {
  List<std::string_view> parameters{};
  Node<> *block{};
};
struct Assignment {
  List<Node<LValue>> variables{};
  List<> values{};
};
struct BinaryOperator {
  Token::Type operation{};
  std::pair<Node<> *, Node<> *> operands{};
};
struct UnaryOperator {
  Token::Type operation{};
  Node<> *operand{};
};
struct Return {
  List<> values;
};
struct Break {};
struct Number {
  double value{};
};
struct Boolean {
  bool value{};
};
struct FunctionCall {
  Node<> *operand{};
  List<> arguments{};
};
struct WhileLoop {
  Node<> *condition{};
  Node<> *block{};
};
struct RepeatLoop {
  Node<> *condition{};
  Node<> *block{};
};
struct NumericForLoop {
  Node<> *declaration{};
  Node<> *end{};
  Node<> *increment{};
  Node<> *block{};
};
struct GenericForLoop {
  List<std::string_view> names{};
  List<> values{};
  Node<> *block{};
};
struct Conditional {
  Node<> *condition{};
  Node<> *block{};
  Node<> *alternate{};
};
struct String {
  std::string_view value{};
};
struct Nil {};

} // namespace ast
