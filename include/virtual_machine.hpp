#pragma once

#include "instructions.hpp"
#include "value.hpp"

#include <array>
#include <format>
#include <iostream>

struct Frame {
  InstructionReader instructionReader{};
  const Function *function{};
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
  Value readOperandRegister() {
    return getRegister(frame().instructionReader.readOperand());
  }
  std::size_t readOperandJump() {
    return frame().function->jumps[frame().instructionReader.readOperand()];
  }
  Value &getRegister(RegisterIndex index) {
    return m_stack[frame().base + index];
  }
  void setRegister(RegisterIndex index, Value value) {
    m_stack[frame().base + index] = value;
  }
  Value getConstant(RegisterIndex index) {
    return frame().function->constants[index];
  }
  void pushFrame(const Function *function, std::size_t base) {
    m_frames[++m_frameIndex] = {
        {function->instructions.data()}, function, base};
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
  void setNil();
  void jumpIfFalsy();
  void jump();
  void newTable();
  void setTable();
  void setList();
  void getTable();
  void numericForLoop();
};
