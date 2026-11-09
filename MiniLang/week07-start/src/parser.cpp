/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "parser.hpp"

#include "diagnostics.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace mini {
namespace {

class Parser {
public:
  Parser(const SourceText &source, const std::vector<Token> &tokens)
      : source_(source), tokens_(tokens) {
    if (tokens_.empty() || tokens_.back().kind != TokenKind::EndOfFile) {
      throw std::invalid_argument("parser requires an EOF-terminated token list");
    }
  }

  Program run() {
    consume(TokenKind::Fun, "expected 'fun main()' at the start of the program");
    const auto &name = consume(TokenKind::Identifier, "expected 'main'");
    if (name.lexeme != "main") {
      throw CompileError(Phase::Parse, source_.name(), name.span,
                         "this version supports only 'fun main()'");
    }
    consume(TokenKind::LeftParen, "expected '(' after 'main'");
    consume(TokenKind::RightParen, "main has no parameters");
    consume(TokenKind::LeftBrace, "expected '{' before main body");
    auto body = blockDeclarations();
    consume(TokenKind::RightBrace, "expected '}' after main body");
    consume(TokenKind::EndOfFile, "expected end of file after main");
    return Program{std::move(body)};
  }

private:
  const Token &peek() const noexcept { return tokens_[current_]; }

  bool check(const TokenKind kind) const noexcept { return peek().kind == kind; }

  bool checkNext(const TokenKind kind) const noexcept {
    return current_ + 1 < tokens_.size() &&
           tokens_[current_ + 1].kind == kind;
  }

  bool match(const TokenKind kind) noexcept {
    if (!check(kind)) {
      return false;
    }
    advance();
    return true;
  }

  const Token &advance() noexcept {
    const auto &token = peek();
    if (current_ + 1 < tokens_.size()) {
      ++current_;
    }
    return token;
  }

  const Token &previous() const noexcept { return tokens_[current_ - 1]; }

  const Token &consume(const TokenKind kind, const std::string &message) {
    if (check(kind)) {
      return advance();
    }
    throw CompileError(Phase::Parse, source_.name(), peek().span, message);
  }

  SourceSpan spanFrom(const SourceSpan &first, const SourceSpan &last) const {
    const auto last_end = last.offset + last.length;
    return source_.span(first.offset, last_end - first.offset);
  }

  Statement declaration() {
    if (match(TokenKind::Var)) {
      return varDeclaration(previous());
    }
    return statement();
  }

  Statement statement() {
    if (match(TokenKind::Print)) {
      return printStatement(previous());
    }
    if (match(TokenKind::LeftBrace)) {
      return blockStatement(previous());
    }
    if (check(TokenKind::Identifier) && checkNext(TokenKind::Equal)) {
      return assignmentStatement();
    }
    return expressionStatement();
  }

  Statement varDeclaration(const Token &var_keyword) {
    const auto &name =
        consume(TokenKind::Identifier, "expected variable name after 'var'");
    consume(TokenKind::Equal, "expected '=' after variable name");
    auto initializer = expression();
    const auto &semicolon = consume(
        TokenKind::Semicolon, "expected ';' after variable declaration");

    return Statement{
        spanFrom(var_keyword.span, semicolon.span),
        VarStatement{name.lexeme, name.span, std::move(initializer),
                     std::nullopt}};
  }

  Statement printStatement(const Token &print_keyword) {
    auto value = expression();
    const auto &semicolon =
        consume(TokenKind::Semicolon, "expected ';' after print value");

    return Statement{spanFrom(print_keyword.span, semicolon.span),
                     PrintStatement{std::move(value)}};
  }

  Statement assignmentWithoutSemicolon() {
    const auto &name = consume(TokenKind::Identifier,
                               "expected assignment target");
    consume(TokenKind::Equal, "expected '=' after assignment target");
    auto value = expression();
    const auto span = spanFrom(name.span, value.span);

    return Statement{span, AssignmentStatement{name.lexeme, name.span,
                                                std::move(value),
                                                std::nullopt}};
  }

  Statement assignmentStatement() {
    auto assignment = assignmentWithoutSemicolon();
    const auto &semicolon =
        consume(TokenKind::Semicolon, "expected ';' after assignment");
    assignment.span = spanFrom(assignment.span, semicolon.span);
    return assignment;
  }

  Statement expressionStatement() {
    auto value = expression();
    const auto &semicolon =
        consume(TokenKind::Semicolon, "expected ';' after expression");
    const auto span = spanFrom(value.span, semicolon.span);
    return Statement{span, ExpressionStatement{std::move(value)}};
  }

  std::vector<Statement> blockDeclarations() {
    std::vector<Statement> statements;
    while (!check(TokenKind::RightBrace) &&
           !check(TokenKind::EndOfFile)) {
      statements.push_back(declaration());
    }
    return statements;
  }

  Statement blockStatement(const Token &left_brace) {
    auto statements = blockDeclarations();
    const auto &right_brace =
        consume(TokenKind::RightBrace, "expected '}' after block");
    return Statement{spanFrom(left_brace.span, right_brace.span),
                     BlockStatement{std::move(statements)}};
  }

