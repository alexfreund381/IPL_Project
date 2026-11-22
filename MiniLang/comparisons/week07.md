# Week 07: expression precedence and left associativity

**Works:** all 16 checks pass in solution mode.
**Unresolved:** nothing
## Comparison with the solution
My code is the same as the solution in both functions:
- `term()`: the same `while` over `Plus`/`Minus`, `advance()` to consume the operator, a ternary to pick `Add` or `Subtract`, the right operand read with `factor()`, and `binaryExpression(std::move(left), operation, std::move(right))` stored back into `left`.
- `unary()`: the same check for `Bang`/`Minus`, `advance()`, a ternary to pick `LogicalNot` or `Negate`, a recursive `unary()` call for the operand, `spanFrom(operator_token.span, operand.span)` for the span, and `std::make_unique` for the owned child.

## The differences are:
- **Comments:** I kept five short comments (why a loop, why `factor()`, why the left side grows, why `unary()` recurses, where the span ends). The solution has none in these two functions.
- **Line wrapping:** I put `operation,` on the same line as `UnaryExpression{` in the `return`; the solution breaks it onto its own line. The code is the same.

My four `w07/` examples give the same `ast` and `run` output with both builds.

## Reflection
My version matches the solution because each part is forced by a test. An `if` instead of the `while` fails `chain.mini` (20 - 3 - 2 + 4 - 1). Reading the right operand with `term()` builds 10 - (3 - 2) = 9 instead of 5. Calling `primary()` in `unary()` rejects `--3`, and calling `expression()` turns `-2 * 3 + 10` into `-(2 * 3 + 10)`. `week07-solution/READ_ME.txt` gives the same reasons.

The part I understand better after reading the solution guide is the loop invariant: at the top of each iteration `left` is the whole tree for everything already consumed, so folding it into the next `binaryExpression` is what gives left association.
