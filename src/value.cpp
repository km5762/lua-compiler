#include "value.hpp"
#include <cassert>
#include <utility>

Value Value::operator==(Value other) const {
  if (type != other.type) {
    return false;
  }

  switch (type) {
  case Type::Nil:
    return true;
  case Type::Number:
    return data.number == other.data.number;
  case Type::String:
    return toStringView() == other.toStringView();
  case Type::Function:
    return data.function == other.data.function;
  case Type::NativeFunction:
    return data.nativeFunction == other.data.nativeFunction;
  case Type::Boolean:
    return data.boolean == other.data.boolean;
  case Type::Table:
    return data.table == other.data.table;
  case Type::Closure:
    return data.closure == other.data.closure;
  }
}

Value Value::operator!=(Value other) const {
  const std::optional<Value> result{*this == other};
  return !result->data.boolean;
}

std::string Value::toString() const {
  switch (type) {
  case Type::Nil:
    return "nil";
  case Type::Number:
    return std::to_string(data.number);
  case Type::String:
    return std::string{*toStringView()};
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
  case Type::Table:
    return "table: " + std::to_string(reinterpret_cast<uintptr_t>(data.table));
    break;
  case Type::Closure:
    return "closure: " +
           std::to_string(reinterpret_cast<uintptr_t>(data.closure));
    break;
  }
}

std::optional<std::string_view> Value::toStringView() const {
  if (type != Type::String) {
    return std::nullopt;
  }

  const StringSize length{*data.string};
  const char *string{reinterpret_cast<const char *>(data.string + 1)};
  return std::string_view{string, length};
}

std::size_t Value::hash() const {
  switch (type) {
  case Type::Nil:
    assert(false && "attempt to hash a nil");
    std::unreachable();
  case Type::Number: {
    std::hash<double> hasher{};
    return hasher(data.number);
  }
  case Type::String: {
    std::hash<std::string_view> hasher{};
    return hasher(*toStringView());
  }
  case Type::Function: {
    std::hash<Function *> hasher{};
    return hasher(data.function);
  }
  case Type::NativeFunction: {
    std::hash<NativeFunction> hasher{};
    return hasher(data.nativeFunction);
  }
  case Type::Boolean: {
    std::hash<bool> hasher{};
    return hasher(data.boolean);
  }
  case Type::Table: {
    std::hash<Table *> hasher{};
    return hasher(data.table);
  }
  case Type::Closure:
    std::hash<Closure *> hasher{};
    return hasher(data.closure);
    break;
  }
}
