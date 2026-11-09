STAGE 05 SOLUTION — Bind reads and writes through lexical lookup

HOW TO USE THIS SOLUTION
After attempting the exercise, compare the three completed parts and trace
which declaration each read and assignment finds. ASSIGNMENT.txt gives all
inputs and manual commands. This explanation connects the lookup policy to
AST traversal, including grouped expressions and assignment values.

WHAT CHANGED AND WHY THE START REGRESSES
Only src/resolver.cpp changes. You complete the body of resolveName(), the
whole resolveExpression() body and the AssignmentStatement branch body in
resolveStatement(). declare(), define(), the VarStatement visitor and block
entry/visit/exit remain complete from the previous exercise.
The start explicitly rejects reads, grouping and assignments with:
    TODO WEEK05: bind variable read
    TODO WEEK05: resolve grouped expression
    TODO WEEK05: bind assignment target and value
The lookup helper itself contains:
    TODO WEEK05: implement name lookup for '<name>'
Once its callers are connected, they reach that helper until it too is
completed. Thus even local reads intentionally wait for your implementation.
Bare literals fall through the expression visitor unchanged and remain a
working baseline. A grouped literal reaches the explicit grouping TODO.
No language syntax or runtime representation changes in this exercise.

LOOKUP: NEAREST EXISTING DECLARATION
The completed method is:
      SymbolId resolveName(const std::string &name, const SourceSpan span) const {
        for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
          const auto binding = scope->find(name);
          if (binding == scope->end()) {
            continue;
          }
          if (!binding->second.defined) {
            throw CompileError(Phase::Resolve, source_.name(), span,
                               "cannot read variable '" + name +
                                   "' in its own initializer");
          }
          return binding->second.id;
        }

        throw CompileError(Phase::Resolve, source_.name(), span,
                           "unknown variable '" + name + "'");
      }

scopes_ is ordered outermost to innermost, so reverse iterators begin at the
most recently entered block. A missing entry in one map means continue, not
unknown name yet. The first matching entry fixes the declaration identity.
The const method never inserts bindings, changes flags or allocates IDs.

DISTINGUISH THE TWO FAILURES
A matching binding with defined == false is present but its initializer has
not finished resolution. Reject it immediately; do not try a same-named outer
declaration instead. Only absence from every active map is an unknown name.
Both diagnostics use Phase::Resolve, source_.name() and the requested span.
They happen before C++ emission or execution. Returning nil, a fresh ID or
an initializer value would violate the SymbolId result contract.
The supplied declaration visitor installs Binding{id, false}, resolves its
initializer, then defines it. In var x = 1; { var x = x; }, the unfinished
inner x already shadows outer x. The right-hand x must therefore be rejected.
For var original = 5; { var copy = original; }, the names differ: the inner
map lacks original, so reading that outer declaration is legitimate.

EXPRESSION TRAVERSAL: ANNOTATE THE REAL AST NODE
The complete expression visitor is:
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

std::get_if gives mutable access to the stored variant node. The variable
case stores the lookup result on that node, not on a copy. Grouping owns a
child expression; dereferencing that pointer allows recursive traversal.
Nested parentheses therefore cannot hide a name from resolution. Both cases
return when handled. Literal variants fall through without name work.
This visitor does not calculate a value, emit C++ or create a new binding.

ASSIGNMENT: BIND THE TARGET AND CHECK ITS VALUE
The AssignmentStatement branch body is:
          assignment->resolved_id =
              resolveName(assignment->name, assignment->name_span);
          resolveExpression(assignment->value);
          return;

The surrounding mutable node-dispatch condition is unchanged. Resolve the
target first, using name_span rather than the whole assignment span. Store
its declaration ID before visiting the right-hand expression. Then return
so the final unsupported-node diagnostic cannot be reached.
The right side may contain a variable inside grouping, and it must receive
annotations too. An unknown target does not implicitly declare a variable;
a known target does not make an unknown right side valid. Assignment changes
a runtime value later, never the declaration's identity during resolution.

PROVIDED INFRASTRUCTURE AND INVARIANTS
The parser, AST types and formatting, declaration protocol, block traversal,
C++ generator and runtime storage are supplied. Each declaration obtains its
ID once; uses and targets reuse it. The generator chooses names such as
mini_variable_0 from these annotations. Resolution itself is an empty result
object: annotations on VarStatement, VariableExpression and AssignmentStatement
carry the identities into generation. Scope maps do not store changing values.
- Current-map matches have priority over enclosing declarations.
- An undefined nearest match blocks fallback, even when outer x is defined.
- Only exhausting active scopes produces the unknown-name error.
- Expression traversal reaches names under every grouping node.
- Assignment resolves its target, then its entire value expression.
- Earlier declaration allocation, duplicate checks and scope pops are intact.
Searching outermost-first breaks shadowing. Returning a fresh ID per use breaks
storage identity. Skipping the assignment value leaves unresolved names for
code generation. These are different errors and need different examples.

