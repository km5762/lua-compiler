#include "parser.hpp"

#include <charconv>
#include <memory_resource>
#include <string_view>

#include "ast.hpp"
#include "scanner.hpp"
#include "token.hpp"

ast::Node *Parser::parse(Scanner &scanner,
                         std::pmr::memory_resource &allocator) {
  Parser parser{scanner, allocator};
  return parser.parseChunk();
}

ast::Node *Parser::parseChunk() {
  return makeNode(ast::Chunk{parseBlock<false>(Token::Type::Eof)});
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

ast::Node *Parser::parseExpression(int precedence) {
  Token token{m_scanner.get().advance()};
  ast::Node *left{parseNud(token)};

  while (getPrecedence(m_scanner.get().peek().type) > precedence) {
    left = parseLed(m_scanner.get().advance(), left);
  }

  return left;
}

ast::Node *Parser::parseNud(Token token) {
  switch (token.type) {
  case Token::Type::False:
    return makeNode(ast::Boolean{false});
  case Token::Type::True:
    return makeNode(ast::Boolean{true});
  case Token::Type::Number:
    return parseNumber(token);
  case Token::Type::Minus:
  case Token::Type::Not:
  case Token::Type::Length:
    return makeNode(ast::UnaryOperator{
        token.type, parseExpression(getPrecedence(token.type))});
  default:
    return parsePrefixExpression(token);
  }
}

ast::Node *Parser::parsePrefixExpression(Token token) {
  ast::Node *left{};
  switch (token.type) {
  case Token::Type::Name:
    left = makeNode(ast::Name{token.data});
    break;
  case Token::Type::LeftParenthesis:
    left = parseExpression(getPrecedence(token.type));
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
          ast::Subscript{left, parseExpression(getPrecedence(token.type))});
      consume(Token::Type::RightBracket);
      break;
    }
    case Token::Type::Dot: {
      m_scanner.get().advance();
      left = makeNode(ast::Access{left, consume(Token::Type::Name).data});
      break;
    }
    case Token::Type::LeftParenthesis: {
      m_scanner.get().advance();
      left = makeNode(ast::FunctionCall{left, parseArguments()});
      break;
    }
    case Token::Type::Colon: {
      m_scanner.get().advance();
      left = makeNode(ast::Access{left, consume(Token::Type::Name).data});
      consume(Token::Type::LeftParenthesis);
      left = makeNode(ast::MethodCall{left, parseArguments()});
      break;
    }
    default:
      return left;
    }
  }

  return left;
}

ast::Node *Parser::parseVariable(Token token) {
  ast::Node *prefix{parsePrefixExpression(token)};
  expect<ast::Access, ast::Subscript, ast::Name>(prefix, token);
  return prefix;
}

ast::Node *Parser::parseNumber(Token token) {
  double number{};
  auto [ptr, ec] = std::from_chars(
      token.data.data(), token.data.data() + token.data.size(), number);
  if (ptr != token.data.end() || ec != std::errc{}) {
    throw MalformedNumber{token};
  }
  return makeNode(ast::Number{number});
}

ast::Node *Parser::parseLed(Token token, ast::Node *left) {
  switch (token.type) {
  case Token::Type::Or:
  case Token::Type::And:
  case Token::Type::LessThan:
  case Token::Type::GreaterThan:
  case Token::Type::LessThanOrEqual:
  case Token::Type::GreaterThanOrEqual:
  case Token::Type::NotEqual:
  case Token::Type::Equal:
  case Token::Type::Plus:
  case Token::Type::Minus:
  case Token::Type::Times:
  case Token::Type::Divide:
  case Token::Type::Modulo:
    return makeNode(ast::BinaryOperator{
        token.type, {left, parseExpression(getPrecedence(token.type))}});
  case Token::Type::Power:
  case Token::Type::Concatenate:
    return makeNode(ast::BinaryOperator{
        token.type, {left, parseExpression(getPrecedence(token.type) - 1)}});
  default:
    throw UnexpectedToken{token, Token::Type::Number};
  }
}

ast::Node *Parser::parseForLoop() {
  Token name{consume(Token::Type::Name)};

  if (match(Token::Type::Assign)) {
    ast::Node *value{parseExpression()};
    auto names{makeList<std::string_view>()};
    ast::List<> values{makeList()};
    names.push_back(name.data);
    values.push_back(value);
    ast::Node *declaration{makeNode(ast::LocalDeclaration{names, values})};
    consume(Token::Type::Comma);

    ast::Node *condition{parseExpression()};

    ast::Node *increment{};
    if (match(Token::Type::Comma)) {
      increment = parseExpression();
    }

    consume(Token::Type::Do);
    ast::Node *block{parseBlock<true>(Token::Type::End)};
    consume(Token::Type::End);

    return makeNode(
        ast::NumericForLoop{declaration, condition, increment, block});
  }

  ast::List<std::string_view> names{parseNameList(name.data)};
  consume(Token::Type::In);
  ast::List<> values{parseExpressionList()};
  consume(Token::Type::Do);
  ast::Node *block{parseBlock<true>(Token::Type::End)};
  consume(Token::Type::End);

  return makeNode(ast::GenericForLoop{names, values, block});
}

ast::List<std::string_view>
Parser::parseNameList(std::optional<std::string_view> first) {
  ast::List<std::string_view> list{};
  if (first) {
    list.push_back(*first);
  } else {
    list.push_back(consume(Token::Type::Name).data);
  }

  while (match(Token::Type::Comma)) {
    list.push_back(consume(Token::Type::Name).data);
  }
  return list;
}

ast::List<> Parser::parseArguments(ast::Node *first) {
  if (match(Token::Type::RightParenthesis)) {
    if (first) {
      return {first};
    }
    return {};
  }

  ast::List<> arguments{parseExpressionList(first)};
  consume(Token::Type::RightParenthesis);
  return arguments;
}

ast::List<> Parser::parseExpressionList(ast::Node *first) {
  ast::List<> list{makeList()};
  if (first) {
    list.push_back(first);
  }

  list.push_back(parseExpression());
  while (match(Token::Type::Comma)) {
    list.push_back(parseExpression());
  }
  return list;
}

ast::List<> Parser::parseVariableList(ast::Node *first) {
  ast::List<> list{makeList()};
  if (first) {
    list.push_back(first);
  }

  list.push_back(parseVariable(m_scanner.get().advance()));
  while (match(Token::Type::Comma)) {
    list.push_back(parseVariable(m_scanner.get().advance()));
  }

  return list;
}

ast::Node *Parser::makeNode(ast::Data &&data) {
  void *buffer =
      m_allocator.get().allocate(sizeof(ast::Node), alignof(ast::Node));
  if (!buffer) {
    return nullptr;
  }

  return new (buffer) ast::Node(std::move(data));
}
