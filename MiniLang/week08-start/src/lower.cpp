/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "lower.hpp"

#include <memory>
#include <stdexcept>
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
  if (std::holds_alternative<BlockStatement>(statement.node) ||
      std::holds_alternative<IfStatement>(statement.node) ||
      std::holds_alternative<ForStatement>(statement.node)) {
    // TODO WEEK08: recursively lower blocks and both if branches, rewrite
    // each for into scoped blocks and while, and preserve other statements.
    throw std::logic_error("TODO WEEK08: implement recursive statement lowering");
  }
  return statement;
}

} // namespace

Program lower(Program program) {
  lowerStatements(program.main_body);
  return program;
}

} // namespace mini
