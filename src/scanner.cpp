#include "scanner.hpp"
#include "token.hpp"

#include <cctype>

void Scanner::skipWhitespace() {
  char c{peekChar()};
  while (std::isspace(static_cast<unsigned char>(c))) {
    advanceChar();
    c = peekChar();
  }
}

Token Scanner::advance() {
  skipWhitespace();

  m_startCursor = m_cursor;
  m_startLine = m_line;
  m_startPosition = m_position;

  char c{advanceChar()};
  if (c == '_' || std::isalpha(static_cast<unsigned char>(c))) {
    std::string_view word{scanWord()};

    if (word == "return") {
      return makeToken(Token::Type::Return);
    } else if (word == "local") {
      return makeToken(Token::Type::Local);
    } else if (word == "or") {
      return makeToken(Token::Type::Or);
    } else if (word == "and") {
      return makeToken(Token::Type::And);
    } else if (word == "not") {
      return makeToken(Token::Type::Not);
    } else if (word == "break") {
      return makeToken(Token::Type::Break);
    } else if (word == "end") {
      return makeToken(Token::Type::End);
    } else if (word == "if") {
      return makeToken(Token::Type::If);
    } else if (word == "elseif") {
      return makeToken(Token::Type::ElseIf);
    } else if (word == "else") {
      return makeToken(Token::Type::Else);
    } else if (word == "while") {
      return makeToken(Token::Type::While);
    } else if (word == "do") {
      return makeToken(Token::Type::Do);
    } else if (word == "repeat") {
      return makeToken(Token::Type::Repeat);
    } else if (word == "until") {
      return makeToken(Token::Type::Until);
    } else if (word == "false") {
      return makeToken(Token::Type::False);
    } else if (word == "true") {
      return makeToken(Token::Type::True);
    } else {
      return makeToken(Token::Type::Name, word);
    }
  } else if (std::isdigit(static_cast<unsigned char>(c))) {
    return scanNumber();
  }

  switch (c) {
  case '(':
    return makeToken(Token::Type::LeftParenthesis);
  case ')':
    return makeToken(Token::Type::RightParenthesis);
  case '{':
    return makeToken(Token::Type::LeftBrace);
  case '}':
    return makeToken(Token::Type::RightBrace);
  case '[':
    return makeToken(Token::Type::LeftBracket);
  case ']':
    return makeToken(Token::Type::RightBracket);
  case ',':
    return makeToken(Token::Type::Comma);
  case ';':
    return makeToken(Token::Type::Semicolon);
  case ':':
    return makeToken(Token::Type::Colon);
  case '+':
    return makeToken(Token::Type::Plus);
  case '-':
    return makeToken(Token::Type::Minus);
  case '*':
    return makeToken(Token::Type::Times);
  case '^':
    return makeToken(Token::Type::Power);
  case '#':
    return makeToken(Token::Type::Length);
  case '/':
    return makeToken(Token::Type::Divide);
  case '%':
    return makeToken(Token::Type::Modulo);
  case '=':
    if (peekChar() == '=') {
      advanceChar();
      return makeToken(Token::Type::Equal);
    }
    return makeToken(Token::Type::Assign);
  case '<':
    if (peekChar() == '=') {
      advanceChar();
      return makeToken(Token::Type::LessThanOrEqual);
    }
    return makeToken(Token::Type::LessThan);
  case '>':
    if (peekChar() == '=') {
      advanceChar();
      return makeToken(Token::Type::GreaterThanOrEqual);
    }
    return makeToken(Token::Type::GreaterThan);
  case '~':
    if (peekChar() == '=') {
      advanceChar();
      return makeToken(Token::Type::NotEqual);
    }
    return makeToken(Token::Type::Invalid);
  case '.':
    if (peekChar() == '.') {
      advanceChar();
      return makeToken(Token::Type::Concatenate);
    }
    return makeToken(Token::Type::Dot);
  case '\0':
    return makeToken(Token::Type::Eof);
  default:
    return makeToken(Token::Type::Invalid);
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

  return std::string_view{&m_text[m_startCursor], m_cursor - m_startCursor};
}

Token Scanner::scanNumber() {
  while (true) {
    char c = peekChar();

    if (!std::isdigit(static_cast<unsigned char>(c)) && c != '.') {
      break;
    }

    advanceChar();
  }

  return makeToken(Token::Type::Number);
}
