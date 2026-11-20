/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "codegen.hpp"

#include "diagnostics.hpp"
#include "number_format.hpp"

#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>

namespace mini {
namespace {

std::string generatedVariableName(const SymbolId id) {
  return "mini_variable_" + std::to_string(id);
}

std::string cppStringLiteral(const std::string &value) {
  // The lexer excludes quotes, backslashes, CR and LF from the payload.
  std::ostringstream output;
  output << '"';
  for (const char character : value) {
    const auto byte = static_cast<unsigned char>(character);
    switch (byte) {
    case '\t':
      output << "\\t";
      break;
    case '?':
      // \? is the standard C++ escape for a question mark. Escaping every
      // question mark prevents valid Mini text such as "??/" from triggering
      // a host compiler's legacy trigraph warning under -Werror.
      output << "\\?";
      break;
    default:
      if (byte >= 0x20U && byte != 0x7FU) {
        output << static_cast<char>(byte);
      } else {
        output << '\\' << std::oct << std::setw(3) << std::setfill('0')
               << static_cast<unsigned int>(byte);
      }
      break;
    }
  }
  output << '"';
  return output.str();
}

const char *runtimeBinaryHelper(const BinaryOperator operation) {
  switch (operation) {
  case BinaryOperator::Add:
    return "addValues";
  case BinaryOperator::Subtract:
    return "subtractValues";
  case BinaryOperator::Multiply:
    return "multiplyValues";
  case BinaryOperator::Less:
    return "lessValues";
  case BinaryOperator::LessEqual:
    return "lessEqualValues";
  case BinaryOperator::Greater:
    return "greaterValues";
  case BinaryOperator::GreaterEqual:
    return "greaterEqualValues";
  case BinaryOperator::Equal:
    return "equalValues";
  case BinaryOperator::NotEqual:
    return "notEqualValues";
  case BinaryOperator::LogicalAnd:
  case BinaryOperator::LogicalOr:
    return nullptr;
  }
  return nullptr;
}

class FunctionEmitter {
public:
  explicit FunctionEmitter(std::ostringstream &output) : output_(output) {}

  void emitStatement(const Statement &statement,
                     const std::size_t indentation = 2) {
    if (const auto *print = std::get_if<PrintStatement>(&statement.node)) {
      const auto value = emitExpression(print->expression, indentation);
      writeLine(indentation, "mini::printValue(" + value + ");");
      return;
    }

    if (const auto *variable = std::get_if<VarStatement>(&statement.node)) {
      const auto initializer =
          emitExpression(variable->initializer, indentation);
      writeLine(indentation,
                "[[maybe_unused]] mini::Value " +
                    generatedVariableName(variable->resolved_id.value()) + " = " +
                    initializer + ";");
      return;
    }

    if (const auto *assignment =
            std::get_if<AssignmentStatement>(&statement.node)) {
      const auto value = emitExpression(assignment->value, indentation);
      writeLine(indentation,
                generatedVariableName(assignment->resolved_id.value()) + " = " +
                    value + ";");
      return;
    }

    if (const auto *expression =
            std::get_if<ExpressionStatement>(&statement.node)) {
      const auto value = emitExpression(expression->expression, indentation);
      writeLine(indentation, "(void)" + value + ";");
      return;
    }

    if (const auto *block = std::get_if<BlockStatement>(&statement.node)) {
      writeLine(indentation, "{");
      for (const auto &nested : block->statements) {
        emitStatement(nested, indentation + 2);
      }
      writeLine(indentation, "}");
      return;
    }

    throw CompileError(Phase::Codegen, "unsupported statement node");
  }

private:
  std::string nextTemporary() {
    return "mini_temp_" + std::to_string(next_temporary_++);
  }

  void writeLine(const std::size_t indentation, const std::string &line) {
    output_ << std::string(indentation, ' ') << line << '\n';
  }

  std::string emitTemporary(const std::string &initializer,
                            const std::size_t indentation) {
    const auto result = nextTemporary();
    writeLine(indentation,
              "const mini::Value " + result + " = " + initializer + ";");
    return result;
  }

