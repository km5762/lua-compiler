#include "bytecode_generator.hpp"
#include "ast.hpp"
#include "instructions.hpp"
#include "native_functions.hpp"
#include "overloads.hpp"
#include "token.hpp"
#include "value.hpp"
#include <type_traits>
#include <utility>

namespace {
Error makeUnresolvedSymbolError(std::string_view name) {
  return {BytecodeGeneratorErrorCode::UnresolvedSymbol,
          std::format("Unresolved symbol: {}", name)};
}
} // namespace

Result<Function>
BytecodeGenerator::generate(const ast::Node &node,
                            std::pmr::memory_resource &compilerAllocator,
                            std::pmr::memory_resource &runtimeAllocator) {
  BytecodeGenerator generator{compilerAllocator, runtimeAllocator};
  generator.defineNativeFunctions();
  BytecodeGenerator::Visitor visitor{generator};
  const Result<RegisterIndex> index{std::visit(visitor, node.data)};
  if (!index) {
    return std::unexpected{index.error()};
  }

  return generator.state().function;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const std::monostate &node) {}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Block &node) {
  for (const auto &statement : node.statements) {
    const Result<RegisterIndex> index{std::visit(*this, statement->data)};
    if (!index) {
      return std::unexpected{index.error()};
    }
  }

  for (const auto &variable : generator.state().scope->variables) {
    generator.state().undefine(variable);
  }

  return {};
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Chunk &node) {
  return std::visit(*this, node.block->data);
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::LocalDeclaration &node) {
  const RegisterIndex firstValueIndex{generator.state().nextRegister()};
  for (std::size_t i{}; i < node.values.size(); ++i) {
    Result<RegisterIndex> valueIndex{std::visit(*this, node.values[i]->data)};
    if (!valueIndex) {
      return std::unexpected{valueIndex.error()};
    }
    if (valueIndex != firstValueIndex + i) {
      generator.state().allocateRegister();
      generator.state().instructionWriter.write(
          Operation::Copy, firstValueIndex + i, *valueIndex);
      valueIndex = firstValueIndex + i;
    }
  }

  const RegisterIndex firstVariableIndex{generator.state().nextRegister()};
  for (std::size_t i{}; i < node.names.size(); ++i) {
    const std::pmr::string key{node.names[i]};
    const RegisterIndex localIndex{generator.state().allocateRegister()};
    generator.state().define(key, Symbol{localIndex});

    if (i > node.values.size() - 1) {
      const RegisterIndex constantNilIndex{generator.state().addConstant()};
      generator.state().instructionWriter.write(Operation::SetNil, localIndex);
      continue;
    }

    const RegisterIndex valueIndex{firstValueIndex + i};
    generator.state().instructionWriter.write(Operation::Copy, localIndex,
                                              valueIndex);
  }

  return firstVariableIndex;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Function &node) {
  generator.pushState();

  for (const auto &parameter : node.parameters) {
    const RegisterIndex parameterIndex{generator.state().allocateRegister()};
    generator.state().define(parameter, {parameterIndex});
  }

  const Result<RegisterIndex> blockIndex{std::visit(*this, node.block->data)};
  if (!blockIndex) {
    return std::unexpected{blockIndex.error()};
  }

  void *storage{generator.m_runtimeAllocator.get().allocate(sizeof(Function),
                                                            alignof(Function))};
  auto allocatedFunction{new (storage)
                             Function{std::move(generator.state().function)}};
  generator.popState();

  const RegisterIndex constantIndex{
      generator.state().addConstant(allocatedFunction)};
  const RegisterIndex functionIndex{generator.state().allocateRegister()};
  generator.state().instructionWriter.write(Operation::GetConstant,
                                            functionIndex, constantIndex);

  return functionIndex;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Assignment &node) {
  const RegisterIndex firstValueIndex{generator.state().nextRegister()};
  for (std::size_t i{}; i < node.values.size(); ++i) {
    Result<RegisterIndex> valueIndex{std::visit(*this, node.values[i]->data)};
    if (!valueIndex) {
      return std::unexpected{valueIndex.error()};
    }
    if (valueIndex != firstValueIndex + i) {
      generator.state().allocateRegister();
      generator.state().instructionWriter.write(
          Operation::Copy, firstValueIndex + i, *valueIndex);
      valueIndex = firstValueIndex + i;
    }
  }

  for (std::size_t i{}; i < node.variables.size(); ++i) {
    std::optional<Symbol> variableSymbol{
        std::visit(Resolver{generator}, node.variables[i]->data)};

    if (!variableSymbol) {
      const auto name{std::get_if<ast::Name>(&node.variables[i]->data)};
      if (!name) {
        return std::unexpected{
            makeUnresolvedSymbolError(node.variables[i]->toJson().dump())};
      }
      const RegisterIndex globalIndex{generator.global().allocateRegister()};
      generator.global().define(name->name, Symbol{globalIndex});
      const std::optional<Symbol> localSymbol{generator.resolve(name->name)};
      assert(localSymbol);
      variableSymbol = localSymbol;
    }

    if (variableSymbol->flags & Symbol::immutable) {
      // NOTE: For some reason in this language this is not a compiler error and
      // will just silently fail.
      continue;
    }

    if (i > node.values.size() - 1) {
      const RegisterIndex constantNilIndex{generator.state().addConstant()};

      if (variableSymbol->flags & Symbol::upvalue) {
        const RegisterIndex localNilIndex{generator.state().allocateRegister()};
        generator.state().instructionWriter.write(Operation::SetNil,
                                                  localNilIndex);
        generator.state().instructionWriter.write(
            Operation::SetUpvalue, variableSymbol->index, localNilIndex);
      } else {
        generator.state().instructionWriter.write(Operation::SetNil,
                                                  variableSymbol->index);
      }
      continue;
    }

    Operation writeOperation{Operation::Copy};
    if (variableSymbol->flags & Symbol::upvalue) {
      writeOperation = Operation::SetUpvalue;
    }

    const RegisterIndex valueIndex{firstValueIndex + i};
    generator.state().instructionWriter.write(
        writeOperation, variableSymbol->index, valueIndex);
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
BytecodeGenerator::Visitor::operator()(const ast::Return &node) {
  const RegisterIndex firstValueIndex{generator.state().nextRegister()};
  for (std::size_t i{}; i < node.values.size(); ++i) {
    const Result<RegisterIndex> valueIndex{
        std::visit(*this, node.values[i]->data)};
    if (!valueIndex) {
      return std::unexpected{valueIndex.error()};
    }

    generator.state().copy(firstValueIndex + i, *valueIndex);
  }

  for (std::size_t i{}; i < node.values.size(); ++i) {
    generator.state().copy(i, firstValueIndex + i);
  }

  generator.state().instructionWriter.write(Operation::Return);

  return {};
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Break &node) {
  assert(!generator.state().breakJumps.empty());

  const JumpIndex breakJumpIndex{generator.state().breakJumps.back()};
  generator.state().breakJumps.pop_back();

  generator.state().instructionWriter.write(Operation::Jump, breakJumpIndex);
  return {};
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Number &node) {
  const RegisterIndex destinationIndex{generator.state().allocateRegister()};
  generator.state().instructionWriter.write(
      Operation::GetConstant, destinationIndex,
      generator.state().addConstant(node.value));

  return destinationIndex;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Boolean &node) {
  const RegisterIndex destinationIndex{generator.state().allocateRegister()};
  generator.state().instructionWriter.write(
      Operation::GetConstant, destinationIndex,
      generator.state().addConstant(node.value));

  return destinationIndex;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Name &node) {
  const std::optional<Symbol> symbol{generator.resolve(node.name)};

  if (!symbol) {
    return std::unexpected{makeUnresolvedSymbolError(node.name)};
  }

  if (!(symbol->flags & Symbol::upvalue)) {
    return symbol->index;
  }

  const RegisterIndex destinationIndex{generator.state().allocateRegister()};
  generator.state().instructionWriter.write(Operation::GetUpvalue,
                                            destinationIndex, symbol->index);
  return destinationIndex;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Subscript &node) {
  const Result<RegisterIndex> indexIndex{std::visit(*this, node.index->data)};
  if (!indexIndex) {
    return std::unexpected{indexIndex.error()};
  }
  const Result<RegisterIndex> operandIndex{
      std::visit(*this, node.operand->data)};
  if (!operandIndex) {
    return std::unexpected{operandIndex.error()};
  }

  const RegisterIndex destinationIndex{generator.state().allocateRegister()};
  generator.state().instructionWriter.write(
      Operation::GetTable, destinationIndex, *operandIndex, *indexIndex);

  return destinationIndex;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Access &node) {
  const RegisterIndex memberIndex{allocateString(node.member)};

  const Result<RegisterIndex> operandIndex{
      std::visit(*this, node.operand->data)};
  if (!operandIndex) {
    return std::unexpected{operandIndex.error()};
  }

  const RegisterIndex destinationIndex{generator.state().allocateRegister()};
  generator.state().instructionWriter.write(
      Operation::GetTable, destinationIndex, *operandIndex, memberIndex);

  return destinationIndex;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::FunctionCall &node) {
  const Result<RegisterIndex> functionIndex{
      std::visit(*this, node.operand->data)};
  if (!functionIndex) {
    return std::unexpected{functionIndex.error()};
  }

  const RegisterIndex firstArgumentIndex{generator.state().nextRegister()};
  for (std::size_t i{}; i < node.arguments.size(); ++i) {
    const Result<RegisterIndex> resolvedIndex{
        std::visit(*this, node.arguments[i]->data)};
    if (!resolvedIndex) {
      return std::unexpected{resolvedIndex.error()};
    }

    const RegisterIndex argumentIndex{firstArgumentIndex + i};
    generator.state().copy(argumentIndex, *resolvedIndex);
  }

  generator.state().instructionWriter.write(Operation::CallFunction,
                                            *functionIndex, firstArgumentIndex,
                                            node.arguments.size());

  return firstArgumentIndex;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::WhileLoop &node) {
  const JumpIndex jumpToConditionIndex{generator.state().addJump()};
  const Result<RegisterIndex> conditionIndex{
      std::visit(*this, node.condition->data)};
  if (!conditionIndex) {
    return std::unexpected{conditionIndex.error()};
  }

  const JumpIndex jumpAfterLoopIndex{generator.state().addJump()};
  generator.state().breakJumps.push_back(jumpAfterLoopIndex);
  generator.state().instructionWriter.write(
      Operation::JumpIfFalsy, *conditionIndex, jumpAfterLoopIndex);

  const Result<RegisterIndex> blockIndex{std::visit(*this, node.block->data)};
  if (!blockIndex) {
    return std::unexpected{blockIndex.error()};
  }

  generator.state().instructionWriter.write(Operation::Jump,
                                            jumpToConditionIndex);
  generator.state().setJump(jumpAfterLoopIndex);

  return {};
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::RepeatLoop &node) {
  const JumpIndex jumpToBodyIndex{generator.state().addJump()};
  const JumpIndex jumpAfterLoopIndex{generator.state().addJump()};

  generator.state().breakJumps.push_back(jumpAfterLoopIndex);
  const Result<RegisterIndex> blockIndex{std::visit(*this, node.block->data)};
  if (!blockIndex) {
    return std::unexpected{blockIndex.error()};
  }

  const Result<RegisterIndex> conditionIndex{
      std::visit(*this, node.condition->data)};
  if (!conditionIndex) {
    return std::unexpected{conditionIndex.error()};
  }

  generator.state().instructionWriter.write(Operation::JumpIfFalsy,
                                            *conditionIndex, jumpToBodyIndex);
  generator.state().setJump(jumpAfterLoopIndex);

  return {};
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Conditional &node) {
  const Result<RegisterIndex> conditionIndex{
      std::visit(*this, node.condition->data)};
  if (!conditionIndex) {
    return std::unexpected{conditionIndex.error()};
  }

  const JumpIndex jumpToAlternateIndex{generator.state().addJump()};
  generator.state().instructionWriter.write(
      Operation::JumpIfFalsy, *conditionIndex, jumpToAlternateIndex);

  const Result<RegisterIndex> blockIndex{std::visit(*this, node.block->data)};
  if (!blockIndex) {
    return std::unexpected{blockIndex.error()};
  }

  if (node.alternate) {
    const JumpIndex jumpAfterAlternateIndex{generator.state().addJump()};
    generator.state().instructionWriter.write(Operation::Jump,
                                              jumpAfterAlternateIndex);
    generator.state().setJump(jumpToAlternateIndex);
    const Result<RegisterIndex> alternateIndex{
        std::visit(*this, node.alternate->data)};
    if (!alternateIndex) {
      return std::unexpected{alternateIndex.error()};
    }
    generator.state().setJump(jumpAfterAlternateIndex);
  } else {
    generator.state().setJump(jumpToAlternateIndex);
  }

  return {};
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::NumericForLoop &node) {
  const RegisterIndex firstOperandIndex{generator.state().nextRegister()};
  const Result<RegisterIndex> startIndex{std::visit(*this, node.start->data)};
  if (!startIndex) {
    return std::unexpected{startIndex.error()};
  }
  generator.state().copy(firstOperandIndex, *startIndex);
  const Result<RegisterIndex> endIndex{std::visit(*this, node.end->data)};
  if (!endIndex) {
    return std::unexpected{endIndex.error()};
  }
  generator.state().copy(firstOperandIndex + 1, *endIndex);
  Result<RegisterIndex> incrementIndex{};
  if (!node.increment) {
    const RegisterIndex constantIndex{generator.state().addConstant(1.0)};
    incrementIndex = generator.state().allocateRegister();
    generator.state().instructionWriter.write(Operation::GetConstant,
                                              *incrementIndex, constantIndex);
  } else {
    incrementIndex = std::visit(*this, node.increment->data);
    if (!incrementIndex) {
      return std::unexpected{incrementIndex.error()};
    }
  }
  generator.state().copy(firstOperandIndex + 2, *incrementIndex);

  const JumpIndex jumpAfterLoopIndex{generator.state().addJump()};
  generator.state().instructionWriter.write(
      Operation::NumericForLoop, firstOperandIndex, jumpAfterLoopIndex);

  generator.state().define(node.variable,
                           {firstOperandIndex, Symbol::immutable});
  generator.state().breakJumps.push_back(jumpAfterLoopIndex);
  const Result<RegisterIndex> blockIndex{std::visit(*this, node.block->data)};
  if (!blockIndex) {
    return std::unexpected{blockIndex.error()};
  }
  generator.state().setJump(jumpAfterLoopIndex);
  generator.state().undefine(node.variable);

  return {};
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::GenericForLoop &node) {}

RegisterIndex
BytecodeGenerator::Visitor::allocateString(std::string_view stringView) {
  const RegisterIndex destinationIndex{generator.state().allocateRegister()};
  auto *string{
      static_cast<StringSize *>(generator.m_runtimeAllocator.get().allocate(
          sizeof(StringSize) + stringView.size(), alignof(StringSize)))};
  if (!string) {
    throw std::bad_alloc{};
  }

  *string = stringView.size();
  std::memcpy(string + 1, stringView.data(), stringView.size());

  generator.state().instructionWriter.write(
      Operation::GetConstant, destinationIndex,
      generator.state().addConstant(string));

  return destinationIndex;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::String &node) {
  return allocateString(node.value);
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::Nil &node) {
  const RegisterIndex destinationIndex{generator.state().allocateRegister()};
  generator.state().instructionWriter.write(Operation::GetConstant,
                                            destinationIndex,
                                            generator.state().addConstant());

  return destinationIndex;
}

Result<RegisterIndex>
BytecodeGenerator::Visitor::operator()(const ast::TableConstructor &node) {
  const RegisterIndex tableIndex{generator.state().allocateRegister()};
  generator.state().instructionWriter.write(Operation::NewTable, tableIndex);

  Operation operation{Operation::SetList};
  for (const auto &[field, value] : node.fields) {
    std::visit(
        Overloads{[](const ast::Number &node) {},
                  [&operation](auto &&) { operation = Operation::SetTable; }},
        field->data);
    if (operation == Operation::SetTable) {
      break;
    }
  }

  const RegisterIndex firstValueIndex{generator.state().nextRegister()};
  for (std::size_t i{}; i < node.fields.size(); ++i) {
    const auto &[field, value] = node.fields[i];
    const Result<RegisterIndex> valueIndex{std::visit(*this, value->data)};
    if (!valueIndex) {
      return std::unexpected{valueIndex.error()};
    }

    if (operation == Operation::SetList) {
      if (valueIndex != firstValueIndex + i) {
        generator.state().allocateRegister();
        generator.state().instructionWriter.write(
            Operation::Copy, firstValueIndex + i, *valueIndex);
      }
    } else {
      assert(operation == Operation::SetTable);

      Result<RegisterIndex> fieldIndex{};
      if (const ast::Name *name{std::get_if<ast::Name>(&field->data)}) {
        fieldIndex = allocateString(name->name);
      } else {
        fieldIndex = std::visit(*this, field->data);
      }

      if (!fieldIndex) {
        return std::unexpected{fieldIndex.error()};
      }

      generator.state().instructionWriter.write(Operation::SetTable, tableIndex,
                                                *fieldIndex, *valueIndex);
    }
  }

  if (operation == Operation::SetList) {
    generator.state().instructionWriter.write(
        Operation::SetList, tableIndex, firstValueIndex, node.fields.size());
  }

  return tableIndex;
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
    global().instructionWriter.write(Operation::GetConstant, registerIndex,
                                     constantIndex);
    global().define(name, Symbol{registerIndex});
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
        key, Symbol{current->function.upvalues.size(), Symbol::upvalue});

    it = currentOuter->symbolTable.find(key);
    if (it != currentOuter->symbolTable.end()) {
      current->function.upvalues.emplace_back(
          it->second.index, it->second.flags & Symbol::upvalue);
      return state().symbolTable[key];
    }
    current->function.upvalues.emplace_back(
        currentOuter->function.upvalues.size(), true);

    current = current->outer;
    currentOuter = current->outer;
  }

  return std::nullopt;
}

