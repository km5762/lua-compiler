#include "parser.hpp"

#include <charconv>
#include <expected>
#include <memory_resource>
#include <string_view>

#include "ast.hpp"
#include "scanner.hpp"
#include "token.hpp"

Result<ast::Node *> Parser::parse(Scanner &scanner,
                                  std::pmr::memory_resource &allocator) {
  Parser parser{scanner, allocator};
  return parser.parseChunk();
}

Result<ast::Node *> Parser::parseChunk() {
  Result<ast::Node *> block{parseBlock<false>(Token::Type::Eof)};
  if (!block) {
    return std::unexpected{block.error()};
  }
  return makeNode(ast::Chunk{*block});
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

Result<ast::Node *> Parser::parseExpression(int precedence) {
  Result<Token> token{m_scanner.get().advance()};
  if (!token) {
    return std::unexpected{token.error()};
  }
  Result<ast::Node *> left{parseNud(*token)};
  if (!left) {
    return std::unexpected{left.error()};
  }

  while (true) {
    Result<Token> token{m_scanner.get().peek()};
    if (!token) {
      return std::unexpected{token.error()};
    }
    if (getPrecedence(token->type) <= precedence) {
      break;
    }

    token = m_scanner.get().advance();
    if (!token) {
      return std::unexpected{token.error()};
    }

    left = parseLed(*token, *left);
  }

  return left;
}

Result<ast::Node *> Parser::parseNud(const Token &token) {
  switch (token.type) {
  case Token::Type::False:
    return makeNode(ast::Boolean{false});
  case Token::Type::True:
    return makeNode(ast::Boolean{true});
  case Token::Type::Number:
    return parseNumber(token);
  case Token::Type::String:
    return makeNode(ast::String{token.data});
  case Token::Type::Nil:
    return makeNode(ast::Nil{});
  case Token::Type::Function:
    return parseFunction();
  case Token::Type::LeftBrace:
    return parseTableConstructor();
  case Token::Type::Minus:
  case Token::Type::Not:
  case Token::Type::Length: {
    Result<ast::Node *> expression{parseExpression(getPrecedence(token.type))};
    if (!expression) {
      return std::unexpected{expression.error()};
    }
    return makeNode(ast::UnaryOperator{token.type, *expression});
  }
  default:
    return parsePrefixExpression(token);
  }
}

Result<ast::Node *> Parser::parsePrefixExpression(const Token &token) {
  Result<ast::Node *> left{};
  switch (token.type) {
  case Token::Type::Name:
    left = makeNode(ast::Name{std::string_view{token.data}});
    break;
  case Token::Type::LeftParenthesis:
    left = parseExpression(getPrecedence(token.type));
    if (Result<Token> token{consume(Token::Type::RightParenthesis)}; !token) {
      return std::unexpected{token.error()};
    }
    break;
  default:
    return std::unexpected{makeUnexpectedTokenError(
        token, Token::Type::Name, Token::Type::LeftParenthesis)};
  }

  if (!left) {
    return std::unexpected{left.error()};
  }

  while (true) {
    Result<Token> token{m_scanner.get().peek()};
    if (!token) {
      return std::unexpected{token.error()};
    }
    switch (token->type) {
    case Token::Type::LeftBracket: {
      if (Result<Token> token{m_scanner.get().advance()}; !token) {
        return std::unexpected{token.error()};
      }
      Result<ast::Node *> index{parseExpression(getPrecedence(token->type))};
      if (!index) {
        return std::unexpected{index.error()};
      }
      left = makeNode(ast::Subscript{*left, *index});
      if (!left) {
        return std::unexpected{left.error()};
      }
      if (Result<Token> token{consume(Token::Type::RightBracket)}; !token) {
        return std::unexpected{token.error()};
      }
      break;
    }
    case Token::Type::Dot: {
      if (Result<Token> token{m_scanner.get().advance()}; !token) {
        return std::unexpected{token.error()};
      }
      Result<Token> member{consume(Token::Type::Name)};
      if (!member) {
        return std::unexpected{member.error()};
      }
      left = makeNode(ast::Access{*left, member->data});
      if (!left) {
        return std::unexpected{left.error()};
      }
      break;
    }
    case Token::Type::LeftParenthesis: {
      if (Result<Token> token{m_scanner.get().advance()}; !token) {
        return std::unexpected{token.error()};
      }
      auto arguments{parseArguments()};
      if (!arguments) {
        return std::unexpected{arguments.error()};
      }
      left = makeNode(ast::FunctionCall{*left, *arguments});
      if (!left) {
        return std::unexpected{left.error()};
      }
      break;
    }
    case Token::Type::Colon: {
      if (Result<Token> token{m_scanner.get().advance()}; !token) {
        return std::unexpected{token.error()};
      }
      ast::Node *self{*left};
      Result<Token> member{consume(Token::Type::Name)};
      if (!member) {
        return std::unexpected{member.error()};
      }
      left = makeNode(ast::Access{*left, member->data});
      if (!left) {
        return std::unexpected{left.error()};
      }
      if (Result<Token> token{consume(Token::Type::LeftParenthesis)}; !token) {
        return std::unexpected{token.error()};
      }
      auto arguments{parseArguments(self)};
      if (!arguments) {
        return std::unexpected{arguments.error()};
      }
      left = makeNode(ast::FunctionCall{*left, std::move(*arguments)});
      if (!left) {
        return std::unexpected{left.error()};
      }
      break;
    }
    default:
      return left;
    }
  }

  return left;
}

Result<ast::Node *> Parser::parseVariable(const Token &token) {
  Result<ast::Node *> prefix{parsePrefixExpression(token)};
  if (!prefix) {
    return std::unexpected{prefix.error()};
  }
  if (!assignable(*prefix)) {
    return std::unexpected{makeUnassignableValueError(token, *prefix)};
  }
  return prefix;
}

Result<ast::Node *> Parser::parseNumber(const Token &token) {
  double number{};
  auto [ptr, ec] = std::from_chars(
      token.data.data(), token.data.data() + token.data.size(), number);
  if (ptr != token.data.data() + token.data.size() || ec != std::errc{}) {
    return std::unexpected{makeMalformedNumberError(token)};
  }
  return makeNode(ast::Number{number});
}

Result<ast::Node *> Parser::parseLed(const Token &token, ast::Node *left) {
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
  case Token::Type::Modulo: {
    Result<ast::Node *> right{parseExpression(getPrecedence(token.type))};
    if (!right) {
      return std::unexpected{right.error()};
    }
    return makeNode(ast::BinaryOperator{token.type, {left, *right}});
  }
  case Token::Type::Power:
  case Token::Type::Concatenate: {
    Result<ast::Node *> right{parseExpression(getPrecedence(token.type) - 1)};
    if (!right) {
      return std::unexpected{right.error()};
    }
    return makeNode(ast::BinaryOperator{token.type, {left, *right}});
  }
  default:
    return std::unexpected{makeUnexpectedTokenError(
        token, Token::Type::Or, Token::Type::And, Token::Type::LessThan,
        Token::Type::GreaterThan, Token::Type::LessThanOrEqual,
        Token::Type::GreaterThanOrEqual, Token::Type::NotEqual,
        Token::Type::Equal, Token::Type::Plus, Token::Type::Minus,
        Token::Type::Times, Token::Type::Divide, Token::Type::Modulo,
        Token::Type::Power, Token::Type::Concatenate)};
  }
}

