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
    // Keep the block node (and its scope); only its contents are visited.
    lowerStatements(block->statements);
    return statement;
  }
  if (auto *branch = std::get_if<IfStatement>(&statement.node)) {
    // Visit both syntax branches; the condition is not evaluated here.
    *branch->then_branch = lowerStatement(std::move(*branch->then_branch));
    if (branch->else_branch) {
      *branch->else_branch = lowerStatement(std::move(*branch->else_branch));
    }
    return statement;
  }
  if (auto *loop = std::get_if<ForStatement>(&statement.node)) {
    const auto span = statement.span;
    auto initializer = std::move(*loop->initializer);
    auto condition = std::move(loop->condition);
    auto update = std::move(*loop->update);
    // Recursive call, so loops nested in the body are rewritten too.
    auto body = lowerStatement(std::move(*loop->body));

    // The body stays one whole block, so its names cannot capture the update.
    std::vector<Statement> while_body;
    while_body.push_back(std::move(body));
    while_body.push_back(std::move(update));
    auto synthetic_body =
        Statement{span, BlockStatement{std::move(while_body)}};
    auto while_statement = Statement{
        span, WhileStatement{std::move(condition),
                             std::make_unique<Statement>(
                                 std::move(synthetic_body))}};

    // The outer block limits the initializer variable to the loop.
    std::vector<Statement> outer;
    outer.push_back(std::move(initializer));
    outer.push_back(std::move(while_statement));
    return Statement{span, BlockStatement{std::move(outer)}};
  }
  return statement;
}

} // namespace

Program lower(Program program) {
  lowerStatements(program.main_body);
  return program;
}

} // namespace mini
