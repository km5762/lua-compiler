#include "virtual_machine.hpp"
#include "bytecode_generator.hpp"
#include "instructions.hpp"
#include "value.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <utility>

void VirtualMachine::run(const Function &function) {
  VirtualMachine virtualMachine{function};

  while (virtualMachine.frame().instructionReader.cursor !=
         virtualMachine.frame().function->instructions.data() +
             virtualMachine.frame().function->instructions.size()) {
    virtualMachine.runInstruction();
  }
}

void VirtualMachine::runInstruction() {
  const Operation operation{frame().instructionReader.readOperation()};

  switch (operation) {
  case Operation::GetConstant:
    getConstant();
    break;
  case Operation::Add:
    binaryOperation(std::plus{});
    break;
  case Operation::Subtract:
    binaryOperation(std::minus{});
    break;
  case Operation::Multiply:
    binaryOperation(std::multiplies{});
    break;
  case Operation::Divide:
    binaryOperation(std::divides{});
    break;
  case Operation::Modulo:
    binaryOperation(std::modulus{});
    break;
  case Operation::Power:
    binaryOperation(
        [](Value base, Value exponent) { return base.power(exponent); });
    break;
  case Operation::Minus:
    unaryOperation([](auto a) { return -a; });
    break;
  case Operation::Equal:
    binaryOperation(std::equal_to{});
    break;
  case Operation::NotEqual:
    binaryOperation(std::not_equal_to{});
    break;
  case Operation::LessThan:
    binaryOperation(std::less{});
    break;
  case Operation::LessThanOrEqual:
    binaryOperation(std::less_equal{});
    break;
  case Operation::GreaterThan:
    binaryOperation(std::greater{});
    break;
  case Operation::GreaterThanOrEqual:
    binaryOperation(std::greater_equal{});
    break;
  case Operation::CallFunction:
    callFunction();
    break;
  case Operation::Copy:
    copy();
    break;
  case Operation::GetUpvalue:
    assert(false);
    break;
  case Operation::SetUpvalue:
    assert(false);
    break;
  case Operation::JumpIfFalsy:
    jumpIfFalsy();
    break;
  case Operation::Jump:
    jump();
    break;
  case Operation::NewTable:
    newTable();
    break;
  case Operation::SetTable:
    setTable();
    break;
  case Operation::SetList:
    setList();
    break;
  case Operation::GetTable:
    getTable();
    break;
  case Operation::NumericForLoop:
    numericForLoop();
    break;
  case Operation::Return:
    popFrame();
    break;
  }
}

void VirtualMachine::getConstant() {
  const RegisterIndex registerIndex{frame().instructionReader.readOperand()};
  const RegisterIndex constantIndex{frame().instructionReader.readOperand()};
  setRegister(registerIndex, getConstant(constantIndex));
}

void VirtualMachine::callFunction() {
  const RegisterIndex functionIndex{frame().instructionReader.readOperand()};
  const Value value{getRegister(functionIndex)};
  const RegisterIndex firstArgumentIndex{
      frame().instructionReader.readOperand()};
  const RegisterIndex argumentsSize{frame().instructionReader.readOperand()};

  switch (value.type) {
  case Value::Type::NativeFunction: {
    const NativeFunction nativeFunction{value.data.nativeFunction};
    const std::span<Value> arguments{&getRegister(firstArgumentIndex),
                                     argumentsSize};
    nativeFunction(arguments);
    break;
  }
  case Value::Type::Function: {
    const Function *function{value.data.function};
    const std::size_t requiredStackSize{frame().base + firstArgumentIndex +
                                        function->registerCount};

    if (requiredStackSize > m_stack.size()) {
      m_stack.resize(requiredStackSize);
    }

    pushFrame(function, frame().base + firstArgumentIndex);
    break;
  }
  default:
    std::unreachable();
  }
}