Result<ast::Node *> Parser::parseWhileLoop() {
  Result<ast::Node *> condition{parseExpression()};
  if (!condition) {
    return std::unexpected{condition.error()};
  }
  if (Result<Token> token{consume(Token::Type::Do)}; !token) {
    return std::unexpected{token.error()};
  }
  Result<ast::Node *> block{parseBlock<true>(Token::Type::End)};
  if (!block) {
    return std::unexpected{block.error()};
  }
  if (Result<Token> token{consume(Token::Type::End)}; !token) {
    return std::unexpected{token.error()};
  }
  return makeNode(ast::WhileLoop{*condition, *block});
}

Result<ast::Node *> Parser::parseRepeatLoop() {
  Result<ast::Node *> block{parseBlock<true>(Token::Type::Until)};
  if (!block) {
    return std::unexpected{block.error()};
  }
  if (Result<Token> token{consume(Token::Type::Until)}; !token) {
    return std::unexpected{token.error()};
  }
  Result<ast::Node *> condition{parseExpression()};
  if (!condition) {
    return std::unexpected{condition.error()};
  }
  return makeNode(ast::RepeatLoop{*condition, *block});
}

Result<ast::Node *> Parser::parseLocalDeclaration() {
  Result<bool> function{match(Token::Type::Function)};
  if (!function) {
    return std::unexpected{function.error()};
  }
  if (*function) {
    Result<Token> token{consume(Token::Type::Name)};
    if (!token) {
      return std::unexpected{token.error()};
    }
    auto names{makeList<std::string_view>({token->data})};
    Result<ast::Node *> function{parseFunction()};
    if (!function) {
      return std::unexpected{function.error()};
    }
    ast::List<> values{makeList({*function})};
    return makeNode(ast::LocalDeclaration{std::move(names), std::move(values)});
  }

  auto nameList{parseNameList()};
  if (!nameList) {
    return std::unexpected{nameList.error()};
  }
  Result<ast::List<>> expressionList{};

  Result<bool> assign{match(Token::Type::Assign)};
  if (!assign) {
    return std::unexpected{assign.error()};
  }
  if (*assign) {
    expressionList = parseExpressionList();

    if (!expressionList) {
      return std::unexpected{expressionList.error()};
    }
  }

  return makeNode(
      ast::LocalDeclaration{std::move(*nameList), std::move(*expressionList)});
}

