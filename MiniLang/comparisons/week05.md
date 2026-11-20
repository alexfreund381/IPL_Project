# Week 05: lookup for reads and assignments

**Works:** all 18 checks pass in solution mode.
**Unresolved:** nothing

## Comparison with the solution
My code is the same as the solution in all three parts:
- `resolveName()`: the same reverse loop with `rbegin()`/`rend()`, `continue` when the name is missing, an error at the first undefined match, and "unknown variable" only after every map has been searched.
- `resolveExpression()`: the same `get_if` on variable reads and groupings, recursing into `*grouping->expression`.
- The assignment branch: the target first, then the value.

## The differences are:
- **Line wrapping:** two `if` lines and one assignment are on single lines in mine and wrapped in the solution. The code is the same.

## Reflection
My version matches the solution because each step is forced by a test. Searching from `begin()` instead of `rbegin()` would bind the inner `print x` to the outer `x`. Skipping an undefined match would let `var x = x` copy the outer `x`. Not resolving the assignment value would leave `x = (y)` without an ID for `y`. `week05-solution/READ_ME.txt` gives the same reasons.

The one change I made from the starter code was replacing `holds_alternative` with `get_if`, because I needed a pointer to write `resolved_id` onto the real node. The solution does the same.
