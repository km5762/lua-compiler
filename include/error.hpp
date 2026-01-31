#pragma once

#include "token.hpp"
#include <expected>
#include <format>
#include <system_error>

struct Error {
  std::error_code code{};
  std::string message{};
};

template <typename T> using Result = std::expected<T, Error>;

inline std::string makeErrorMessage(std::size_t line, std::size_t position,
                                    std::string_view message) {
  return std::format("{}:{}: {}", line, position, message);
}

inline std::string makeErrorMessage(const Token &token,
                                    std::string_view message) {
  return makeErrorMessage(token.line, token.position, message);
}
