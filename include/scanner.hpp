#pragma once

#include "error.hpp"
#include "token.hpp"

#include <cstring>
#include <expected>
#include <string_view>
#include <system_error>

enum class ScannerErrorCode { UnterminatedStringLiteral };
class ScannerErrorCategory : public std::error_category {
  const char *name() const noexcept override { return "scanner"; }

  std::string message(int ev) const override {
    switch (static_cast<ScannerErrorCode>(ev)) {
    case ScannerErrorCode::UnterminatedStringLiteral:
      return "unterminated string literal";
    default:
      return "unknown scanner error";
    }
  }
};
inline const std::error_category &scannerCategory() {
  static ScannerErrorCategory instance;
  return instance;
}
inline std::error_code make_error_code(ScannerErrorCode e) noexcept {
  return {static_cast<int>(e), scannerCategory()};
}
namespace std {
template <> struct is_error_code_enum<ScannerErrorCode> : true_type {};
} // namespace std

class Scanner {
public:
  explicit Scanner(std::string_view text, std::pmr::memory_resource &allocator)
      : m_text{text}, m_allocator{allocator} {};
  Result<Token> advance();
  Result<Token> peek() {
    std::size_t startCursor{m_cursor};
    std::size_t startLine{m_line};
    std::size_t startPosition{m_position};
    Result<Token> token{advance()};
    if (!token) {
      return std::unexpected{token.error()};
    }
    m_cursor = startCursor;
    m_line = startLine;
    m_position = startPosition;
    return token;
  };

private:
  std::reference_wrapper<std::pmr::memory_resource> m_allocator;
  std::string_view m_text{};
  std::size_t m_cursor{};
  std::size_t m_line{1};
  std::size_t m_position{1};
  std::size_t m_startCursor{};
  std::size_t m_startLine{1};
  std::size_t m_startPosition{1};

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
  Result<Token> scanNumber();
  Result<Token> scanString();
  void scanComment();
  Result<Token> makeToken(Token::Type type) {
    std::size_t size{m_cursor - m_startCursor};
    char *string{
        static_cast<char *>(m_allocator.get().allocate(size, alignof(char)))};
    std::memcpy(string, &m_text[m_startCursor], size);

    return Token{type, {string, size}, m_startLine, m_startPosition};
  }
  template <typename T> Result<Token> makeToken(Token::Type type, T &&data) {
    return Token{type, data, m_startLine, m_startPosition};
  }
  Result<std::string_view> unescapeString(std::string_view string);
  Error makeError(std::error_code code) {
    return {code,
            makeErrorMessage(m_startLine, m_startPosition, code.message())};
  }
};
