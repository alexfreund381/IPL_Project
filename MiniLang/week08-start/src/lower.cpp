/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "lower.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace mini {
namespace {

Statement lowerStatement(Statement statement);

void lowerStatements(std::vector<Statement> &statements) {
  for (auto &statement : statements) {
    statement = lowerStatement(std::move(statement));
  }
}

Statement lowerStatement(Statement statement) {
  if (auto *block = std::get_if<BlockStatement>(&statement.node)) {
    // Lower the children in place; the block node itself (and its scope) stays.
    lowerStatements(block->statements);
    return statement;
  }

  if (auto *conditional = std::get_if<IfStatement>(&statement.node)) {
    // Both branches are syntax, so both are visited; the condition is not
    // evaluated here.
    conditional->then_branch = std::make_unique<Statement>(
        lowerStatement(std::move(*conditional->then_branch)));
    // A missing else is a null pointer, so check before dereferencing.
    if (conditional->else_branch != nullptr) {
      conditional->else_branch = std::make_unique<Statement>(
          lowerStatement(std::move(*conditional->else_branch)));
    }
    return statement;
  }

  if (auto *loop = std::get_if<ForStatement>(&statement.node)) {
    const auto span = statement.span;
    auto initializer = std::move(*loop->initializer);
    auto condition = std::move(loop->condition);
    auto update = std::move(*loop->update);
    // Recursive call, so a for inside the body is lowered too.
    auto body = lowerStatement(std::move(*loop->body));

    // The original body stays one whole block before the update, so its local
    // names cannot capture the update.
    std::vector<Statement> while_body_statements;
    while_body_statements.push_back(std::move(body));
    while_body_statements.push_back(std::move(update));
    Statement while_body{span, BlockStatement{std::move(while_body_statements)}};

    Statement while_statement{
        span, WhileStatement{std::move(condition),
                             std::make_unique<Statement>(std::move(while_body))}};

    // The outer block runs the initializer once and limits the counter to the
    // loop.
    std::vector<Statement> outer_statements;
    outer_statements.push_back(std::move(initializer));
    outer_statements.push_back(std::move(while_statement));
    return Statement{span, BlockStatement{std::move(outer_statements)}};
  }

  // Print, var, assignment and expression statements pass through unchanged.
  return statement;
}

} // namespace

Program lower(Program program) {
  lowerStatements(program.main_body);
  return program;
}

} // namespace mini
