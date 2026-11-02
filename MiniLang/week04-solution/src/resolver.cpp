/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "resolver.hpp"

#include "diagnostics.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace mini {
namespace {

struct Binding {
  SymbolId id;
  bool defined;
};

class Resolver {
public:
  Resolver(const SourceText &source, Program &program)
      : source_(source), program_(program) {}

  Resolution run() {
    beginScope();
    resolveStatements(program_.main_body);
    endScope();
    return Resolution{};
  }

private:
  using Scope = std::unordered_map<std::string, Binding>;

  void beginScope() { scopes_.emplace_back(); }

  void endScope() { scopes_.pop_back(); }

  void resolveStatements(std::vector<Statement> &statements) {
    for (auto &statement : statements) {
      resolveStatement(statement);
    }
  }

  void resolveStatement(Statement &statement) {
    if (auto *print = std::get_if<PrintStatement>(&statement.node)) {
      resolveExpression(print->expression);
      return;
    }

    if (auto *variable = std::get_if<VarStatement>(&statement.node)) {
      declare(*variable);
      resolveExpression(variable->initializer);
      define(variable->name);
      return;
    }

    if (auto *assignment =
            std::get_if<AssignmentStatement>(&statement.node)) {
      assignment->resolved_id =
          resolveName(assignment->name, assignment->name_span);
      resolveExpression(assignment->value);
      return;
    }

    if (auto *expression =
            std::get_if<ExpressionStatement>(&statement.node)) {
      resolveExpression(expression->expression);
      return;
    }

    if (auto *block = std::get_if<BlockStatement>(&statement.node)) {
      beginScope();
      resolveStatements(block->statements);
      endScope();
      return;
    }

    throw CompileError(Phase::Resolve, source_.name(), statement.span,
                       "unsupported statement node");
  }

  void declare(VarStatement &variable) {
    auto &scope = scopes_.back();
    const auto id = next_symbol_;
    if (!scope.emplace(variable.name, Binding{id, false}).second) {
      throw CompileError(Phase::Resolve, source_.name(), variable.name_span,
                         "duplicate variable '" + variable.name +
                             "' in the same scope");
    }

    variable.resolved_id = id;
    ++next_symbol_;
  }

  void define(const std::string &name) { scopes_.back().at(name).defined = true; }

  SymbolId resolveName(const std::string &name, const SourceSpan span) const {
    const auto &scope = scopes_.back();
    const auto binding = scope.find(name);
    if (binding == scope.end()) {
      throw CompileError(Phase::Resolve, source_.name(), span,
                         "unknown variable '" + name + "' in current scope");
    }
    if (!binding->second.defined) {
      throw CompileError(Phase::Resolve, source_.name(), span,
                         "cannot read variable '" + name +
                             "' in its own initializer");
    }
    return binding->second.id;
  }

  void resolveExpression(Expression &expression) {
    if (auto *variable =
            std::get_if<VariableExpression>(&expression.node)) {
      variable->resolved_id = resolveName(variable->name, expression.span);
      return;
    }

    if (auto *grouping =
            std::get_if<GroupingExpression>(&expression.node)) {
      resolveExpression(*grouping->expression);
      return;
    }

  }

  const SourceText &source_;
  Program &program_;
  std::vector<Scope> scopes_;
  SymbolId next_symbol_{0};
};

} // namespace

Resolution resolve(const SourceText &source, Program &program) {
  return Resolver(source, program).run();
}

} // namespace mini
