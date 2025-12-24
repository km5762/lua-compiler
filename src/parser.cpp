#include "parser.hpp"

#include <charconv>
#include <iostream>
#include <memory_resource>
#include <string_view>
#include <vector>

#include "ast_node.hpp"
#include "scanner.hpp"
#include "token.hpp"

AstNode *Parser::parse(Scanner &scanner, std::pmr::memory_resource &allocator) {
  Parser parser{scanner, allocator};
  return parser.parseBlock();
}

AstNode *Parser::parseBlock() { return makeNode(AstNode::Block{parseChunk()}); }

AstNode *Parser::parseChunk() {
  AstNode::List<> statements{makeList()};
  AstNode *lastStatement{};

  while (!eof() && !check(Token::Type::Return, Token::Type::Break)) {
    try {
      statements.push_back(parseStat());
      match(Token::Type::Semicolon);
    } catch (const ParseError &error) {
      std::cerr << error.what() << std::endl;
      return makeNode(AstNode::Chunk{statements});
    }
  }

  AstNode *lastStat{parseLastStat()};
  match(Token::Type::Semicolon);
  return makeNode(AstNode::Chunk{statements, lastStat});
}

AstNode *Parser::parseStat() {
  Token token{m_scanner.get().advance()};
  switch (token.type) {
    case Token::Type::Local: {
      AstNode::List<std::string_view> nameList{parseNameList()};
      AstNode::List<> expList{};

      if (match(Token::Type::Assign)) {
        expList = parseExpList();
      }

      return makeNode(AstNode::LocalDeclaration{nameList, expList});
    }
    default: {
      AstNode *prefix{parsePrefixExpression(token)};

      if (match(Token::Type::Assign)) {
        prefix = makeNode(AstNode::Assignment{{prefix}, {parseExp()}});
      } else if (match(Token::Type::Comma)) {
        AstNode::List<> vars{parseVarList(prefix)};
        consume(Token::Type::Assign);
        prefix = makeNode(AstNode::Assignment{vars, parseExpList()});
      }

      return prefix;
    }
  }
}

AstNode *Parser::parseLastStat() {
  if (match(Token::Type::Return)) {
    return makeNode(AstNode::Return{parseExpList()});
  }
  consume(Token::Type::Break);
  return makeNode(AstNode::Break{});
}

int Parser::getPrecedence(Token::Type type) {
  switch (type) {
    case Token::Type::Or:
      return 1;

    case Token::Type::And:
      return 2;

    case Token::Type::LessThan:
    case Token::Type::GreaterThan:
    case Token::Type::LessThanOrEqual:
    case Token::Type::GreaterThanOrEqual:
    case Token::Type::NotEqual:
    case Token::Type::Equal:
      return 3;

    case Token::Type::Concatenate:
      return 4;

    case Token::Type::Plus:
    case Token::Type::Minus:
      return 5;

    case Token::Type::Times:
    case Token::Type::Divide:
    case Token::Type::Modulo:
      return 6;

    case Token::Type::Not:
    case Token::Type::Length:
      return 7;

    case Token::Type::Power:
      return 8;

    default:
      return 0;
  }
}

AstNode *Parser::parseExp(int precedence) {
  Token token{m_scanner.get().advance()};
  AstNode *left{parseNud(token)};

  while (getPrecedence(m_scanner.get().peek().type) > precedence) {
    left = parseLed(m_scanner.get().advance(), left);
  }

  return left;
}

AstNode *Parser::parseNud(Token token) {
  switch (token.type) {
    case Token::Type::Number:
      return parseNumber(token);
    case Token::Type::Minus:
    case Token::Type::Not:
    case Token::Type::Length:
      return makeNode(AstNode::UnaryOperator{
          token.type, parseExp(getPrecedence(token.type))});
    default:
      return parsePrefixExpression(token);
  }
}

