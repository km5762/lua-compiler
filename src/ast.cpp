#include "ast.hpp"

#include "token.hpp"

namespace ast {
namespace {
Json toJson(const List<> &nodes) {
  std::vector<Json> json(nodes.size());
  for (auto i{0uz}; i < nodes.size(); ++i) {
    json[i] = nodes[i]->toJson();
  }
  return json;
}

Json toJson(const List<std::string_view> &strings) {
  std::vector<Json> json(strings.size());
  for (auto i{0uz}; i < strings.size(); ++i) {
    json[i] = std::string{strings[i]};
  }
  return json;
}

Json toJson(const std::pair<Node *, Node *> &nodes) {
  std::vector<Json> json{nodes.first->toJson(), nodes.second->toJson()};
  return json;
}

struct Visitor {
  Json &json;
  void operator()(const std::monostate &node) {}
  void operator()(const Block &node) { json["Block"] = node.chunk->toJson(); }
  void operator()(const Chunk &node) {
    Json chunk{};
    chunk["statements"] = toJson(node.statements);
    if (node.lastStatement) {
      chunk["lastStatement"] = node.lastStatement->toJson();
    }
    json["Chunk"] = chunk;
  }
  void operator()(const LocalDeclaration &node) {
    json["LocalDeclaration"] = {{"names", toJson(node.names)},
                                {"values", toJson(node.values)}};
  }
  void operator()(const Assignment &node) {
    json["Assignment"] = {{"vars", toJson(node.vars)},
                          {"values", toJson(node.values)}};
  }
  void operator()(const BinaryOperator &node) {
    json["BinaryOperator"] = {{"operator", Token::toString(node.op)},
                              {"operands", toJson(node.operands)}};
  }
  void operator()(const UnaryOperator &node) {
    json["UnaryOperator"] = {{"operator", Token::toString(node.op)},
                             {"operand", node.operand->toJson()}};
  }
  void operator()(const Return &node) {
    json["Return"] = {{"values", toJson(node.values)}};
  }
  void operator()(const Break &node) { json["Break"] = {}; }
  void operator()(const Number &node) { json["Number"] = node.value; }
  void operator()(const Name &node) { json["Name"] = std::string{node.name}; }
  void operator()(const Subscript &node) {
    json["Subscript"] = {{"index", node.index->toJson()},
                         {"operand", node.operand->toJson()}};
  }
  void operator()(const Access &node) {
    json["Access"] = {{"member", node.member},
                      {"operand", node.operand->toJson()}};
  }
  void operator()(const FunctionCall &node) {
    json["FunctionCall"] = {{"arguments", toJson(node.arguments)},
                            {"operand", node.operand->toJson()}};
  }
};
} // namespace

Json Node::toJson() const {
  if (std::holds_alternative<std::monostate>(data)) {
    return nullptr;
  }

  Json json{};
  buildJson(json);
  return json;
}

void Node::buildJson(Json &json) const { std::visit(Visitor{json}, data); }

} // namespace ast