Result<ast::Node *> Parser::parseForLoop() {
  Result<Token> name{consume(Token::Type::Name)};
  if (!name) {
    return std::unexpected{name.error()};
  }

  Result<bool> assign{match(Token::Type::Assign)};
  if (!assign) {
    return std::unexpected{assign.error()};
  }
  if (*assign) {
    Result<ast::Node *> value{parseExpression()};
    if (!value) {
      return std::unexpected{value.error()};
    }
    auto names{makeList<std::string_view>({name->data})};
    ast::List<> values{makeList({*value})};
    Result<ast::Node *> declaration{
        makeNode(ast::LocalDeclaration{std::move(names), std::move(values)})};
    if (!declaration) {
      return std::unexpected{declaration.error()};
    }
    if (Result<Token> token{consume(Token::Type::Comma)}; !token) {
      return std::unexpected{token.error()};
    }

    Result<ast::Node *> condition{parseExpression()};
    if (!condition) {
      return std::unexpected{condition.error()};
    }

    Result<ast::Node *> increment{};
    Result<bool> comma{match(Token::Type::Comma)};
    if (!comma) {
      return std::unexpected{comma.error()};
    }
    if (*comma) {
      increment = parseExpression();
      if (!increment) {
        return std::unexpected{increment.error()};
      }
    }

    if (Result<Token> token{consume(Token::Type::Do)}; !token) {
      return std::unexpected{token.error()};
    }

    Result<ast::Node *> block{parseBlock<true>(Token::Type::End)};
    if (Result<Token> token{consume(Token::Type::End)}; !token) {
      return std::unexpected{token.error()};
    }

    return makeNode(
        ast::NumericForLoop{*declaration, *condition, *increment, *block});
  }

  if (Result<Token> token{consume(Token::Type::Comma)}; !token) {
    return std::unexpected{token.error()};
  }
  auto names{parseNameList(name->data)};
  if (!names) {
    return std::unexpected{names.error()};
  }
  if (Result<Token> token{consume(Token::Type::In)}; !token) {
    return std::unexpected{token.error()};
  }
  auto values{parseExpressionList()};
  if (!values) {
    return std::unexpected{values.error()};
  }
  if (Result<Token> token{consume(Token::Type::Do)}; !token) {
    return std::unexpected{token.error()};
  }
  Result<ast::Node *> block{parseBlock<true>(Token::Type::End)};
  if (!block) {
    return std::unexpected{block.error()};
  }
  if (Result<Token> token{consume(Token::Type::End)}; !token) {
    return std::unexpected{token.error()};
  }

  return makeNode(ast::GenericForLoop{*names, *values, *block});
}

