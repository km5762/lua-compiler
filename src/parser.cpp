#include "parser.hpp"
#include "scanner.hpp"

#include <charconv>
#include <iostream>
#include <memory>

AstNode::Ptr Parser::parse(Scanner &scanner) {
  Parser parser{scanner};
  try {
    return parser.parseChunk();
  } catch (const UnexpectedToken &error) {
    std::cerr << error.what() << std::endl;
    return nullptr;
  }
}

AstNode::Ptr Parser::parseBlock() {
  return std::make_unique<AstNode>(AstNode::Block{parseChunk()});
}

AstNode::Ptr Parser::parseChunk() {
  std::vector<AstNode::Ptr> statements{};
  AstNode::Ptr lastStatement{};

  while (!eof() && !check(Token::Type::Return)) {
    statements.emplace_back(parseStat());
  }

  if (check(Token::Type::Return)) {
    lastStatement = parseLastStat();
  }

  return std::make_unique<AstNode>(
      AstNode::Chunk{std::move(statements), std::move(lastStatement)});
}

AstNode::Ptr Parser::parseStat() {
  if (match(Token::Type::Local)) {
    std::vector<std::string_view> nameList{parseNameList()};
    std::vector<AstNode::Ptr> expList{};

    if (check(Token::Type::Assign)) {
      m_scanner.get().advance();
      expList = parseExpList();
    }

    return std::make_unique<AstNode>(
        AstNode::LocalDeclaration{nameList, std::move(expList)});
  } else {
    std::vector<AstNode::Ptr> varList{parseVarList()};
    consume(Token::Type::Assign);
    std::vector<AstNode::Ptr> expList{parseExpList()};
    return std::make_unique<AstNode>(
        AstNode::Assignment{std::move(varList), std::move(expList)});
  }

  return nullptr;
}

AstNode::Ptr Parser::parseLastStat() {
  m_scanner.get().advance();
  return std::make_unique<AstNode>(AstNode::Return{parseExpList()});
}

AstNode::Ptr Parser::parseVar() {
  AstNode::Ptr prefix{parsePrimary()};
  while (true) {
    if (check(Token::Type::Dot)) {
      Token token{m_scanner.get().advance()};
      return std::make_unique<AstNode>(AstNode::Access{token.data});
    } else {
      break;
    }
  }
  return prefix;
}

std::vector<AstNode::Ptr> Parser::parseVarList() {
  return parseList<&Parser::parseVar>();
}

std::vector<std::string_view> Parser::parseNameList() {
  return parseList<&Parser::parseName>();
}

std::vector<AstNode::Ptr> Parser::parseExpList() {
  return parseList<&Parser::parseExp>();
}

AstNode::Ptr Parser::parseExp() { return parseOr(); }

AstNode::Ptr Parser::parseOr() {
  return parseLeftBinaryOperation<&Parser::parseAnd>(Token::Type::Or);
}

AstNode::Ptr Parser::parseAnd() {
  return parseLeftBinaryOperation<&Parser::parseComparison>(Token::Type::And);
}

AstNode::Ptr Parser::parseComparison() {
  return parseLeftBinaryOperation<&Parser::parseConcatenation>(
      Token::Type::LessThan, Token::Type::LessThanOrEqual,
      Token::Type::GreaterThan, Token::Type::GreaterThanOrEqual,
      Token::Type::NotEqual, Token::Type::Equal);
}

AstNode::Ptr Parser::parseConcatenation() {
  return parseRightBinaryOperation<&Parser::parseAdditive>(
      Token::Type::Concatenate);
}

AstNode::Ptr Parser::parseAdditive() {
  return parseLeftBinaryOperation<&Parser::parseMultiplicative>(
      Token::Type::Plus, Token::Type::Minus);
}

AstNode::Ptr Parser::parseMultiplicative() {
  return parseLeftBinaryOperation<&Parser::parseUnary>(
      Token::Type::Times, Token::Type::Divide, Token::Type::Modulo);
}

AstNode::Ptr Parser::parseUnary() {
  if (check(Token::Type::Not, Token::Type::Length, Token::Type::Minus)) {
    Token token{m_scanner.get().advance()};
    return std::make_unique<AstNode>(
        AstNode::UnaryOperator{token, parseUnary()});
  }
  return parsePower();
}

AstNode::Ptr Parser::parsePower() {
  return parseRightBinaryOperation<&Parser::parsePrimary>(Token::Type::Power);
}

AstNode::Ptr Parser::parsePrimary() {
  if (match(Token::Type::LeftParenthesis)) {
    AstNode::Ptr exp{parseExp()};
    consume(Token::Type::RightParenthesis);
    return exp;

  } else if (check(Token::Type::Number)) {
    Token token{m_scanner.get().advance()};
    double number{};
    auto [ptr, ec] = std::from_chars(
        token.data.data(), token.data.data() + token.data.size(), number);
    if (ptr != token.data.end() || ec != std::errc{}) {
      throw MalformedNumber{token};
    }
    return std::make_unique<AstNode>(AstNode::Number{number});
  } else if (check(Token::Type::Name)) {
    Token token{m_scanner.get().advance()};
    return std::make_unique<AstNode>(AstNode::Name{token.data});
  }
  throw UnexpectedToken{m_scanner.get().peek(), Token::Type::LeftParenthesis,
                        Token::Type::Name};
  return nullptr;
}
