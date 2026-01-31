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
  void operator()(const Block &node) {
    Json block{};
    block["statements"] = toJson(node.statements);
    json[NodeName<ast::Block>::value] = block;
  }
  void operator()(const Chunk &node) {
    json[NodeName<Chunk>::value] = node.block->toJson();
  }
  void operator()(const LocalDeclaration &node) {
    json[NodeName<LocalDeclaration>::value] = {{"names", toJson(node.names)},
                                               {"values", toJson(node.values)}};
  }
  void operator()(const Function &node) {
    json[NodeName<Function>::value] = {{"parameters", toJson(node.parameters)},
                                       {"block", node.block->toJson()}};
  }
  void operator()(const Assignment &node) {
    json[NodeName<Assignment>::value] = {{"variables", toJson(node.variables)},
                                         {"values", toJson(node.values)}};
  }
  void operator()(const BinaryOperator &node) {
    json[NodeName<BinaryOperator>::value] = {
        {"operands", toJson(node.operands)},
        {"operator", Token::toString(node.operation)},
    };
  }
  void operator()(const UnaryOperator &node) {
    json[NodeName<UnaryOperator>::value] = {
        {"operator", Token::toString(node.operation)},
        {"operand", node.operand->toJson()}};
  }
  void operator()(const Return &node) {
    json[NodeName<Return>::value] = {{"values", toJson(node.values)}};
  }
  void operator()(const Break &node) { json[NodeName<Break>::value] = {}; }
  void operator()(const Number &node) {
    json[NodeName<Number>::value] = node.value;
  }
  void operator()(const Boolean &node) {
    json[NodeName<Boolean>::value] = node.value;
  }
  void operator()(const Name &node) {
    json[NodeName<Name>::value] = std::string{node.name};
  }
  void operator()(const Subscript &node) {
    json[NodeName<Subscript>::value] = {
        {"operand", node.operand->toJson()},
        {"index", node.index->toJson()},
    };
  }
  void operator()(const Access &node) {
    json[NodeName<Access>::value] = {
        {"operand", node.operand->toJson()},
        {"member", node.member},
    };
  }
  void operator()(const FunctionCall &node) {
    json[NodeName<FunctionCall>::value] = {
        {"operand", node.operand->toJson()},
        {"arguments", toJson(node.arguments)}};
  }
  void operator()(const WhileLoop &node) {
    json[NodeName<WhileLoop>::value] = {{"condition", node.condition->toJson()},
                                        {"block", node.block->toJson()}};
  }
  void operator()(const RepeatLoop &node) {
    json[NodeName<RepeatLoop>::value] = {
        {"condition", node.condition->toJson()},
        {"block", node.block->toJson()}};
  }
  void operator()(const Conditional &node) {
    json[NodeName<Conditional>::value] = {
        {"condition", node.condition->toJson()},
        {"block", node.block->toJson()},
        {"alternate", node.alternate->toJson()}};
  }
  void operator()(const NumericForLoop &node) {
    json[NodeName<NumericForLoop>::value] = {
        {"declaration", node.declaration->toJson()},
        {"end", node.end->toJson()},
        {"increment", node.increment->toJson()},
        {"block", node.block->toJson()}};
  }
  void operator()(const GenericForLoop &node) {
    json[NodeName<GenericForLoop>::value] = {{"names", toJson(node.names)},
                                             {"values", toJson(node.values)},
                                             {"block", node.block->toJson()}};
  }
  void operator()(const String &node) {
    json[NodeName<String>::value] = {node.value};
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
