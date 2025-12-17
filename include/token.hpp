#include <ostream>
#include <string_view>

struct Token {
public:
  enum class Type {
    Invalid,
    Name,
    Local,
    Return,
    Number,
    Assign,
    Minus,
    LeftParenthesis,
    RightParenthesis,
    LeftBrace,
    RightBrace,
    Comma,
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
    case Token::Type::Number:
      return "Number";
    case Token::Type::Assign:
      return "Assign";
    case Token::Type::Minus:
      return "Minus";
    case Token::Type::LeftParenthesis:
      return "LeftParenthesis";
    case Token::Type::RightParenthesis:
      return "RightParenthesis";
    case Token::Type::LeftBrace:
      return "LeftBrace";
    case Token::Type::RightBrace:
      return "RightBrace";
    case Token::Type::Comma:
      return "Comma";
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
};
