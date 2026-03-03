#pragma once

#include <cmath>
#include <cstdint>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

struct Value;
struct VirtualMachine;

using RegisterIndex = uint64_t;
using JumpIndex = uint64_t;
using StringSize = std::size_t;
using NativeFunction = Value (*)(std::span<Value> arguments);

struct Function;
struct Table;

struct Value {
  enum class Type {
    Nil,
    Number,
    String,
    Function,
    NativeFunction,
    Boolean,
    Table,
  };

  Type type{};
  union {
    double number;
    StringSize *string;
    Function *function;
    NativeFunction nativeFunction;
    bool boolean;
    Table *table;
  } data;

  Value() : type{Type::Nil} {};
  Value(double number) : type{Type::Number} { data.number = number; }
  Value(StringSize *string) : type{Type::String} { data.string = string; }
  Value(Function *function) : type{Type::Function} { data.function = function; }
  Value(NativeFunction nativeFunction) : type{Type::NativeFunction} {
    data.nativeFunction = nativeFunction;
  }
  Value(bool boolean) : type{Type::Boolean} { data.boolean = boolean; }
  Value(Table *table) : type{Type::Table} { data.table = table; }

  explicit operator bool() const {
    return !(type == Type::Nil || (type == Type::Boolean && !data.boolean));
  }

  Value operator==(Value other) const;
  Value operator!=(Value other) const;

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

  std::string toString() const;
  std::optional<std::string_view> toStringView() const;
  std::size_t hash() const;
};

namespace std {
template <> struct hash<Value> {
  size_t operator()(const Value &value) const noexcept { return value.hash(); }
};
} // namespace std

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

struct ValueComparator {
  bool operator()(const Value &left, const Value &right) const noexcept {
    const Value result(left == right);
    return result.data.boolean;
  }
};

class Table {
public:
  Value &get(Value field) { return m_map[field]; }
  void set(Value field, Value value) { m_map[field] = value; }

private:
  std::unordered_map<Value, Value, std::hash<Value>, ValueComparator> m_map{};
};
