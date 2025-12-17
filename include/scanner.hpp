#pragma once

#include "token.hpp"

#include <string_view>

class Scanner {
public:
  explicit Scanner(std::string_view text) : m_text{text} {};
  Token advance();
  Token peek() {
    Token token{advance()};
    m_current = m_start;
    return token;
  };

private:
  Token next();
  char advanceChar() { return eof() ? '\0' : m_text[m_current++]; }
  char peekChar() { return eof() ? '\0' : m_text[m_current]; }
  bool eof() { return m_current >= m_text.size(); }
  void skipWhitespace();

  std::string_view scanWord();
  Token scanNumber();

  std::string_view m_text{};
  std::size_t m_current{};
  std::size_t m_start{};
};
