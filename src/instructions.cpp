#include "instructions.hpp"

namespace {
inline uint8_t computeOperandSize(RegisterIndex operand) {
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
    return "LoadConstant";
  case Operation::Add:
    return "Add";
  case Operation::Subtract:
    return "Subtract";
  case Operation::Multiply:
    return "Multiply";
  case Operation::Divide:
    return "Divide";
  case Operation::CallFunction:
    return "CallFunction";
  case Operation::Copy:
    return "Move";
  case Operation::GetUpvalue:
    return "LoadUpvalue";
  }
}

Operation InstructionReader::readOperation() {
  uint8_t operation{*cursor++};
  m_operandSize = operation >> 6;
  return static_cast<Operation>(operation & 0x3F);
}

void InstructionWriter::write(Operation operation, RegisterIndex operand1) {
  m_operandSize = computeOperandSize(operand1);
  writeOperation(operation);
  writeOperand(operand1);
}

void InstructionWriter::write(Operation operation, RegisterIndex operand1,
                              RegisterIndex operand2) {
  m_operandSize =
      std::max(computeOperandSize(operand1), computeOperandSize(operand2));
  writeOperation(operation);
  writeOperand(operand1);
  writeOperand(operand2);
}

void InstructionWriter::write(Operation operation, RegisterIndex operand1,
                              RegisterIndex operand2, RegisterIndex operand3) {
  m_operandSize =
      std::max(computeOperandSize(operand1), computeOperandSize(operand2));
  m_operandSize = std::max(m_operandSize, computeOperandSize(operand3));
  writeOperation(operation);
  writeOperand(operand1);
  writeOperand(operand2);
  writeOperand(operand3);
}
