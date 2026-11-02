# Week 04: declarations and block scopes

**Works:** all 15 checks pass in solution mode.
**Unresolved:** nothing

## Comparison with the solution
The four parts are the same as in the solution: the `var` branch calls `declare` → resolve initializer → `define`, and the block branch calls `beginScope` → `resolveStatements` → `endScope`.  

## The differences are:
- **Insert:** I used `scope.insert({variable.name, Binding{id, false}})` and the solution uses `scope.emplace(variable.name, Binding{id, false})`. Both return a pair whose `.second` is `false` when the name already exists, and neither overwrites the existing binding. `emplace` builds the map entry directly from its arguments, while `insert` first builds a pair and then moves it in. The behavior is the same.
- **ID type:** I wrote `const SymbolId id`, and the solution uses `const auto id`. The type is the same.
- **`define()`:** identical code, but the solution writes it on one line.


## Reflection
My version uses the same algorithm and step order as the solution. I kept the order `declare` → initializer → `define` because defining first would accept `var x = x`. For blocks I used one map per block and always pop it: a map per statement would hide duplicates, and skipping `endScope()` would leak block names. `week04-solution/READ_ME.txt` gives the same reasons.

My only real choice was `insert` over `emplace`. Mine works the same, but `emplace` is more direct, so I'd use it next time. I also kept short comments on each step, because they helped me follow the order.