Result<ast::Node *> Parser::parseFunctionDeclaration() {
  Result<Token> token{consume(Token::Type::Name)};
  if (!token) {
    return std::unexpected{token.error()};
  }
  Result<ast::Node *> name{makeNode(ast::Name{token->data})};
  if (!name) {
    return std::unexpected{name.error()};
  }

  while (true) {
    Result<bool> dot{match(Token::Type::Dot)};
    if (!dot) {
      return std::unexpected{dot.error()};
    }
    if (!*dot) {
      break;
    }
    Result<Token> member{consume(Token::Type::Name)};
    if (!member) {
      return std::unexpected{member.error()};
    }
    name = makeNode(ast::Access{*name, member->data});
    if (!name) {
      return std::unexpected{name.error()};
    }
  }

  Result<ast::Node *> function{};
  Result<bool> colon{match(Token::Type::Colon)};
  if (!colon) {
    return std::unexpected{colon.error()};
  }
  if (*colon) {
    Result<Token> member{consume(Token::Type::Name)};
    if (!member) {
      return std::unexpected{member.error()};
    }
    name = makeNode(ast::Access{*name, member->data});
    if (!name) {
      return std::unexpected{name.error()};
    }
    function = parseFunction("self");
  } else {
    function = parseFunction();
  }

  if (!function) {
    return std::unexpected{function.error()};
  }

  ast::List<> variables{makeList({*name})};
  ast::List<> values{makeList({*function})};

  return makeNode(ast::Assignment{std::move(variables), std::move(values)});
}

Result<ast::Node *>
Parser::parseFunction(std::optional<std::string_view> first) {
  if (Result<Token> token{consume(Token::Type::LeftParenthesis)}; !token) {
    return std::unexpected{token.error()};
  }
  auto parameters{parseParameters(first)};
  if (!parameters) {
    return std::unexpected{parameters.error()};
  }
  Result<ast::Node *> block{parseBlock<false>(Token::Type::End)};
  if (!block) {
    return std::unexpected{block.error()};
  }
  if (Result<Token> token{consume(Token::Type::End)}; !token) {
    return std::unexpected{token.error()};
  }

  return makeNode(ast::Function{std::move(*parameters), *block});
}

Result<ast::List<std::string_view>>
Parser::parseNameList(std::optional<std::string_view> first) {
  auto list{makeList<std::string_view>()};
  if (first) {
    list.push_back(*first);
  }
  Result<Token> token{consume(Token::Type::Name)};
  if (!token) {
    return std::unexpected{token.error()};
  }
  list.push_back(token->data);

  while (true) {
    Result<bool> comma{match(Token::Type::Comma)};
    if (!comma) {
      return std::unexpected{comma.error()};
    }
    if (!*comma) {
      break;
    }
    Result<Token> token{consume(Token::Type::Name)};
    if (!token) {
      return std::unexpected{token.error()};
    }
    list.push_back(token->data);
  }
  return list;
}

Result<ast::List<std::string_view>>
Parser::parseParameters(std::optional<std::string_view> first) {
  auto list{makeList<std::string_view>()};
  Result<bool> rightParenthesis{match(Token::Type::RightParenthesis)};
  if (!rightParenthesis) {
    return std::unexpected{rightParenthesis.error()};
  }
  if (*rightParenthesis) {
    if (first) {
      list.push_back(*first);
      return list;
    }
    return list;
  }

  auto parameters{parseNameList(first)};
  if (!parameters) {
    return std::unexpected{parameters.error()};
  }
  if (Result<Token> token{consume(Token::Type::RightParenthesis)}; !token) {
    return std::unexpected{token.error()};
  }
  return parameters;
}

Result<ast::List<>> Parser::parseArguments(ast::Node *first) {
  ast::List<> list{makeList()};
  Result<bool> rightParenthesis{match(Token::Type::RightParenthesis)};
  if (!rightParenthesis) {
    return std::unexpected{rightParenthesis.error()};
  }
  if (*rightParenthesis) {
    if (first) {
      list.push_back(first);
      return list;
    }
    return list;
  }

  auto arguments{parseExpressionList(first)};
  if (!arguments) {
    return std::unexpected{arguments.error()};
  }
  if (Result<Token> token{consume(Token::Type::RightParenthesis)}; !token) {
    return std::unexpected{token.error()};
  }
  return arguments;
}

