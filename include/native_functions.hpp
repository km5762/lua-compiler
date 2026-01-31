#pragma once

#include "value.hpp"
#include <cstdio>

namespace native {
Value print(std::span<Value> arguments);
Value error(std::span<Value> arguments);
Value lua_assert(std::span<Value> arguments);
} // namespace native
