#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

struct Value;
struct VirtualMachine;

using RegisterIndex = uint64_t;
using NativeFunction = Value (*)(std::span<Value> arguments);

struct Function;

struct Value {
  enum class Type {
    Number,
    Function,
    NativeFunction,
  };

  Type type{};
  union {
    double number;
    Function *function;
    NativeFunction nativeFunction;
  } data;

  Value() = default;
  Value(double number) : type{Type::Number} { data.number = number; }
  Value(Function *function) : type{Type::Function} { data.function = function; }
  Value(NativeFunction nativeFunction) : type{Type::NativeFunction} {
    data.nativeFunction = nativeFunction;
  }

  std::string toString() const {
    switch (type) {
    case Type::Number:
      return std::to_string(data.number);
    case Type::Function:
      return "function: " +
             std::to_string(reinterpret_cast<uintptr_t>(data.function));
    case Type::NativeFunction:
      return "nativeFunction: " +
             std::to_string(reinterpret_cast<uintptr_t>(data.nativeFunction));
      break;
    }
  }
};

struct Upvalue {
  RegisterIndex index{};
  bool inOuterUpvalues{};
};

struct Function {
  std::pmr::vector<uint8_t> instructions{};
  std::pmr::vector<Value> constants{};
  std::pmr::vector<Upvalue> upvalues{};
  RegisterIndex registerCount{};
};
