#pragma once

#include "token.hpp"

#include <string_view>

class Scanner {
public:
  explicit Scanner(std::string_view text) : m_text{text} {};
  Token advance();
  Token peek() {
    std::size_t startCursor{m_cursor};
    std::size_t startLine{m_line};
    std::size_t startPosition{m_position};
    Token token{advance()};
    m_cursor = startCursor;
    m_line = startLine;
    m_position = startPosition;
    return token;
  };

private:
  char advanceChar() {
    if (eof()) {
      return '\0';
    }
    char c = m_text[m_cursor++];
    if (c == '\n') {
      ++m_line;
      m_position = 1;
    } else {
      ++m_position;
    }
    return c;
  }
  char peekChar() { return eof() ? '\0' : m_text[m_cursor]; }
  bool eof() { return m_cursor >= m_text.size(); }
  void skipWhitespace();

  std::string_view scanWord();
  Token scanNumber();
  Token makeToken(Token::Type type) {
    return Token{type,
                 {&m_text[m_startCursor], m_cursor - m_startCursor},
                 m_startLine,
                 m_startPosition};
  }
  Token makeToken(Token::Type type, std::string_view data) {
    return Token{type,
                 {&m_text[m_startCursor], m_cursor - m_startCursor},
                 m_startLine,
                 m_startPosition};
  }

  std::string_view m_text{};
  std::size_t m_cursor{};
  std::size_t m_line{1};
  std::size_t m_position{1};
  std::size_t m_startCursor{};
  std::size_t m_startLine{1};
  std::size_t m_startPosition{1};
};