Result<ast::List<>> Parser::parseExpressionList(ast::Node *first) {
  ast::List<> list{makeList()};
  if (first) {
    list.push_back(first);
  }

  Result<ast::Node *> expression{parseExpression()};
  if (!expression) {
    return std::unexpected{expression.error()};
  }
  list.push_back(*expression);
  while (true) {
    Result<bool> comma{match(Token::Type::Comma)};
    if (!comma) {
      return std::unexpected{comma.error()};
    }
    if (!*comma) {
      break;
    }
    Result<ast::Node *> expression{parseExpression()};
    if (!expression) {
      return std::unexpected{expression.error()};
    }
    list.push_back(*expression);
  }
  return list;
}

Result<ast::List<>> Parser::parseVariableList(ast::Node *first) {
  ast::List<> list{makeList()};
  if (first) {
    list.push_back(first);
  }

  Result<Token> token{m_scanner.get().advance()};
  if (!token) {
    return std::unexpected{token.error()};
  }

  Result<ast::Node *> variable{parseVariable(*token)};
  if (!variable) {
    return std::unexpected{variable.error()};
  }

  list.push_back(*variable);
  while (true) {
    Result<bool> comma{match(Token::Type::Comma)};
    if (!comma) {
      return std::unexpected{comma.error()};
    }
    if (!*comma) {
      break;
    }
    Result<Token> token{m_scanner.get().advance()};
    if (!token) {
      return std::unexpected{token.error()};
    }

    Result<ast::Node *> variable{parseVariable(*token)};
    if (!variable) {
      return std::unexpected{variable.error()};
    }

    list.push_back(*variable);
  }

  return list;
}

ast::Node *Parser::makeNode(ast::Data &&data) {
  void *buffer =
      m_allocator.get().allocate(sizeof(ast::Node), alignof(ast::Node));
  if (!buffer) {
    throw std::bad_alloc{};
  }

  return new (buffer) ast::Node{std::move(data)};
}

Result<ast::Node *> Parser::parseTableConstructor() {
  auto fields{makeList<std::pair<ast::Node *, ast::Node *>>()};
  double index{1};

  while (true) {
    const Result<bool> rightBrace{match(Token::Type::RightBrace)};
    if (!rightBrace) {
      return std::unexpected{rightBrace.error()};
    }

    if (*rightBrace) {
      break;
    }

    const Result<bool> leftBracket{match(Token::Type::LeftBracket)};
    if (!leftBracket) {
      return std::unexpected{leftBracket.error()};
    }

    Result<ast::Node *> field{parseExpression()};
    if (!field) {
      return std::unexpected{field.error()};
    }
    Result<ast::Node *> value{};
    if (*leftBracket) {
      if (const Result<Token> rightBracket{consume(Token::Type::RightBracket)};
          !rightBracket) {
        return std::unexpected{rightBracket.error()};
      }

      if (const Result<Token> assign{consume(Token::Type::Assign)}; !assign) {
        return std::unexpected{assign.error()};
      }

      value = parseExpression();
      if (!value) {
        return std::unexpected{value.error()};
      }
    } else {
      if ((*field)->is<ast::Name>()) {
        const Result<bool> assign{match(Token::Type::Assign)};
        if (!assign) {
          return std::unexpected{assign.error()};
        }
        if (*assign) {
          value = parseExpression();
          if (!value) {
            return std::unexpected{value.error()};
          }
        }
      }

      if (!*value) {
        value = field;
        field = makeNode(ast::Number{index++});
      }
    }
    fields.emplace_back(*field, *value);

    if (const Result<bool> token{
            match(Token::Type::Semicolon, Token::Type::Comma)};
        !token) {
      return std::unexpected{token.error()};
    }
  }

  return makeNode(ast::TableConstructor{std::move(fields)});
}
