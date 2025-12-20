#pragma once

#include <ostream>
#include <string_view>

struct Token {
public:
  enum class Type {
    Invalid,
    Name,
    Local,
    Return,
    Break,
    Number,
    String,
    False,
    True,
    Assign,
    Or,
    And,
    LessThan,
    GreaterThan,
    LessThanOrEqual,
    GreaterThanOrEqual,
    NotEqual,
    Equal,
    Concatenate,
    Plus,
    Minus,
    Times,
    Divide,
    Modulo,
    Not,
    Length,
    Power,
    Dot,
    LeftParenthesis,
    RightParenthesis,
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    Comma,
    Colon,
    Semicolon,
    Eof,
  };

  static std::string_view toString(Token::Type type) {
    switch (type) {
    case Type::Invalid:
      return "Invalid";
    case Type::Name:
      return "Name";
    case Type::Local:
      return "Local";
    case Type::Return:
      return "Return";
    case Type::Break:
      return "Break";
    case Type::Number:
      return "Number";
    case Type::Assign:
      return "Assign";
    case Type::Or:
      return "Or";
    case Type::And:
      return "And";
    case Type::LessThan:
      return "LessThan";
    case Type::GreaterThan:
      return "GreaterThan";
    case Type::LessThanOrEqual:
      return "LessThanOrEqual";
    case Type::GreaterThanOrEqual:
      return "GreaterThanOrEqaul";
    case Type::NotEqual:
      return "NotEqual";
    case Type::Equal:
      return "Equal";
    case Type::Concatenate:
      return "Concatenate";
    case Type::Plus:
      return "Plus";
    case Type::Minus:
      return "Minus";
    case Type::Times:
      return "Times";
    case Type::Divide:
      return "Divide";
    case Type::Modulo:
      return "Modulo";
    case Type::Not:
      return "Not";
    case Type::Length:
      return "Length";
    case Type::Power:
      return "Power";
    case Type::Dot:
      return "Dot";
    case Type::LeftParenthesis:
      return "LeftParenthesis";
    case Type::RightParenthesis:
      return "RightParenthesis";
    case Type::LeftBrace:
      return "LeftBrace";
    case Type::RightBrace:
      return "RightBrace";
    case Type::LeftBracket:
      return "LeftBracket";
    case Type::RightBracket:
      return "RightBracket";
    case Type::Comma:
      return "Comma";
    case Type::Colon:
      return "Colon";
    case Type::Semicolon:
      return "Semicolon";
    case Type::Eof:
      return "Eof";
    }
  }

  bool operator==(const Token &other) {
    return other.type == type && other.data == data;
  };

  friend std::ostream &operator<<(std::ostream &stream, const Token &token) {
    stream << "[" << toString(token.type);
    if (!token.data.empty()) {
      stream << ": '" << token.data << "'";
    }
    stream << "]";
    return stream;
  }

  Type type{};
  std::string_view data{};
  std::size_t line{};
  std::size_t position{};
};
