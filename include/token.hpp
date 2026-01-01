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
    Do,
    While,
    Repeat,
    Then,
    End,
    ElseIf,
    If,
    Else,
    Until,
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
    case Token::Type::Invalid:
      return "Invalid";
    case Token::Type::Name:
      return "Name";
    case Token::Type::Local:
      return "Local";
    case Token::Type::Return:
      return "Return";
    case Token::Type::Break:
      return "Break";
    case Token::Type::Do:
      return "Do";
    case Token::Type::While:
      return "While";
    case Token::Type::Repeat:
      return "Repeat";
    case Token::Type::Then:
      return "Then";
    case Token::Type::End:
      return "End";
    case Token::Type::ElseIf:
      return "ElseIf";
    case Token::Type::If:
      return "If";
    case Token::Type::Else:
      return "Else";
    case Token::Type::Until:
      return "Until";
    case Token::Type::Number:
      return "Number";
    case Token::Type::String:
      return "String";
    case Token::Type::False:
      return "False";
    case Token::Type::True:
      return "True";
    case Token::Type::Assign:
      return "Assign";
    case Token::Type::Or:
      return "Or";
    case Token::Type::And:
      return "And";
    case Token::Type::LessThan:
      return "LessThan";
    case Token::Type::GreaterThan:
      return "GreaterThan";
    case Token::Type::LessThanOrEqual:
      return "LessThanOrEqual";
    case Token::Type::GreaterThanOrEqual:
      return "GreaterThanOrEqual";
    case Token::Type::NotEqual:
      return "NotEqual";
    case Token::Type::Equal:
      return "Equal";
    case Token::Type::Concatenate:
      return "Concatenate";
    case Token::Type::Plus:
      return "Plus";
    case Token::Type::Minus:
      return "Minus";
    case Token::Type::Times:
      return "Times";
    case Token::Type::Divide:
      return "Divide";
    case Token::Type::Modulo:
      return "Modulo";
    case Token::Type::Not:
      return "Not";
    case Token::Type::Length:
      return "Length";
    case Token::Type::Power:
      return "Power";
    case Token::Type::Dot:
      return "Dot";
    case Token::Type::LeftParenthesis:
      return "LeftParenthesis";
    case Token::Type::RightParenthesis:
      return "RightParenthesis";
    case Token::Type::LeftBrace:
      return "LeftBrace";
    case Token::Type::RightBrace:
      return "RightBrace";
    case Token::Type::LeftBracket:
      return "LeftBracket";
    case Token::Type::RightBracket:
      return "RightBracket";
    case Token::Type::Comma:
      return "Comma";
    case Token::Type::Colon:
      return "Colon";
    case Token::Type::Semicolon:
      return "Semicolon";
    case Token::Type::Eof:
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