std::optional<BytecodeGenerator::Symbol>
BytecodeGenerator::Resolver::operator()(const ast::Name &node) {
  return generator.resolve(node.name);
}

std::optional<BytecodeGenerator::Symbol>
BytecodeGenerator::Resolver::operator()(const ast::Access &node) {
  return std::visit(*this, node.operand->data);
}

std::optional<BytecodeGenerator::Symbol>
BytecodeGenerator::Resolver::operator()(const ast::Subscript &node) {
  return std::visit(*this, node.operand->data);
}

void BytecodeGenerator::State::define(std::string_view name, Symbol symbol) {
  std::pmr::string key{name, &m_compilerAllocator.get()};
  auto result{symbolTable.insert({std::move(key), symbol})};
  if (result.second) {
    return;
  }
  static_assert(std::is_trivially_copyable<Symbol>());
  auto oldValue{static_cast<Symbol *>(
      m_compilerAllocator.get().allocate(sizeof(Symbol), alignof(Symbol)))};
  if (!oldValue) {
    throw std::bad_alloc{};
  }
  std::memcpy(oldValue, &result.first->second, sizeof(result.first->second));
  symbol.outer = oldValue;
  result.first->second = std::move(symbol);
}

void BytecodeGenerator::State::undefine(std::string_view name) {
  std::pmr::string key{name, &m_compilerAllocator.get()};
  auto it{symbolTable.find(key)};
  if (it == symbolTable.end()) {
    return;
  }
  Symbol &symbol{it->second};
  if (symbol.outer) {
    it->second = *symbol.outer;
  } else {
    symbolTable.erase(it);
  }
}
