#pragma once

#include "instructions.hpp"
#include "value.hpp"

#include <array>
#include <format>
#include <iostream>

class Stack {
public:
  Value *allocate(std::size_t size) {
    m_stackIndex += size;
    if (m_stackIndex > m_stack.size()) {
      m_stack.resize(m_stackIndex);
    }
    return &m_stack[m_stack.size() - size];
  }

  void free(std::size_t size) { m_stackIndex -= size; }

private:
  std::vector<Value> m_stack{};
  std::size_t m_stackIndex{};
};

struct Frame {
  Value *registers{};
  InstructionReader instructionReader{};
  const Function *function{};
};

constexpr std::size_t maxFrames{1024};

class VirtualMachine {
public:
  static void run(const Function &function);

private:
  Stack m_stack{};
  std::size_t m_frameIndex{};
  std::array<Frame, maxFrames> m_frames{};

  VirtualMachine(const Function &function) { loadFunction(function); }

  void run();
  Frame &frame() { return m_frames[m_frameIndex]; }
  Value getRegister(RegisterIndex index) { return frame().registers[index]; }
  void setRegister(RegisterIndex index, Value value) {
    frame().registers[index] = value;
  }
  Value getConstant(RegisterIndex index) {
    return frame().function->constants[index];
  }
  void pushFrame(Value *registers, InstructionReader instructionReader,
                 const Function *function) {
    m_frames[m_frameIndex++] = {registers, instructionReader, function};
  }
  void popFrame() { --m_frameIndex; }
  void loadFunction(const Function &function) {
    Value *registers{m_stack.allocate(function.registerCount)};
    m_frames[m_frameIndex] = {
        registers, {function.instructions.data()}, &function};
  }
  template <typename... Args>
  [[noreturn]] void panic(std::string_view message, Args &&...args) {
    auto formatted = std::vformat(message, std::make_format_args(args...));
    std::cerr << formatted << '\n';
    std::terminate();
  }
  void getConstant();
  template <typename Op> void binaryOperation(Op operation) {
    const RegisterIndex destinationRegisterIndex{
        frame().instructionReader.readOperand()};
    const RegisterIndex leftOperandIndex{
        frame().instructionReader.readOperand()};
    const RegisterIndex rightOperandIndex{
        frame().instructionReader.readOperand()};
    const Value leftOperand{getRegister(leftOperandIndex)};
    const Value rightOperand{getRegister(rightOperandIndex)};
    const std::optional<Value> result{
        operation(leftOperand.data.number, rightOperand.data.number)};
    if (!result) {
      panic("Invalid operands to binary operation: {}, {}", leftOperand,
            rightOperand);
    }
    setRegister(destinationRegisterIndex, *result);
  }
  template <typename Op> void unaryOperation(Op operation) {
    const RegisterIndex destinationRegisterIndex{
        frame().instructionReader.readOperand()};
    const RegisterIndex operand{frame().instructionReader.readOperand()};
    Value result{operation(getRegister(operand).data.number)};
    setRegister(destinationRegisterIndex, result);
  }

  void callFunction();
  void copy();
  void setNil();
};
