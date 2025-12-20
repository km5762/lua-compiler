#include "parser.hpp"
#include "ast_node.hpp"
#include "scanner.hpp"
#include "token.hpp"

#include <charconv>
#include <iostream>
#include <memory_resource>

AstNode::Ptr Parser::parse(Scanner &scanner,
                           std::pmr::memory_resource &allocator) {
  Parser parser{scanner, allocator};
  return parser.parseBlock();
}

AstNode::Ptr Parser::parseBlock() {
  return allocate<AstNode>(AstNode::Block{parseChunk()});
}

AstNode::Ptr Parser::parseChunk() {
  std::vector<AstNode::Ptr> statements{};
  AstNode::Ptr lastStatement{};

  while (!eof() && !check(Token::Type::Return, Token::Type::Break)) {
    try {
      statements.emplace_back(parseStat());
      match(Token::Type::Semicolon);
    } catch (const ParseError &error) {
      std::cerr << error.what() << std::endl;
      return allocate<AstNode>(AstNode::Chunk{statements});
    }
  }

  AstNode::Ptr lastStat{parseLastStat()};
  match(Token::Type::Semicolon);
  return allocate<AstNode>(AstNode::Chunk{statements, lastStat});
}

AstNode::Ptr Parser::parseStat() {
  if (match(Token::Type::Local)) {
    std::vector<std::string_view> nameList{parseNameList()};
    std::vector<AstNode::Ptr> expList{};

    if (check(Token::Type::Assign)) {
      m_scanner.get().advance();
      expList = parseExpList();
    }

    return allocate<AstNode>(AstNode::LocalDeclaration{nameList, expList});
  }

  AstNode::Ptr prefix{parsePrefixExp()};
  if (match(Token::Type::Comma)) {
    std::vector<AstNode::Ptr> varList{parseVarList(prefix)};
    consume(Token::Type::Assign);
    std::vector<AstNode::Ptr> expList{parseExpList()};
    prefix = allocate<AstNode>(AstNode::Assignment{varList, expList});
  } else if (match(Token::Type::Assign)) {
    std::vector<AstNode::Ptr> vars{};
    vars.push_back(prefix);
    std::vector<AstNode::Ptr> values{};
    values.push_back(parseExp());
    prefix = allocate<AstNode>(AstNode::Assignment{vars, values});
  }

  return prefix;
}

AstNode::Ptr Parser::parseLastStat() {
  if (match(Token::Type::Return)) {
    return allocate<AstNode>(AstNode::Return{parseExpList()});
  }
  consume(Token::Type::Break);
  m_scanner.get().advance();
  return allocate<AstNode>(AstNode::Break{});
}

std::vector<AstNode::Ptr>
Parser::parseVarList(std::optional<AstNode::Ptr> initial) {
  return parseList<&Parser::parsePrefixExp>(initial);
}

std::vector<std::string_view> Parser::parseNameList() {
  return parseList<&Parser::parseName>();
}

std::vector<AstNode::Ptr>
Parser::parseExpList(std::optional<AstNode::Ptr> initial) {
  return parseList<&Parser::parseExp>(initial);
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
    return allocate<AstNode>(AstNode::UnaryOperator{token, parseUnary()});
  }
  return parsePower();
}

AstNode::Ptr Parser::parsePower() {
  return parseRightBinaryOperation<&Parser::parsePrimary>(Token::Type::Power);
}

AstNode::Ptr Parser::parsePrimary() {
  if (check(Token::Type::Number)) {
    Token token{m_scanner.get().advance()};
    double number{};
    auto [ptr, ec] = std::from_chars(
        token.data.data(), token.data.data() + token.data.size(), number);
    if (ptr != token.data.end() || ec != std::errc{}) {
      throw MalformedNumber{token};
    }
    return allocate<AstNode>(AstNode::Number{number});
  } else {
    return parsePrefixExp();
  }
  throw UnexpectedToken{m_scanner.get().peek(), Token::Type::LeftParenthesis,
                        Token::Type::Name};
  return nullptr;
}

AstNode::Ptr Parser::parsePrefixExp() {
  AstNode::Ptr prefix{};
  if (match(Token::Type::LeftParenthesis)) {
    prefix = parseExp();
    consume(Token::Type::RightParenthesis);
  } else if (check(Token::Type::Name)) {
    Token token{m_scanner.get().advance()};
    prefix = allocate<AstNode>(AstNode::Name{token.data});
  } else {
    throw UnexpectedToken{m_scanner.get().peek(), Token::Type::LeftParenthesis,
                          Token::Type::Name};
  }

  while (true) {
    if (match(Token::Type::LeftBracket)) {
      prefix = allocate<AstNode>(AstNode::Subscript{prefix, parseExp()});
      consume(Token::Type::RightBracket);
    } else if (match(Token::Type::Dot)) {
      Token token{consume(Token::Type::Name)};
      prefix = allocate<AstNode>(AstNode::Access{prefix, token.data});
    } else if (match(Token::Type::LeftParenthesis)) {
      prefix =
          allocate<AstNode>(AstNode::FunctionCall{prefix, parseArguments()});
    } else if (match(Token::Type::Colon)) {
      Token token{consume(Token::Type::Name)};
      prefix = allocate<AstNode>(AstNode::Access{prefix, token.data});
      consume(Token::Type::LeftParenthesis);
      prefix =
          allocate<AstNode>(AstNode::FunctionCall{prefix, parseArguments()});
    } else {
      break;
    }
  }

  return prefix;
}

std::vector<AstNode::Ptr> Parser::parseArguments() {
  std::vector<AstNode::Ptr> arguments{};

  if (!match(Token::Type::RightParenthesis)) {
    arguments = parseExpList();
    consume(Token::Type::RightParenthesis);
  }

  return arguments;
}
