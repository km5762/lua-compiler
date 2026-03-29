#include "compilation.hpp"
#include "ast.hpp"
#include "bytecode_generator.hpp"
#include "parser.hpp"
#include "scanner.hpp"
#include "value.hpp"
#include "virtual_machine.hpp"
#include <memory_resource>

namespace compilation {
namespace {
Result<Function> compile(std::string_view program) {
  std::pmr::monotonic_buffer_resource compilerAllocator{};
  Scanner scanner{program, compilerAllocator};
  Result<ast::Node *> result{Parser::parse(scanner, compilerAllocator)};
  if (!result) {
    return std::unexpected{result.error()};
  }
  std::pmr::monotonic_buffer_resource runtimeAllocator{};
  Result<Function> function{BytecodeGenerator::generate(
      **result, compilerAllocator, runtimeAllocator)};
  return function;
}
} // namespace

std::optional<Error> run(std::string_view program) {
  const Result<Function> function{compile(program)};
  if (!function) {
    return function.error();
  }
  VirtualMachine::run(*function);

  return {};
}

std::optional<Error> disassemble(std::string_view program) {
  const Result<Function> function{compile(program)};
  if (!function) {
    return function.error();
  }

  std::cout << "=== Function ===\n";
  std::cout << std::format("Registers: {}\n", function->registerCount);

  std::cout << std::format("\n-- Constants ({}) --\n",
                           function->constants.size());
  for (std::size_t i{}; i < function->constants.size(); ++i) {
    std::cout << std::format("  c{}: {}\n", i,
                             function->constants[i].toString());
  }

  std::cout << std::format("\n-- Upvalues ({}) --\n",
                           function->upvalues.size());
  for (std::size_t i{}; i < function->upvalues.size(); ++i) {
    const Upvalue &upvalue{function->upvalues[i]};
    std::cout << std::format("  u{}: index={} outer={}\n", i, upvalue.index,
                             upvalue.inOuterUpvalues);
  }

  std::cout << std::format("\n-- Jumps ({}) --\n", function->jumps.size());
  for (std::size_t i{}; i < function->jumps.size(); ++i) {
    std::cout << std::format("  j{}: offset={}\n", i, function->jumps[i]);
  }

  std::cout << "\n-- Instructions --\n";
  InstructionReader reader{function->instructions.data()};
  while (reader.cursor <= &function->instructions.back()) {
    const Operation operation{reader.readOperation()};
    switch (operation) {
    case Operation::GetConstant: {
      const RegisterIndex registerIndex{reader.readOperand()};
      const RegisterIndex constantIndex{reader.readOperand()};
      std::cout << std::format("  GetConstant r{} c{}\n", registerIndex,
                               constantIndex);
      break;
    }
    case Operation::Add:
    case Operation::Subtract:
    case Operation::Multiply:
    case Operation::Divide:
    case Operation::Modulo:
    case Operation::Power:
    case Operation::Equal:
    case Operation::NotEqual:
    case Operation::LessThan:
    case Operation::LessThanOrEqual:
    case Operation::GreaterThan:
    case Operation::GreaterThanOrEqual: {
      const RegisterIndex destinationIndex{reader.readOperand()};
      const RegisterIndex leftIndex{reader.readOperand()};
      const RegisterIndex rightIndex{reader.readOperand()};
      const std::string_view name{[operation] {
        switch (operation) {
        case Operation::Add:
          return "Add";
        case Operation::Subtract:
          return "Subtract";
        case Operation::Multiply:
          return "Multiply";
        case Operation::Divide:
          return "Divide";
        case Operation::Modulo:
          return "Modulo";
        case Operation::Power:
          return "Power";
        case Operation::Equal:
          return "Equal";
        case Operation::NotEqual:
          return "NotEqual";
        case Operation::LessThan:
          return "LessThan";
        case Operation::LessThanOrEqual:
          return "LessThanOrEqual";
        case Operation::GreaterThan:
          return "GreaterThan";
        case Operation::GreaterThanOrEqual:
          return "GreaterThanOrEqual";
        default:
          return "Unknown";
        }
      }()};
      std::cout << std::format("  {} r{} r{} r{}\n", name, destinationIndex,
                               leftIndex, rightIndex);
      break;
    }
    case Operation::Minus: {
      const RegisterIndex destinationIndex{reader.readOperand()};
      const RegisterIndex sourceIndex{reader.readOperand()};
      std::cout << std::format("  Minus r{} r{}\n", destinationIndex,
                               sourceIndex);
      break;
    }
    case Operation::CallFunction: {
      const RegisterIndex functionIndex{reader.readOperand()};
      const RegisterIndex firstArgumentIndex{reader.readOperand()};
      const RegisterIndex argumentsSize{reader.readOperand()};
      std::cout << std::format("  CallFunction r{} r{} {}\n", functionIndex,
                               firstArgumentIndex, argumentsSize);
      break;
    }
    case Operation::Copy: {
      const RegisterIndex destinationIndex{reader.readOperand()};
      const RegisterIndex sourceIndex{reader.readOperand()};
      std::cout << std::format("  Copy r{} r{}\n", destinationIndex,
                               sourceIndex);
      break;
    }
    case Operation::GetUpvalue: {
      const RegisterIndex registerIndex{reader.readOperand()};
      const RegisterIndex upvalueIndex{reader.readOperand()};
      std::cout << std::format("  GetUpvalue r{} u{}\n", registerIndex,
                               upvalueIndex);
      break;
    }
    case Operation::SetUpvalue: {
      const RegisterIndex upvalueIndex{reader.readOperand()};
      const RegisterIndex registerIndex{reader.readOperand()};
      std::cout << std::format("  SetUpvalue u{} r{}\n", upvalueIndex,
                               registerIndex);
      break;
    }
    case Operation::JumpIfFalsy: {
      const RegisterIndex conditionIndex{reader.readOperand()};
      const JumpIndex jumpIndex{reader.readOperand()};
      std::cout << std::format("  JumpIfFalsy r{} j{}\n", conditionIndex,
                               jumpIndex);
      break;
    }
    case Operation::Jump: {
      const JumpIndex jumpIndex{reader.readOperand()};
      std::cout << std::format("  Jump j{}\n", jumpIndex);
      break;
    }
    case Operation::NewTable: {
      const RegisterIndex tableIndex{reader.readOperand()};
      std::cout << std::format("  NewTable r{}\n", tableIndex);
      break;
    }
    case Operation::SetTable: {
      const RegisterIndex tableIndex{reader.readOperand()};
      const RegisterIndex fieldIndex{reader.readOperand()};
      const RegisterIndex valueIndex{reader.readOperand()};
      std::cout << std::format("  SetTable r{} r{} r{}\n", tableIndex,
                               fieldIndex, valueIndex);
      break;
    }
    case Operation::SetList: {
      const RegisterIndex tableIndex{reader.readOperand()};
      const RegisterIndex firstValueIndex{reader.readOperand()};
      const std::size_t size{reader.readOperand()};
      std::cout << std::format("  SetList r{} r{} {}\n", tableIndex,
                               firstValueIndex, size);
      break;
    }
    case Operation::GetTable: {
      const RegisterIndex destinationIndex{reader.readOperand()};
      const RegisterIndex tableIndex{reader.readOperand()};
      const RegisterIndex fieldIndex{reader.readOperand()};
      std::cout << std::format("  GetTable r{} r{} r{}\n", destinationIndex,
                               tableIndex, fieldIndex);
      break;
    }
    case Operation::NumericForLoop: {
      const RegisterIndex variableIndex{reader.readOperand()};
      const JumpIndex jumpIndex{reader.readOperand()};
      std::cout << std::format("  NumericForLoop r{} j{}\n", variableIndex,
                               jumpIndex);
      break;
    }
    case Operation::Return:
      std::cout << "  Return\n";
      break;
    }
  }

  return {};
}
} // namespace compilation
