#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

struct Value;
struct VirtualMachine;

using RegisterIndex = uint64_t;
using StringSize = std::size_t;
using NativeFunction = Value (*)(std::span<Value> arguments);

struct Function;

struct Value {
  enum class Type {
    Nil,
    Number,
    String,
    Function,
    NativeFunction,
    Boolean,
  };

  Type type{};
  union {
    double number;
    StringSize *string;
    Function *function;
    NativeFunction nativeFunction;
    bool boolean;
  } data;

  Value() : type{Type::Nil} {};
  Value(double number) : type{Type::Number} { data.number = number; }
  Value(StringSize *string) : type{Type::String} { data.string = string; }
  Value(Function *function) : type{Type::Function} { data.function = function; }
  Value(NativeFunction nativeFunction) : type{Type::NativeFunction} {
    data.nativeFunction = nativeFunction;
  }
  Value(bool boolean) : type{Type::Boolean} { data.boolean = boolean; }

  std::string toString() const {
    switch (type) {
    case Type::Nil:
      return "nil";
    case Type::Number:
      return std::to_string(data.number);
    case Type::String: {
      const StringSize length{*data.string};
      const char *string{reinterpret_cast<const char *>(data.string + 1)};
      return std::string{string, length};
    }
    case Type::Function:
      return "function: " +
             std::to_string(reinterpret_cast<uintptr_t>(data.function));
    case Type::NativeFunction:
      return "native function: " +
             std::to_string(reinterpret_cast<uintptr_t>(data.nativeFunction));
      break;
    case Type::Boolean:
      return data.boolean ? "true" : "false";
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