AstNode *Parser::parsePrefixExpression(Token token) {
  AstNode *left{};
  switch (token.type) {
    case Token::Type::Name:
      left = makeNode(AstNode::Name{token.data});
      break;
    case Token::Type::LeftParenthesis:
      left = parseExp(getPrecedence(token.type));
      consume(Token::Type::RightParenthesis);
      break;
    default:
      throw UnexpectedToken{token, Token::Type::Name,
                            Token::Type::LeftParenthesis};
  }

  while (true) {
    Token token{m_scanner.get().peek()};
    switch (token.type) {
      case Token::Type::LeftBracket: {
        m_scanner.get().advance();
        left = makeNode(
            AstNode::Subscript{left, parseExp(getPrecedence(token.type))});
        consume(Token::Type::RightBracket);
        break;
      }
      case Token::Type::Dot: {
        m_scanner.get().advance();
        left = makeNode(AstNode::Access{left, consume(Token::Type::Name).data});
        break;
      }
      case Token::Type::LeftParenthesis: {
        m_scanner.get().advance();
        left = makeNode(AstNode::FunctionCall{left, parseArguments()});
        break;
      }
      case Token::Type::Colon: {
        m_scanner.get().advance();
        AstNode *operand = left;
        left = makeNode(AstNode::Access{left, consume(Token::Type::Name).data});
        consume(Token::Type::LeftParenthesis);
        left =
            makeNode(AstNode::FunctionCall{operand, parseArguments(operand)});
        break;
      }
      default:
        return left;
    }
  }

  return left;
}

AstNode *Parser::parseNumber(Token token) {
  double number{};
  auto [ptr, ec] = std::from_chars(
      token.data.data(), token.data.data() + token.data.size(), number);
  if (ptr != token.data.end() || ec != std::errc{}) {
    throw MalformedNumber{token};
  }
  return makeNode(AstNode::Number{number});
}

AstNode *Parser::parseLed(Token token, AstNode *left) {
  switch (token.type) {
    case Token::Type::Or:
    case Token::Type::And:
    case Token::Type::LessThan:
    case Token::Type::GreaterThan:
    case Token::Type::LessThanOrEqual:
    case Token::Type::GreaterThanOrEqual:
    case Token::Type::NotEqual:
    case Token::Type::Equal:
    case Token::Type::Concatenate:
    case Token::Type::Plus:
    case Token::Type::Minus:
    case Token::Type::Times:
    case Token::Type::Divide:
    case Token::Type::Modulo:
    case Token::Type::Power:
      return makeNode(AstNode::BinaryOperator{
          token.type, {left, parseExp(getPrecedence(token.type))}});
    default:
      throw UnexpectedToken{token, Token::Type::Number};
  }
}

AstNode::List<std::string_view> Parser::parseNameList() {
  AstNode::List<std::string_view> list{};
  list.push_back(consume(Token::Type::Name).data);
  while (match(Token::Type::Comma)) {
    list.push_back(consume(Token::Type::Name).data);
  }
  return list;
}

AstNode::List<> Parser::parseArguments(AstNode *first) {
  if (match(Token::Type::RightParenthesis)) {
    if (first) {
      return {first};
    }
    return {};
  }

  AstNode::List<> arguments{parseExpList(first)};
  consume(Token::Type::RightParenthesis);
  return arguments;
}

AstNode::List<> Parser::parseExpList(AstNode *first) {
  AstNode::List<> list{makeList()};
  if (first) {
    list.push_back(first);
  }

  list.push_back(parseExp());
  while (match(Token::Type::Comma)) {
    list.push_back(parseExp());
  }
  return list;
}

AstNode::List<> Parser::parseVarList(AstNode *first) {
  AstNode::List<> list{makeList()};
  if (first) {
    list.push_back(first);
  }

  list.push_back(parsePrefixExpression(m_scanner.get().advance()));
  while (match(Token::Type::Comma)) {
    list.push_back(parsePrefixExpression(m_scanner.get().advance()));
  }

  return list;
}

AstNode *Parser::makeNode(AstNode::Data &&data) {
  void *buffer = m_allocator.get().allocate(sizeof(AstNode), alignof(AstNode));
  if (!buffer) {
    return nullptr;
  }

  return new (buffer) AstNode(std::move(data));
}
