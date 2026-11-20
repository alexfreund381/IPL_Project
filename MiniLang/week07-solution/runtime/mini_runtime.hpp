/*
 * Educational Mini compiler runtime.
 * Supports documented Mini 1.0 programs on the intentional happy path.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#pragma once

#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

namespace mini {

using Value = std::variant<std::monostate, double, bool, std::string>;

class RuntimeError final : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

inline double requireNumber(const Value &value, const char *context) {
  if (const auto *number = std::get_if<double>(&value)) {
    return *number;
  }
  throw RuntimeError(std::string(context) + " must be number");
}

inline bool requireBool(const Value &value, const char *context) {
  if (const auto *boolean = std::get_if<bool>(&value)) {
    return *boolean;
  }
  throw RuntimeError(std::string(context) + " must be bool");
}

inline Value addValues(const Value &left, const Value &right) {
  const auto *left_number = std::get_if<double>(&left);
  const auto *right_number = std::get_if<double>(&right);
  if (left_number != nullptr && right_number != nullptr) {
    return Value{*left_number + *right_number};
  }

  throw RuntimeError("operator '+' expects two numbers");
}

inline Value subtractValues(const Value &left, const Value &right) {
  const double left_number = requireNumber(left, "left operand of '-'");
  const double right_number = requireNumber(right, "right operand of '-'");
  return Value{left_number - right_number};
}

inline Value multiplyValues(const Value &left, const Value &right) {
  const double left_number = requireNumber(left, "left operand of '*'");
  const double right_number = requireNumber(right, "right operand of '*'");
  return Value{left_number * right_number};
}

inline Value negateValue(const Value &value) {
  return Value{-requireNumber(value, "operand of unary '-'")};
}

inline Value logicalNotValue(const Value &value) {
  return Value{!requireBool(value, "operand of '!'")};
}

inline Value lessValues(const Value &left, const Value &right) {
  const double left_number = requireNumber(left, "left operand of '<'");
  const double right_number = requireNumber(right, "right operand of '<'");
  return Value{left_number < right_number};
}

inline Value lessEqualValues(const Value &left, const Value &right) {
  const double left_number = requireNumber(left, "left operand of '<='");
  const double right_number = requireNumber(right, "right operand of '<='");
  return Value{left_number <= right_number};
}

inline Value greaterValues(const Value &left, const Value &right) {
  const double left_number = requireNumber(left, "left operand of '>'");
  const double right_number = requireNumber(right, "right operand of '>'");
  return Value{left_number > right_number};
}

inline Value greaterEqualValues(const Value &left, const Value &right) {
  const double left_number = requireNumber(left, "left operand of '>='");
  const double right_number = requireNumber(right, "right operand of '>='");
  return Value{left_number >= right_number};
}

inline Value equalValues(const Value &left, const Value &right) {
  return Value{left == right};
}

inline Value notEqualValues(const Value &left, const Value &right) {
  return Value{left != right};
}

inline std::string formatValueForOutput(const Value &value) {
  if (const auto *number = std::get_if<double>(&value)) {
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << *number;
    return output.str();
  }
  if (const auto *boolean = std::get_if<bool>(&value)) {
    return *boolean ? "true" : "false";
  }
  if (const auto *string = std::get_if<std::string>(&value)) {
    return *string;
  }
  return "nil";
}

inline void printValue(const Value &value) {
  std::cout << formatValueForOutput(value) << '\n';
}

} // namespace mini
