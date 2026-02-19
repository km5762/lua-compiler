#pragma once

#include <cmath>
#include <cstdint>
#include <format>
#include <optional>
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

  explicit operator bool() const {
    return !(type == Type::Nil || (type == Type::Boolean && !data.boolean));
  }

  std::optional<Value> operator==(Value other) const {
    if (type != other.type) {
      return false;
    }

    switch (type) {
    case Type::Nil:
      return true;
    case Type::Number:
      return data.number == other.data.number;
    case Type::String:
      return data.string == other.data.string;
    case Type::Function:
      return data.function == other.data.function;
    case Type::NativeFunction:
      return data.nativeFunction == other.data.nativeFunction;
    case Type::Boolean:
      return data.boolean == other.data.boolean;
    }

    return std::nullopt;
  }

  std::optional<Value> operator!=(Value other) const {
    const std::optional<Value> result{*this == other};
    if (!result) {
      return std::nullopt;
    }
    return !result->data.boolean;
  }

private:
  template <typename Op>
  std::optional<Value> numericBinaryOperation(Value other, Op operation) const {
    if (type != Type::Number || other.type != Type::Number) {
      return std::nullopt;
    }

    return operation(data.number, other.data.number);
  }

public:
  std::optional<Value> operator+(Value other) const {
    return numericBinaryOperation(other, std::plus{});
  }

  std::optional<Value> operator-(Value other) const {
    return numericBinaryOperation(other, std::minus{});
  }

  std::optional<Value> operator*(Value other) const {
    return numericBinaryOperation(other, std::multiplies{});
  }

  std::optional<Value> operator/(Value other) const {
    return numericBinaryOperation(other, std::divides{});
  }

  std::optional<Value> power(Value other) const {
    return numericBinaryOperation(other, [](double base, double exponent) {
      return std::pow(base, exponent);
    });
  }

  std::optional<Value> operator%(Value other) const {
    return numericBinaryOperation(other, [](double left, double right) {
      return std::fmod(left, right);
    });
  }

  std::optional<Value> operator<(Value other) const {
    return numericBinaryOperation(other, std::less{});
  }

  std::optional<Value> operator<=(Value other) const {
    return numericBinaryOperation(other, std::less_equal{});
  }

  std::optional<Value> operator>(Value other) const {
    return numericBinaryOperation(other, std::greater{});
  }

  std::optional<Value> operator>=(Value other) const {
    return numericBinaryOperation(other, std::greater_equal{});
  }

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

template <> struct std::formatter<Value> {
  auto parse(std::format_parse_context &context) { return context.begin(); }

  auto format(Value value, std::format_context &context) const {
    return std::format_to(context.out(), "{}", value.toString());
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
  std::pmr::vector<std::size_t> jumps{};
  RegisterIndex registerCount{};

  Function(std::pmr::memory_resource &allocator)
      : instructions(&allocator), constants(&allocator), upvalues(&allocator),
        jumps(&allocator) {}
};
