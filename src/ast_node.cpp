#include "ast_node.hpp"
#include "nlohmann/json_fwd.hpp"

namespace {
nlohmann::json toJson(const std::vector<AstNode::Ptr> &nodes) {
  std::vector<nlohmann::json> json(nodes.size());
  for (auto i{0uz}; i < nodes.size(); ++i) {
    json[i] = nodes[i]->toJson();
  }
  return json;
}

nlohmann::json toJson(const std::vector<std::string_view> &strings) {
  std::vector<nlohmann::json> json(strings.size());
  for (auto i{0uz}; i < strings.size(); ++i) {
    json[i] = std::string{strings[i]};
  }
  return json;
}

nlohmann::json toJson(const std::pair<AstNode::Ptr, AstNode::Ptr> &nodes) {
  std::vector<nlohmann::json> json{nodes.first->toJson(),
                                   nodes.second->toJson()};
  return json;
}

struct Visitor {
  nlohmann::json &json;
  void operator()(const std::monostate &node) {}
  void operator()(const AstNode::Block &node) {
    json["Block"] = node.chunk->toJson();
  }
  void operator()(const AstNode::Chunk &node) {
    nlohmann::json chunk{};
    chunk["statements"] = toJson(node.statements);
    if (node.lastStatement) {
      chunk["lastStatement"] = node.lastStatement->toJson();
    }
    json["Chunk"] = chunk;
  }
  void operator()(const AstNode::LocalDeclaration &node) {
    json["LocalDeclaration"] = {{"names", toJson(node.names)},
                                {"values", toJson(node.values)}};
  }
  void operator()(const AstNode::Assignment &node) {
    json["Assignment"] = {{"vars", toJson(node.vars)},
                          {"values", toJson(node.values)}};
  }
  void operator()(const AstNode::BinaryOperator &node) {
    json["BinaryOperator"] = {{"operator", node.op.data},
                              {"operands", toJson(node.operands)}};
  }
  void operator()(const AstNode::UnaryOperator &node) {
    json["UnaryOperator"] = {{"operator", node.op.data},
                             {"operand", node.operand->toJson()}};
  }
  void operator()(const AstNode::Return &node) {
    json["Return"] = {{"values", toJson(node.values)}};
  }
  void operator()(const AstNode::Break &node) { json["Break"] = {}; }
  void operator()(const AstNode::Number &node) { json["Number"] = node.value; }
  void operator()(const AstNode::Name &node) {
    json["Name"] = std::string{node.name};
  }
  void operator()(const AstNode::Subscript &node) {
    json["Subscript"] = {{"index", node.index->toJson()},
                         {"operand", node.operand->toJson()}};
  }
  void operator()(const AstNode::Access &node) {
    json["Access"] = {{"member", node.member},
                      {"operand", node.operand->toJson()}};
  }
  void operator()(const AstNode::FunctionCall &node) {
    json["FunctionCall"] = {{"arguments", toJson(node.arguments)},
                            {"operand", node.operand->toJson()}};
  }
};
} // namespace

nlohmann::json AstNode::toJson() const {
  if (std::holds_alternative<std::monostate>(data)) {
    return nullptr;
  }

  nlohmann::json json{};
  buildJson(json);
  return json;
}

void AstNode::buildJson(nlohmann::json &json) const {
  std::visit(Visitor{json}, data);
}
