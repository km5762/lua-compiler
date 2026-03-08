#include "instructions.hpp"

namespace {
inline uint8_t computeOperandSize(uint64_t operand) {
  uint8_t size{1};
  size += (operand > 0xFF);
  size += (operand > 0xFFFF);
  size += (operand > 0xFFFFFFFF);
  return size;
}
} // namespace

std::string toString(Operation operation) {
  switch (operation) {
  case Operation::GetConstant:
    return "GetConstant";
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
  case Operation::Minus:
    return "Minus";
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
  case Operation::CallFunction:
    return "CallFunction";
  case Operation::Copy:
    return "Copy";
  case Operation::GetUpvalue:
    return "GetUpvalue";
  case Operation::SetUpvalue:
    return "SetUpvalue";
  case Operation::SetNil:
    return "SetNil";
  case Operation::JumpIfFalsy:
    return "JumpIfFalsy";
  case Operation::Jump:
    return "Jump";
  case Operation::NewTable:
    return "NewTable";
  case Operation::SetTable:
    return "SetTable";
  case Operation::SetList:
    return "SetList";
  case Operation::GetTable:
    return "GetTable";
  case Operation::NumericForLoop:
    return "NumericForLoop";
  default:
    return "Unknown";
  }
}

Operation InstructionReader::readOperation() {
  uint8_t operation{*cursor++};
  m_operandSize = operation >> 6;
  return static_cast<Operation>(operation & 0x3F);
}

void InstructionWriter::write(Operation operation) {
  writeOperation(operation);
}

void InstructionWriter::write(Operation operation, uint64_t operand1) {
  m_operandSize = computeOperandSize(operand1);
  writeOperation(operation);
  writeOperand(operand1);
}

void InstructionWriter::write(Operation operation, uint64_t operand1,
                              uint64_t operand2) {
  m_operandSize =
      std::max(computeOperandSize(operand1), computeOperandSize(operand2));
  writeOperation(operation);
  writeOperand(operand1);
  writeOperand(operand2);
}

void InstructionWriter::write(Operation operation, uint64_t operand1,
                              uint64_t operand2, uint64_t operand3) {
  m_operandSize =
      std::max(computeOperandSize(operand1), computeOperandSize(operand2));
  m_operandSize = std::max(m_operandSize, computeOperandSize(operand3));
  writeOperation(operation);
  writeOperand(operand1);
  writeOperand(operand2);
  writeOperand(operand3);
}