void VirtualMachine::copy() {
  const RegisterIndex destinationIndex{frame().instructionReader.readOperand()};
  const RegisterIndex sourceIndex{frame().instructionReader.readOperand()};
  setRegister(destinationIndex, getRegister(sourceIndex));
}

void VirtualMachine::jumpIfFalsy() {
  const RegisterIndex conditionIndex{frame().instructionReader.readOperand()};
  const JumpIndex jumpIndex{frame().instructionReader.readOperand()};

  const Value condition{getRegister(conditionIndex)};
  if (!condition) {
    const std::size_t jump{frame().function->jumps[jumpIndex]};
    frame().instructionReader.cursor = &frame().function->instructions[jump];
  }
}

void VirtualMachine::jump() {
  const JumpIndex jumpIndex{frame().instructionReader.readOperand()};
  const std::size_t jump{frame().function->jumps[jumpIndex]};
  frame().instructionReader.cursor = &frame().function->instructions[jump];
}

void VirtualMachine::newTable() {
  const RegisterIndex tableIndex{frame().instructionReader.readOperand()};
  // TODO: Implement GC so this doesn't leak
  setRegister(tableIndex, new Table{});
}

void VirtualMachine::setTable() {
  const RegisterIndex tableIndex{frame().instructionReader.readOperand()};
  const RegisterIndex fieldIndex{frame().instructionReader.readOperand()};
  const RegisterIndex valueIndex{frame().instructionReader.readOperand()};

  Value table{getRegister(tableIndex)};
  assert(table.type == Value::Type::Table);
  assert(table.data.table);
  const Value field{getRegister(fieldIndex)};
  const Value value{getRegister(valueIndex)};

  table.data.table->set(field, value);
}

void VirtualMachine::setList() {
  const RegisterIndex tableIndex{frame().instructionReader.readOperand()};
  const RegisterIndex firstValueIndex{frame().instructionReader.readOperand()};
  const std::size_t size{frame().instructionReader.readOperand()};

  Value table{getRegister(tableIndex)};
  assert(table.type == Value::Type::Table);
  assert(table.data.table);

  for (std::size_t i{}; i < size; ++i) {
    const Value value{getRegister(firstValueIndex + i)};
    table.data.table->set(static_cast<double>(i), value);
  }
}

void VirtualMachine::getTable() {
  const RegisterIndex destinationIndex{frame().instructionReader.readOperand()};
  const RegisterIndex tableIndex{frame().instructionReader.readOperand()};
  const RegisterIndex fieldIndex{frame().instructionReader.readOperand()};

  Value table{getRegister(tableIndex)};
  assert(table.type == Value::Type::Table);
  assert(table.data.table);

  Value value{table.data.table->get(getRegister(fieldIndex))};
  setRegister(destinationIndex, value);
}

void VirtualMachine::numericForLoop() {
  const RegisterIndex variableIndex{frame().instructionReader.readOperand()};
  const RegisterIndex endIndex{variableIndex + 1};
  const RegisterIndex incrementIndex{variableIndex + 2};
  const std::size_t jumpAfterLoop{readOperandJump()};

  const uint8_t *jumpToStartOfLoop{frame().instructionReader.cursor};
  while (true) {
    const Value variable{getRegister(variableIndex)};
    const Value end{getRegister(endIndex)};
    const Value increment{getRegister(incrementIndex)};
    if ((increment.data.number > 0 && variable.data.number > end.data.number) ||
        (increment.data.number < 0 && variable.data.number < end.data.number)) {
      frame().instructionReader.cursor =
          &frame().function->instructions[jumpAfterLoop];
      return;
    }

    while (frame().instructionReader.cursor !=
           &frame().function->instructions[jumpAfterLoop]) {
      runInstruction();
    }

    frame().instructionReader.cursor = jumpToStartOfLoop;
    const double newValue{variable.data.number + increment.data.number};
    setRegister(variableIndex, newValue);
  }
}
