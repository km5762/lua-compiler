#include "bytecode_generator.hpp"
#include "ast.hpp"
#include "instructions.hpp"
#include "native_functions.hpp"
#include "token.hpp"
#include <utility>

Function
BytecodeGenerator::generate(const ast::Node &node,
                            std::pmr::memory_resource &compilerAllocator,
                            std::pmr::memory_resource &runtimeAllocator) {
  BytecodeGenerator generator{compilerAllocator, runtimeAllocator};
  generator.defineNativeFunctions();
  BytecodeGenerator::Visitor visitor{generator};
  std::visit(visitor, node.data);

  return generator.state().function;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const std::monostate &node) {}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Block &node) {
  for (const auto &statement : node.statements) {
    std::visit(*this, statement->data);
  }

  return generator.state().lastRegister();
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Chunk &node) {
  return std::visit(*this, node.block->data);
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::LocalDeclaration &node) {}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Function &node) {}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Assignment &node) {
  for (std::size_t i{}; i < node.values.size(); ++i) {
    Result<RegisterIndex> variableIndex{
        std::visit(*this, node.variables[i]->data)};
    const Result<RegisterIndex> valueIndex{
        std::visit(*this, node.values[i]->data)};
    if (!valueIndex) {
      return std::unexpected{valueIndex.error()};
    }

    if (!variableIndex) {
      if (variableIndex.error().code !=
          BytecodeGeneratorErrorCode::UnresolvedSymbol) {
        return std::unexpected{variableIndex.error()};
      }

      const auto &name{std::get<ast::Name>(node.variables[i]->data)};
      const std::pmr::string key{name.name};
      variableIndex = generator.state().allocateRegister();
      generator.global().symbolTable.try_emplace(key, Symbol{*variableIndex});
    }

    generator.state().instructionWriter.write(Operation::Move, *variableIndex,
                                              *valueIndex);
  }

  return {};
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::BinaryOperator &node) {
  const Result<RegisterIndex> leftIndex{
      std::visit(*this, node.operands.first->data)};
  if (!leftIndex) {
    return std::unexpected{leftIndex.error()};
  }
  const Result<RegisterIndex> rightIndex{
      std::visit(*this, node.operands.second->data)};
  if (!rightIndex) {
    return std::unexpected{rightIndex.error()};
  }
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
  case Token::Type::Modulo:
    operation = Operation::Modulo;
    break;
  case Token::Type::Power:
    operation = Operation::Power;
    break;
  case Token::Type::Equal:
    operation = Operation::Equal;
    break;
  case Token::Type::NotEqual:
    operation = Operation::NotEqual;
    break;
  case Token::Type::LessThan:
    operation = Operation::LessThan;
    break;
  case Token::Type::LessThanOrEqual:
    operation = Operation::LessThanOrEqual;
    break;
  case Token::Type::GreaterThan:
    operation = Operation::GreaterThan;
    break;
  case Token::Type::GreaterThanOrEqual:
    operation = Operation::GreaterThanOrEqual;
    break;
  default:
    std::unreachable();
  };

  generator.state().instructionWriter.write(operation, destinationIndex,
                                            *leftIndex, *rightIndex);
  return destinationIndex;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::UnaryOperator &node) {
  const RegisterIndex destinationIndex{generator.state().allocateRegister()};
  const Result<RegisterIndex> operandIndex{
      std::visit(*this, node.operand->data)};
  if (!operandIndex) {
    return std::unexpected{operandIndex.error()};
  }

  Operation operation{};
  switch (node.operation) {
  case Token::Type::Minus:
    operation = Operation::Minus;
    break;
  default:
    std::unreachable();
  }

  generator.state().instructionWriter.write(operation, destinationIndex,
                                            *operandIndex);
  return destinationIndex;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Return &node) {}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Break &node) {}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Number &node) {
  const RegisterIndex destinationIndex{generator.state().allocateRegister()};
  generator.state().instructionWriter.write(
      Operation::LoadConstant, destinationIndex,
      generator.state().addConstant(node.value));

  return destinationIndex;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Boolean &node) {}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Name &node) {
  const std::optional<Symbol> symbol{generator.resolve(node.name)};

  if (!symbol) {
    return std::unexpected{Error{BytecodeGeneratorErrorCode::UnresolvedSymbol}};
  }

  if (!symbol->upvalue) {
    return symbol->index;
  }

  const RegisterIndex destinationIndex{generator.state().allocateRegister()};
  generator.state().instructionWriter.write(Operation::LoadUpvalue,
                                            destinationIndex, symbol->index);
  return destinationIndex;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Subscript &node) {}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Access &node) {}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::FunctionCall &node) {
  const Result<RegisterIndex> operandIndex{
      std::visit(*this, node.operand->data)};
  if (!operandIndex) {
    return std::unexpected{operandIndex.error()};
  }

  const RegisterIndex firstArgumentIndex{generator.state().nextRegister()};
  for (std::size_t i{}; i < node.arguments.size(); ++i) {
    const Result<RegisterIndex> resolvedIndex{
        std::visit(*this, node.arguments[i]->data)};
    if (!resolvedIndex) {
      return std::unexpected{resolvedIndex.error()};
    }
    const RegisterIndex argumentIndex{firstArgumentIndex + i};

    if (resolvedIndex != argumentIndex) {
      generator.state().allocateRegister();
      generator.state().instructionWriter.write(Operation::Move, argumentIndex,
                                                *resolvedIndex);
    }
  }

  generator.state().instructionWriter.write(Operation::CallFunction,
                                            *operandIndex, firstArgumentIndex,
                                            node.arguments.size());

  return firstArgumentIndex;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::WhileLoop &node) {}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::RepeatLoop &node) {}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Conditional &node) {}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::NumericForLoop &node) {}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::GenericForLoop &node) {}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::String &node) {
  const RegisterIndex destinationIndex{generator.state().allocateRegister()};
  auto *string{
      static_cast<StringSize *>(generator.m_runtimeAllocator.get().allocate(
          sizeof(StringSize) + node.value.size(), alignof(StringSize)))};
  if (!string) {
    throw std::bad_alloc{};
  }

  *string = node.value.size();
  std::memcpy(string + 1, node.value.data(), node.value.size());

  generator.state().instructionWriter.write(
      Operation::LoadConstant, destinationIndex,
      generator.state().addConstant(string));

  return destinationIndex;
}

void BytecodeGenerator::defineNativeFunctions() {
  const std::array nativeFunctions{
      std::pair<std::string_view, NativeFunction>{"print", native::print},
      std::pair<std::string_view, NativeFunction>{"error", native::error},
      std::pair<std::string_view, NativeFunction>{"assert", native::lua_assert},
  };

  for (const auto &[name, function] : nativeFunctions) {
    const RegisterIndex constantIndex{global().addConstant(function)};
    const RegisterIndex registerIndex{global().allocateRegister()};
    global().instructionWriter.write(Operation::LoadConstant, registerIndex,
                                     constantIndex);
    global().symbolTable.try_emplace(
        std::pmr::string{name, &m_compilerAllocator.get()},
        Symbol{registerIndex});
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
  std::pmr::string key{name, &m_compilerAllocator.get()};
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
