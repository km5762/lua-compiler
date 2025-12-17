#include "scanner.hpp"

#include <cctype>

void Scanner::skipWhitespace() {
  while (std::isspace(static_cast<unsigned char>(peekChar()))) {
    advanceChar();
  }
}

Token Scanner::advance() {
  skipWhitespace();

  m_start = m_current;
  char c{advanceChar()};
  if (c == '_' || std::isalpha(static_cast<unsigned char>(c))) {
    std::string_view word{scanWord()};

    if (word == "return") {
      return Token{Token::Type::Return};
    } else if (word == "int") {
      return Token{Token::Type::Number};
    } else if (word == "local") {
      return Token{Token::Type::Local};
    } else {
      return Token{Token::Type::Name, word};
    }
  } else if (std::isdigit(static_cast<unsigned char>(c))) {
    return scanNumber();
  }

  switch (c) {
  case '(':
    return Token{Token::Type::LeftParenthesis};
  case ')':
    return Token{Token::Type::RightParenthesis};
  case '{':
    return Token{Token::Type::LeftBrace};
  case '}':
    return Token{Token::Type::RightBrace};
  case ',':
    return Token{Token::Type::Comma};
  case '-':
    return Token{Token::Type::Minus};
  case '=':
    return Token{Token::Type::Assign};
  case '\0':
    return Token{Token::Type::Eof};
  default:
    return Token{Token::Type::Invalid};
  }
}

std::string_view Scanner::scanWord() {
  while (true) {
    char c = peekChar();
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
      break;
    }

    advanceChar();
  }

  return std::string_view{&m_text[m_start], m_current - m_start};
}

Token Scanner::scanNumber() {
  while (true) {
    char c = peekChar();

    if (!std::isdigit(static_cast<unsigned char>(c)) && c != '.') {
      break;
    }

    advanceChar();
  }

  return {Token::Type::Number,
          std::string_view{&m_text[m_start], m_current - m_start}};
}
