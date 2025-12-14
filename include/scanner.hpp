#pragma once

#include <ostream>
#include <string_view>

class Token {
public:
  enum class Type {
    Int,
    Return,
    Identifier,
    NumberLiteral,
    Assign,
    Minus,
    LeftParenthesis,
    RightParenthesis,
    LeftBrace,
    RightBrace,
    Semicolon,
    Eof,
    Invalid,
  };

  Token(Type type, std::string_view data) : type_{type}, data_{data} {}
  explicit Token(Type type) : type_{type} {}

  friend std::ostream &operator<<(std::ostream &stream, const Token &token) {
    stream << "[" << token.to_string(token.type_);
    if (!token.data_.empty()) {
      stream << ": '" << token.data_ << "'";
    }
    stream << "]";
    return stream;
  }

  Type type() { return type_; };
  std::string_view data() { return data_; };

private:
  Type type_{};
  std::string_view data_{};

  static std::string_view to_string(Type type) {
    switch (type) {
    case Type::Int:
      return "Int";
    case Type::Return:
      return "Return";
    case Type::Identifier:
      return "Identifier";
    case Type::NumberLiteral:
      return "NumberLiteral";
    case Type::Assign:
      return "Assign";
    case Type::Minus:
      return "Minus";
    case Type::LeftParenthesis:
      return "LeftParenthesis";
    case Type::RightParenthesis:
      return "RightParenthesis";
    case Type::LeftBrace:
      return "LeftBrace";
    case Type::RightBrace:
      return "RightBrace";
    case Type::Semicolon:
      return "Semicolon";
    case Type::Eof:
      return "Eof";
    case Type::Invalid:
      return "Invalid";
    default:
      return "Unknown";
    }
  }
};

class Scanner {
public:
  explicit Scanner(std::string_view text) : text_{text} {};
  Token next();

private:
  char advance() { return eof() ? '\0' : text_[current_++]; }
  char peek() { return eof() ? '\0' : text_[current_]; }
  bool eof() { return current_ >= text_.size(); }
  void skip_whitespace();

  std::string_view scan_word();
  Token scan_number();

  std::string_view text_{};
  std::size_t current_{};
  std::size_t start_{};
};
