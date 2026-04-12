#pragma once

#include "instructions.hpp"
#include "value.hpp"

#include <array>
#include <cassert>
#include <format>
#include <iostream>

struct Frame {
  InstructionReader instructionReader{};
  const Function *function{};
  Closure *closure{};
  std::size_t base{};
};

constexpr std::size_t maxFrames{1024};

class VirtualMachine {
public:
  static void run(const Function &function);

private:
  std::vector<Value> m_stack{};
  std::size_t m_frameIndex{};
  std::array<Frame, maxFrames> m_frames{};

  VirtualMachine(const Function &function) {
    m_stack.resize(function.registerCount);
    m_frames[m_frameIndex] = {{function.instructions.data()}, &function};
  }

  void runInstruction();
  Frame &frame() { return m_frames[m_frameIndex]; }
  Value &readOperandRegister() {
    return getRegister(frame().instructionReader.readOperand());
  }
  std::size_t readOperandJump() {
    return frame().function->jumps[frame().instructionReader.readOperand()];
  }
  Value &readOperandUpvalue() {
    const RegisterIndex upvalueIndex{frame().instructionReader.readOperand()};
    assert(frame().closure->upvalueIndices.size() > upvalueIndex);
    const StackIndex stackIndex{frame().closure->upvalueIndices[upvalueIndex]};

    return m_stack[stackIndex];
  }
  RegisterIndex stackIndex(const Frame &frame, RegisterIndex index) {
    return frame.base + index;
  }
  Value &getRegister(RegisterIndex index) {
    return m_stack[stackIndex(frame(), index)];
  }
  Value &getRegister(Frame &frame, RegisterIndex index) {
    return m_stack[stackIndex(frame, index)];
  }
  void setRegister(RegisterIndex index, Value value) {
    m_stack[frame().base + index] = value;
  }
  Value getConstant(RegisterIndex index) {
    return frame().function->constants[index];
  }
  void pushFrame(const Function *function, Closure *closure, std::size_t base) {
    m_frames[++m_frameIndex] = {
        {function->instructions.data()}, function, closure, base};
  }
  void popFrame() { --m_frameIndex; }
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

    using Result = std::invoke_result_t<Op, Value, Value>;
    if constexpr (std::is_same_v<Result, std::optional<Value>>) {
      auto result = operation(leftOperand, rightOperand);
      if (!result) {
        panic("Invalid operands to binary operation: {}, {}", leftOperand,
              rightOperand);
      }
      setRegister(destinationRegisterIndex, *result);
    } else {
      setRegister(destinationRegisterIndex,
                  operation(leftOperand, rightOperand));
    }
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
  void getUpvalue();
  void setUpvalue();
  void jumpIfFalsy();
  void jump();
  void newTable();
  void setTable();
  void setList();
  void getTable();
  void numericForLoop();
  void newClosure();
};
