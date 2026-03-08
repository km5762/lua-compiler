#pragma once

#include "error.hpp"
#include <string_view>

namespace compilation {
std::optional<Error> run(std::string_view program);
std::optional<Error> disassemble(std::string_view program);
} // namespace compilation
