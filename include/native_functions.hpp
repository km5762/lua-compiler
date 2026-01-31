#pragma once

#include "value.hpp"
#include <cstdio>
#include <iostream>

Value print(std::span<Value> arguments) {
  for (const Value &argument : arguments) {
    std::string string{argument.toString()};
    std::cout << string;
    // fwrite(string.data(), 1, string.size(), stdout);
  }

  return {};
}

Value error(std::span<Value> arguments) {
  std::cout << arguments[0].toString();
  exit(EXIT_FAILURE);
}