  std::string emitExpression(const Expression &expression,
                             const std::size_t indentation) {
    if (const auto *number =
            std::get_if<NumberExpression>(&expression.node)) {
      return emitTemporary("mini::Value{" + formatNumberForCpp(number->value) +
                               "}",
                           indentation);
    }

    if (const auto *string =
            std::get_if<StringExpression>(&expression.node)) {
      return emitTemporary("mini::Value{std::string{" +
                               cppStringLiteral(string->value) + ", " +
                               std::to_string(string->value.size()) + "}}",
                           indentation);
    }

    if (const auto *boolean =
            std::get_if<BoolExpression>(&expression.node)) {
      return emitTemporary(boolean->value ? "mini::Value{true}"
                                          : "mini::Value{false}",
                           indentation);
    }

    if (std::holds_alternative<NilExpression>(expression.node)) {
      const auto result = nextTemporary();
      writeLine(indentation, "const mini::Value " + result + "{};");
      return result;
    }

    if (const auto *variable =
            std::get_if<VariableExpression>(&expression.node)) {
      return emitTemporary(generatedVariableName(variable->resolved_id.value()),
                           indentation);
    }

    if (const auto *grouping =
            std::get_if<GroupingExpression>(&expression.node)) {
      return emitExpression(*grouping->expression, indentation);
    }

    if (const auto *unary = std::get_if<UnaryExpression>(&expression.node)) {
      const auto operand = emitExpression(*unary->operand, indentation);
      const auto *helper = unary->operation == UnaryOperator::LogicalNot
                               ? "logicalNotValue"
                               : "negateValue";
      return emitTemporary(std::string{"mini::"} + helper + "(" + operand + ")",
                           indentation);
    }

    if (const auto *binary =
            std::get_if<BinaryExpression>(&expression.node)) {
      if (binary->operation == BinaryOperator::LogicalAnd ||
          binary->operation == BinaryOperator::LogicalOr) {
        return emitLogical(*binary, indentation);
      }

      const auto left = emitExpression(*binary->left, indentation);
      const auto right = emitExpression(*binary->right, indentation);
      const auto *helper = runtimeBinaryHelper(binary->operation);
      if (helper == nullptr) {
        throw CompileError(Phase::Codegen,
                           "unsupported binary expression node");
      }
      return emitTemporary(std::string{"mini::"} + helper + "(" + left + ", " +
                               right + ")",
                           indentation);
    }

    throw CompileError(Phase::Codegen, "unsupported expression node");
  }

  std::string emitLogical(const BinaryExpression &binary,
                          const std::size_t indentation) {
    const bool is_and = binary.operation == BinaryOperator::LogicalAnd;
    const auto operation = is_and ? std::string{"and"} : std::string{"or"};
    const auto left = emitExpression(*binary.left, indentation);
    const auto result = nextTemporary();
    writeLine(indentation, "mini::Value " + result + ";");

    const auto condition = "mini::requireBool(" + left +
                           ", \"left operand of '" + operation + "'\")";
    if (is_and) {
      writeLine(indentation, "if (" + condition + ") {");
      const auto right = emitExpression(*binary.right, indentation + 2);
      writeLine(indentation + 2,
                result + " = mini::Value{mini::requireBool(" + right +
                    ", \"right operand of 'and'\")};");
      writeLine(indentation, "} else {");
      writeLine(indentation + 2, result + " = mini::Value{false};");
      writeLine(indentation, "}");
    } else {
      writeLine(indentation, "if (" + condition + ") {");
      writeLine(indentation + 2, result + " = mini::Value{true};");
      writeLine(indentation, "} else {");
      const auto right = emitExpression(*binary.right, indentation + 2);
      writeLine(indentation + 2,
                result + " = mini::Value{mini::requireBool(" + right +
                    ", \"right operand of 'or'\")};");
      writeLine(indentation, "}");
    }
    return result;
  }

  std::ostringstream &output_;
  std::size_t next_temporary_{0};
};

} // namespace

std::string emitCpp(const Program &program, const Resolution &) {
  std::ostringstream output;
  output << "// Generated by the educational Mini compiler.\n"
            "#include \"mini_runtime.hpp\"\n"
            "#include <iostream>\n\n"
            "namespace mini_generated {\n\n"
            "void mini_main() {\n";
  FunctionEmitter emitter(output);
  for (const auto &statement : program.main_body) {
    emitter.emitStatement(statement);
  }
  output << "}\n\n"
            "} // namespace mini_generated\n\n"
            "int main() {\n"
            "  try {\n"
            "    mini_generated::mini_main();\n"
            "    return 0;\n"
            "  } catch (const mini::RuntimeError &error) {\n"
            "    std::cerr << \"runtime: \" << error.what() << '\\n';\n"
            "    return 1;\n"
            "  }\n"
            "}\n";
  return output.str();
}

} // namespace mini
