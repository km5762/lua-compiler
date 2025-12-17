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
  if (check(Token::Type::Local)) {
    m_scanner.get().advance();
    std::vector<std::string_view> nameList{parseNameList()};
    std::vector<AstNode::Ptr> expList{};

    if (check(Token::Type::Assign)) {
      m_scanner.get().advance();
      expList = parseExpList();
    }

    return std::make_unique<AstNode>(
        AstNode::LocalDeclaration{nameList, std::move(expList)});
  } else {
    throw UnexpectedToken{m_scanner.get().peek().type, Token::Type::Local};
  }

  return nullptr;
}

AstNode::Ptr Parser::parseLastStat() {
  m_scanner.get().advance();
  return std::make_unique<AstNode>(AstNode::Return{parseExpList()});
}

std::vector<std::string_view> Parser::parseNameList() {
  std::vector<std::string_view> nameList{consume(Token::Type::Name).data};

  while (check(Token::Type::Comma)) {
    m_scanner.get().advance();
    nameList.emplace_back(consume(Token::Type::Name).data);
  }

  return nameList;
}

std::vector<AstNode::Ptr> Parser::parseExpList() {
  std::vector<AstNode::Ptr> expList{};
  expList.emplace_back(parseExp());

  while (check(Token::Type::Comma)) {
    m_scanner.get().advance();
    expList.emplace_back(parseExp());
  }

  return expList;
}

AstNode::Ptr Parser::parseExp() { return parseAdditive(); }

AstNode::Ptr Parser::parseAdditive() {
  AstNode::Ptr left{parsePrimary()};

  while (check(Token::Type::Minus)) {
    Token token{m_scanner.get().advance()};
    AstNode::Ptr right{parsePrimary()};
    left = std::make_unique<AstNode>(
        AstNode::BinaryOperator{token, {std::move(left), std::move(right)}});
  }

  return left;
}

AstNode::Ptr Parser::parsePrimary() {
  if (check(Token::Type::LeftParenthesis)) {
    m_scanner.get().advance();
    AstNode::Ptr exp{parseExp()};
    consume(Token::Type::RightParenthesis);
    return exp;

  } else if (check(Token::Type::Number)) {
    Token token{m_scanner.get().advance()};
    double number{};
    auto [ptr, ec] = std::from_chars(
        token.data.data(), token.data.data() + token.data.size(), number);
    if (ptr != token.data.end() || ec != std::errc{}) {
      throw MalformedNumber{token.data};
    }
    return std::make_unique<AstNode>(AstNode::Number{number});
  } else if (check(Token::Type::Name)) {
    Token token{m_scanner.get().advance()};
    return std::make_unique<AstNode>(AstNode::Name{token.data});
  }
  return nullptr;
}
