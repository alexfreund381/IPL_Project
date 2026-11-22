# Week 07: expression precedence and left associativity

**Works:** all 16 checks pass in solution mode.
**Unresolved:** nothing

## Comparison with the solution
My code is the same as the solution in both parts:
- `term()`: the same `while` on `Plus`/`Minus`, `advance()` for the operator, `factor()` for the right operand, and `binaryExpression(std::move(left), operation, std::move(right))` assigned back to `left`.
- `unary()`: the same `Bang`/`Minus` check, a recursive `unary()` call for the operand, `spanFrom(operator_token.span, operand.span)`, and the child owned with `std::make_unique<Expression>(std::move(operand))`.

## The differences are:
- **Comments:** I kept short comments on why each step is there (loop instead of `if`, `factor()` on the right, the tree becoming the left side, recursion in `unary()`, the span). The solution has no comments in these functions.
- **Line wrapping:** the `UnaryExpression{...}` return is wrapped differently. The code is the same.

## Reflection
My version matches the solution because each choice is forced by a test. A single `if` would leave the rest of `20 - 3 - 2 + 4 - 1` unread. Calling `term()` for the right operand would build `10 - (3 - 2)` and print 9 instead of 5. Calling `primary()` in `unary()` would reject `--3`, and calling `expression()` would turn `-2 * 3 + 10` into `-(2 * 3 + 10)`. `week07-solution/READ_ME.txt` gives the same reasons.

The parser only builds the tree. No value is computed here, which is why `print !1;` passes `check` and fails only at runtime.
