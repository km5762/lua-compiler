#include "virtual_machine.hpp"
#include "instructions.hpp"
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
    case Operation::LoadConstant:
      loadConstant();
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
    case Operation::CallFunction:
      callFunction();
      break;
    case Operation::Move:
      move();
      break;
    }
  }
}

void VirtualMachine::loadConstant() {
  const RegisterIndex constantIndex{frame().instructionReader.readOperand()};
  const RegisterIndex registerIndex{frame().instructionReader.readOperand()};
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

void VirtualMachine::move() {
  const RegisterIndex destinationIndex{frame().instructionReader.readOperand()};
  const RegisterIndex sourceIndex{frame().instructionReader.readOperand()};
  setRegister(destinationIndex, getRegister(sourceIndex));
}
