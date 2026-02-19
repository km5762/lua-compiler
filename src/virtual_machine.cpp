#include "virtual_machine.hpp"
#include "instructions.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <utility>

void VirtualMachine::run(const Function &function) {
  VirtualMachine virtualMachine{function};

  virtualMachine.run();
}

void VirtualMachine::run() {
  while (frame().instructionReader.cursor !=
         frame().function->instructions.data() +
             frame().function->instructions.size()) {
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
    case Operation::SetNil:
      setNil();
      break;
    case Operation::JumpIfFalsy:
      jumpIfFalsy();
      break;
    case Operation::Jump:
      jump();
      break;
    }
  }
}

void VirtualMachine::getConstant() {
  const RegisterIndex registerIndex{frame().instructionReader.readOperand()};
  const RegisterIndex constantIndex{frame().instructionReader.readOperand()};
  frame().registers[registerIndex] = frame().function->constants[constantIndex];
}

void VirtualMachine::callFunction() {
  const RegisterIndex functionIndex{frame().instructionReader.readOperand()};
  const Value value{getRegister(functionIndex)};

  switch (value.type) {
  case Value::Type::NativeFunction: {
    NativeFunction nativeFunction{value.data.nativeFunction};
    RegisterIndex firstArgumentIndex{frame().instructionReader.readOperand()};
    RegisterIndex argumentsSize{frame().instructionReader.readOperand()};
    std::span<Value> arguments{&frame().registers[firstArgumentIndex],
                               argumentsSize};
    nativeFunction(arguments);
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

void VirtualMachine::setNil() {
  const RegisterIndex destinationIndex{frame().instructionReader.readOperand()};
  setRegister(destinationIndex, {});
}

void VirtualMachine::jumpIfFalsy() {
  const RegisterIndex conditionIndex{frame().instructionReader.readOperand()};
  const RegisterIndex jumpIndex{frame().instructionReader.readOperand()};

  const Value condition{getRegister(conditionIndex)};
  if (!condition) {
    const std::size_t jump{frame().function->jumps[jumpIndex]};
    frame().instructionReader.cursor = &frame().function->instructions[jump];
  }
}

void VirtualMachine::jump() {
  const RegisterIndex jumpIndex{frame().instructionReader.readOperand()};
  const std::size_t jump{frame().function->jumps[jumpIndex]};
  frame().instructionReader.cursor = &frame().function->instructions[jump];
}
