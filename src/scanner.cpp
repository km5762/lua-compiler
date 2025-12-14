#include "scanner.hpp"
#include <cctype>

void Scanner::skip_whitespace() {
  while (std::isspace(static_cast<unsigned char>(peek()))) {
    advance();
  }
}

Token Scanner::next() {
  skip_whitespace();

  start_ = current_;
  char c{advance()};
  if (c == '_' || std::isalpha(static_cast<unsigned char>(c))) {
    std::string_view word{scan_word()};

    if (word == "return") {
      return Token{Token::Type::Return};
    } else if (word == "int") {
      return Token{Token::Type::Int};
    } else {
      return Token{Token::Type::Identifier, word};
    }
  } else if (std::isdigit(static_cast<unsigned char>(c))) {
    return scan_number();
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
  case ';':
    return Token{Token::Type::Semicolon};
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

std::string_view Scanner::scan_word() {
  while (true) {
    char c = peek();
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
      break;
    }

    advance();
  }

  return std::string_view{&text_[start_], current_ - start_};
}

Token Scanner::scan_number() {
  while (true) {
    char c = peek();

    if (!std::isdigit(static_cast<unsigned char>(c)) && c != '.') {
      break;
    }

    advance();
  }

  return {Token::Type::NumberLiteral,
          std::string_view{&text_[start_], current_ - start_}};
}