  Expression expression() { return logicOr(); }

  Expression logicOr() {
    auto left = logicAnd();
    while (match(TokenKind::Or)) {
      auto right = logicAnd();
      left = binaryExpression(std::move(left), BinaryOperator::LogicalOr,
                              std::move(right));
    }
    return left;
  }

  Expression logicAnd() {
    auto left = equality();
    while (match(TokenKind::And)) {
      auto right = equality();
      left = binaryExpression(std::move(left), BinaryOperator::LogicalAnd,
                              std::move(right));
    }
    return left;
  }

  Expression equality() {
    auto left = comparison();
    while (check(TokenKind::EqualEqual) || check(TokenKind::BangEqual)) {
      const auto &operator_token = advance();
      const auto operation = operator_token.kind == TokenKind::EqualEqual
                                 ? BinaryOperator::Equal
                                 : BinaryOperator::NotEqual;
      auto right = comparison();
      left = binaryExpression(std::move(left), operation, std::move(right));
    }
    return left;
  }

  Expression comparison() {
    auto left = term();
    while (check(TokenKind::Less) || check(TokenKind::LessEqual) ||
           check(TokenKind::Greater) || check(TokenKind::GreaterEqual)) {
      const auto &operator_token = advance();
      BinaryOperator operation;
      switch (operator_token.kind) {
      case TokenKind::Less:
        operation = BinaryOperator::Less;
        break;
      case TokenKind::LessEqual:
        operation = BinaryOperator::LessEqual;
        break;
      case TokenKind::Greater:
        operation = BinaryOperator::Greater;
        break;
      case TokenKind::GreaterEqual:
        operation = BinaryOperator::GreaterEqual;
        break;
      default:
        throw std::logic_error("unexpected comparison token");
      }
      auto right = term();
      left = binaryExpression(std::move(left), operation, std::move(right));
    }
    return left;
  }

  Expression term() {
    auto left = factor();
    if (check(TokenKind::Plus) || check(TokenKind::Minus)) {
      // TODO WEEK07: Replace this guard with a loop building a left-associated
      // binary expression. Read the right operand with factor().
      throw CompileError(Phase::Parse, source_.name(), peek().span,
                         "TODO WEEK07: implement addition and subtraction parsing");
    }
    return left;
  }

  Expression factor() {
    auto left = unary();
    while (match(TokenKind::Star)) {
      auto right = unary();
      left = binaryExpression(std::move(left), BinaryOperator::Multiply,
                              std::move(right));
    }
    return left;
  }

  Expression unary() {
    if (check(TokenKind::Bang) || check(TokenKind::Minus)) {
      // TODO WEEK07: consume the operator, recursively parse its operand,
      // and return a unary node with the complete source span.
      throw CompileError(Phase::Parse, source_.name(), peek().span,
                         "TODO WEEK07: implement unary operator parsing");
    }
    return primary();
  }

  Expression primary() {
    if (match(TokenKind::Number)) {
      const auto &token = previous();
      return Expression{token.span,
                        NumberExpression{std::get<double>(token.literal)}};
    }

    if (match(TokenKind::String)) {
      const auto &token = previous();
      return Expression{token.span,
                        StringExpression{std::get<std::string>(token.literal)}};
    }

    if (match(TokenKind::True)) {
      return Expression{previous().span, BoolExpression{true}};
    }
    if (match(TokenKind::False)) {
      return Expression{previous().span, BoolExpression{false}};
    }
    if (match(TokenKind::Nil)) {
      return Expression{previous().span, NilExpression{}};
    }
    if (match(TokenKind::Identifier)) {
      const auto &name = previous();
      return Expression{name.span,
                        VariableExpression{name.lexeme, std::nullopt}};
    }

    if (match(TokenKind::LeftParen)) {
      const auto &left_paren = previous();
      auto nested = expression();
      const auto &right_paren =
          consume(TokenKind::RightParen, "expected ')' after expression");
      return Expression{
          spanFrom(left_paren.span, right_paren.span),
          GroupingExpression{
              std::make_unique<Expression>(std::move(nested))}};
    }

    throw CompileError(Phase::Parse, source_.name(), peek().span,
                       "expected expression");
  }

  Expression binaryExpression(Expression left, const BinaryOperator operation,
                              Expression right) {
    const auto span = spanFrom(left.span, right.span);
    auto left_child = std::make_unique<Expression>(std::move(left));
    auto right_child = std::make_unique<Expression>(std::move(right));
    BinaryExpression binary{operation, std::move(left_child),
                            std::move(right_child)};
    return Expression{span, std::move(binary)};
  }

  const SourceText &source_;
  const std::vector<Token> &tokens_;
  std::size_t current_{0};
};

} // namespace

Program parse(const SourceText &source, const std::vector<Token> &tokens) {
  return Parser(source, tokens).run();
}

} // namespace mini
