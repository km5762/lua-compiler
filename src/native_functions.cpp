#include "native_functions.hpp"
#include "value.hpp"
#include <iostream>

namespace native {
Value print(std::span<Value> arguments) {
  for (const Value &argument : arguments) {
    const auto string{argument.toString()};
    std::cout << string;
    // fwrite(string.data(), 1, string.size(), stdout);
  }

  return {};
}

Value error(std::span<Value> arguments) {
  std::cerr << arguments.front().toString() << '\n';
  exit(EXIT_FAILURE);
}

Value lua_assert(std::span<Value> arguments) {
  const Value &condition{arguments.front()};

  if (condition.type == Value::Type::Boolean && !condition.data.boolean) {
    if (arguments.size() < 2) {
      exit(EXIT_FAILURE);
    }
    error(arguments.subspan(1));
  }

  return {};
}
} // namespace native
