#include "bytecode_generator.hpp"
#include "ast.hpp"
#include "instructions.hpp"
#include "native_functions.hpp"
#include "token.hpp"

Function BytecodeGenerator::generate(const ast::Node &node,
                                     std::pmr::memory_resource &allocator) {
  BytecodeGenerator generator{allocator};
  generator.defineNativeFunctions();
  BytecodeGenerator::Visitor visitor{generator};
  std::visit(visitor, node.data);

  return generator.state().function;
}

RegisterIndex
BytecodeGenerator::Visitor::operator()(const std::monostate &node) {}

RegisterIndex BytecodeGenerator::Visitor::operator()(const ast::Block &node) {
  for (const auto &statement : node.statements) {
    std::visit(*this, statement->data);
  }

  return generator.state().lastRegister();
}

RegisterIndex BytecodeGenerator::Visitor::operator()(const ast::Chunk &node) {
  return std::visit(*this, node.block->data);
}

RegisterIndex
BytecodeGenerator::Visitor::operator()(const ast::LocalDeclaration &node) {}

RegisterIndex
BytecodeGenerator::Visitor::operator()(const ast::Function &node) {}

RegisterIndex
BytecodeGenerator::Visitor::operator()(const ast::Assignment &node) {}

RegisterIndex
BytecodeGenerator::Visitor::operator()(const ast::BinaryOperator &node) {
  const RegisterIndex leftIndex{std::visit(*this, node.operands.first->data)};
  const RegisterIndex rightIndex{std::visit(*this, node.operands.second->data)};
  const RegisterIndex destinationIndex{generator.state().allocateRegister()};

  Operation operation{};
  switch (node.operation) {
  case Token::Type::Plus:
    operation = Operation::Add;
    break;
  case Token::Type::Minus:
    operation = Operation::Subtract;
    break;
  case Token::Type::Times:
    operation = Operation::Multiply;
    break;
  case Token::Type::Divide:
    operation = Operation::Divide;
    break;
  default:
    std::unreachable();
  };

  generator.state().instructionWriter.write(operation, destinationIndex,
                                            leftIndex, rightIndex);

  return destinationIndex;
}

RegisterIndex
BytecodeGenerator::Visitor::operator()(const ast::UnaryOperator &node) {}

RegisterIndex BytecodeGenerator::Visitor::operator()(const ast::Return &node) {}

RegisterIndex BytecodeGenerator::Visitor::operator()(const ast::Break &node) {}

RegisterIndex BytecodeGenerator::Visitor::operator()(const ast::Number &node) {
  const RegisterIndex destinationIndex{generator.state().allocateRegister()};
  generator.state().instructionWriter.write(
      Operation::LoadConstant, destinationIndex,
      generator.state().addConstant(node.value));

  return destinationIndex;
}

RegisterIndex BytecodeGenerator::Visitor::operator()(const ast::Boolean &node) {
}

RegisterIndex BytecodeGenerator::Visitor::operator()(const ast::Name &node) {
  const std::optional<Symbol> symbol{generator.resolve(node.name)};

  if (!symbol->upvalue) {
    return symbol->index;
  }

  const RegisterIndex destinationIndex{generator.state().allocateRegister()};
  generator.state().instructionWriter.write(Operation::LoadUpvalue,
                                            destinationIndex, symbol->index);
  return destinationIndex;
}

RegisterIndex
BytecodeGenerator::Visitor::operator()(const ast::Subscript &node) {}

RegisterIndex BytecodeGenerator::Visitor::operator()(const ast::Access &node) {}

RegisterIndex
BytecodeGenerator::Visitor::operator()(const ast::FunctionCall &node) {
  const RegisterIndex operandIndex{std::visit(*this, node.operand->data)};

  const RegisterIndex firstArgumentIndex{generator.state().nextRegister()};
  for (std::size_t i{}; i < node.arguments.size(); ++i) {
    const RegisterIndex resolvedIndex{
        std::visit(*this, node.arguments[i]->data)};
    const RegisterIndex argumentIndex{firstArgumentIndex + i};

    if (resolvedIndex != argumentIndex) {
      generator.state().instructionWriter.write(Operation::Move, argumentIndex,
                                                resolvedIndex);
    }
  }

  generator.state().instructionWriter.write(Operation::CallFunction,
                                            operandIndex, firstArgumentIndex,
                                            node.arguments.size());

  return firstArgumentIndex;
}

RegisterIndex
BytecodeGenerator::Visitor::operator()(const ast::WhileLoop &node) {}

RegisterIndex
BytecodeGenerator::Visitor::operator()(const ast::RepeatLoop &node) {}

RegisterIndex
BytecodeGenerator::Visitor::operator()(const ast::Conditional &node) {}

RegisterIndex
BytecodeGenerator::Visitor::operator()(const ast::NumericForLoop &node) {}

RegisterIndex
BytecodeGenerator::Visitor::operator()(const ast::GenericForLoop &node) {}

RegisterIndex BytecodeGenerator::Visitor::operator()(const ast::String &node) {
  const RegisterIndex destinationIndex{generator.state().allocateRegister()};
  void *buffer{generator.m_allocator.get().allocate(
      sizeof(StringSize) + node.value.size(), alignof(StringSize))};
  auto *string{static_cast<StringSize *>(buffer)};
  *string = node.value.size();
  std::memcpy(string + 1, node.value.data(), node.value.size());

  generator.state().instructionWriter.write(
      Operation::LoadConstant, destinationIndex,
      generator.state().addConstant(string));

  return destinationIndex;
}

void BytecodeGenerator::defineNativeFunctions() {
  const std::array nativeFunctions{
      std::pair<std::string_view, NativeFunction>{"print", print},
  };

  for (const auto &[name, function] : nativeFunctions) {
    const RegisterIndex constantIndex{global().addConstant(function)};
    const RegisterIndex registerIndex{global().allocateRegister()};
    global().instructionWriter.write(Operation::LoadConstant, registerIndex,
                                     constantIndex);
    global().symbolTable.try_emplace(std::pmr::string{name, &m_allocator.get()},
                                     Symbol{registerIndex, false});
  };
}

std::optional<BytecodeGenerator::Symbol>
BytecodeGenerator::resolve(std::string_view name) {
  auto it{state().symbolTable.find(name)};
  if (it != state().symbolTable.end()) {
    return it->second;
  }

  State *current{&state()};
  State *currentOuter{state().outer};
  std::pmr::string key{name, &m_allocator.get()};
  while (currentOuter) {
    current->symbolTable.insert_or_assign(
        key, Symbol{current->function.upvalues.size(), true});

    it = currentOuter->symbolTable.find(key);
    if (it != currentOuter->symbolTable.end()) {
      current->function.upvalues.emplace_back(it->second.index,
                                              it->second.upvalue);
      return state().symbolTable[key];
    }
    current->function.upvalues.emplace_back(
        currentOuter->function.upvalues.size(), true);

    current = current->outer;
    currentOuter = current->outer;
  }

  return std::nullopt;
}