BASELINE AND POSITIVE ACCEPTANCE
inputs/starting_point.mini contains:
    fun main() { print 42; }
Run it from the package root with your actual rebuilt executable:
    ./build/minic run inputs/starting_point.mini
Both versions print exactly 42 and a newline, with empty stderr and exit 0.
This exercises native execution without requesting the unfinished name work.

inputs/acceptance.mini contains:
    fun main() {
      var x = 10;
      { x = 30; }
      print x;
    }
Here minic means the executable from your build, for example ./build/minic:
    minic ast inputs/acceptance.mini
    minic check inputs/acceptance.mini
    minic run inputs/acceptance.mini
ast succeeds in both variants. It shows declaration, block, assignment and
read, but not resolved_id fields. check/run in the start exit 1 with empty
stdout and resolve: TODO WEEK05: bind assignment target and value on stderr,
preceded by source path/location. The literal initializer itself is valid.
The solution check succeeds silently. run exits 0 with empty stderr and
prints exactly 30 followed by a newline. Declaration, assignment and later
read all use SymbolId 0. The inner map lacks x, so search continues outward.
These IDs are a hand trace, not ast stdout. minic cpp inputs/acceptance.mini
uses mini_variable_0 for all three occurrences. Its value becomes 30 only
when generated C++ executes. Stage 04 rejected this example intentionally
under its temporary current-only lookup rule.

EDGE CONTRACTS
ASSIGNMENT.txt and inputs/ contain all complete programs. Successful runs
have empty stderr, exit 0 and newline-terminated outputs. Invalid checks have
empty stdout, resolve diagnostics on stderr with path/location, and exit 1.
- Local reads print 10. Nearest shadowing prints 20 then 10.
- Nearest outward assignment prints 30 then 10, leaving outermost x unchanged.
- An unfinished inner x is rejected without fallback to outer x.
- Completely missing names are rejected after the active maps are exhausted.
- A different outer name can initialize a local copy; that program prints 5.
- A popped block's y stays unknown; same-scope duplicates remain rejected.
- Nested parentheses around an outward read print 7; grouped literals print 42.
- A grouped right-hand read in assignment prints 2 after updating the target.
- Unknown assignment targets and unknown right-hand names are both rejected.
- A grouped initializer resolves its outer read and prints 5.
The duplicate error is supplied in both modes. Other read/grouping/assignment
edges reach the corresponding caller TODO in the untouched start, before
more specific lookup results become available after completion.

REBUILD AND VERIFY
    cmake --build <your-build-directory> --config Debug
Use your actual build directory or IDE Build. Stop if compilation fails.
Before completing the original start:
    python3 CHECK_PROJECT.py --mode start --compiler ./build/minic
After your edits, or when running this solution:
    python3 CHECK_PROJECT.py --mode solution --compiler ./build/minic
Substitute your executable path. Python 3.8+ is only needed for this optional
runner; manual commands remain available without it. The 18 checks cover the
baseline, acceptance AST/check/run and fourteen semantic edges. Exact expected
outputs, diagnostic fragments and exit statuses are in checks/CASES.json.
A PASS includes expected rejections. Start-mode PASS does not claim the task
is finished. Use solution mode after completing the task: start mode still expects the
TODO errors. Then try a nested block of your own and explain which declaration
each use of a name reaches.

UNCHANGED LANGUAGE AND NEXT EXERCISE
One parameterless main, literals, grouping, print, variables, assignments and
blocks are supported. Operators, branches, loops, general calls and explicit
returns are absent. --help describes final Mini, and lowering remains identity.
No interpreter, runtime memory manager, garbage collector or static type
checker is introduced. Compiler scope tables hold bindings; native variables
hold values. Successful check alone does not execute the Mini program.
Keep the completed binding traversal. In Stage 07, we will implement the
additive parser loop and unary operator branch. The next project supplies
operator AST nodes, other parser levels, additional resolver traversal,
C++ generation and runtime helpers. Existing binding rules still apply inside
larger expressions. Division and string concatenation remain outside that task.

LOCAL MATERIALS
This package has 31 compiler/configuration source files, plus documentation,
inputs and check assets. ASSIGNMENT.txt states the work; READ_ME.txt explains
this solution; PROJECT_DOCUMENTATION.pdf describes the components. Keep src/,
runtime/ and CMakeLists.txt together, and use the build for this source folder.
