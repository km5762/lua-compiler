#pragma once

#include "value.hpp"

#include <cstdint>
#include <cstring>

enum class Operation : uint8_t {
  LoadConstant,
  Add,
  Subtract,
  Multiply,
  Divide,
  CallFunction,
  Move,
  LoadUpvalue,
};

std::string toString(Operation operation);

class InstructionReader {
public:
  const uint8_t *cursor{};

  InstructionReader() = default;
  InstructionReader(const uint8_t *cursor) : cursor{cursor} {}

  Operation readOperation();
  uint64_t readOperand() {
    uint64_t operand{};
    std::memcpy(&operand, cursor, m_operandSize);
    cursor += m_operandSize;
    return operand;
  }

private:
  uint8_t m_operandSize{};
};

class InstructionWriter {
public:
  std::pmr::vector<uint8_t> &instructions;

  InstructionWriter(std::pmr::vector<uint8_t> &instructions)
      : instructions{instructions} {}

  void write(Operation operation, RegisterIndex operand1);
  void write(Operation operation, RegisterIndex operand1,
             RegisterIndex operand2);
  void write(Operation operation, RegisterIndex operand1,
             RegisterIndex operand2, RegisterIndex operand3);

private:
  uint8_t m_operandSize{};

  inline void writeOperation(Operation operation) {
    instructions.push_back((m_operandSize << 6) |
                           static_cast<uint8_t>(operation));
  }

  inline void writeOperand(RegisterIndex operand) {
    std::size_t offset{instructions.size()};
    instructions.resize(instructions.size() + m_operandSize);
    std::memcpy(instructions.data() + offset, &operand, m_operandSize);
  }
};
