STAGE 04 SOLUTION — Declarations and block scopes

HOW TO USE THIS SOLUTION
After attempting the exercise, compare your four completed bodies with this
reference and trace the binding states in the examples. ASSIGNMENT.txt gives
the complete inputs and manual commands. This explanation connects the code
to declaration identity, initialization order and scope lifetime.

WHAT CHANGED
Only src/resolver.cpp changes between the start and solution. The completed
parts are Resolver::declare(), Resolver::define(), and the VarStatement and
BlockStatement branch bodies in resolveStatement(). The method signatures
and dispatch conditions remain the same. This is one declaration-and-scope
protocol; no parser rules, AST types, runtime storage or backend code change.
The start's explicit placeholders are:
    TODO WEEK04: resolve variable declaration
    TODO WEEK04: declare variable '<name>'
    TODO WEEK04: define variable '<name>'
    TODO WEEK04: implement block scope
A VarStatement first reaches its visitor TODO, so the inner helper TODOs
become visible only after that caller is connected. A block with only a print
isolates the block placeholder. All failures occur before generated execution.
The placeholder (void) expressions only keep the unfinished start warning-free;
remove them together with the TODO bodies when inserting the actual algorithm.

DECLARATION: RESERVE AN ID BEFORE RESOLVING THE INITIALIZER
The completed declaration and definition helpers are:
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

The insertion checks only scopes_.back(). An outer declaration with the same
spelling is therefore allowed, but a duplicate in this map is rejected.
emplace does not replace the existing binding when insertion fails.
The new binding starts undefined. After insertion succeeds, its ID is recorded
on VarStatement and next_symbol_ advances once. The counter is not reset when
a block ends: even sibling declarations must have distinct IDs.
define uses at(name) because the current binding must already exist. It marks
that same binding ready; it neither inserts another declaration nor changes ID.
Binding contains id and defined, not the initializer's runtime value or type.

CONNECT THE DECLARATION PROTOCOL TO AST TRAVERSAL
The VarStatement branch body is:
          declare(*variable);
          resolveExpression(variable->initializer);
          define(variable->name);
          return;

This order is essential. declare first makes the new name visible but
undefined. Resolving var x = x therefore finds that unfinished binding and
rejects the read. Defining before checking the initializer would accept an
invalid self-read. Defining only afterwards permits subsequent uses.
The return prevents fall-through into the unsupported-statement diagnostic.

GIVE EACH BLOCK ONE MAP
The BlockStatement branch body is:
          beginScope();
          resolveStatements(block->statements);
          endScope();
          return;

The supplied std::get_if dispatch gives mutable access to block->statements.
One beginScope precedes all children, resolveStatements visits them in source
order, and one endScope follows successful traversal. Recursive nested blocks
use this same visitor. No map is created separately for each declaration.
A declaration and its later reads must share their block's map.
If a child throws, compilation aborts and its resolver object is discarded.
Error recovery and reuse after failure are outside this exercise.

INVARIANTS TO EXPLAIN
- A successful declaration owns one distinct SymbolId and one current-map entry.
- The binding is undefined during its initializer, then defined for later uses.
- Same-map duplicates fail; same spelling in a new child map may shadow.
- A successful block enters once, visits in order, leaves once and returns.
- Popped maps cannot answer later lookups; sibling maps are independent.
- Scope maps hold compiler bindings, while generated variables hold values.
Creating a scope per statement would hide duplicates and make local reads
fail. Reusing the parent map would reject valid shadowing. Forgetting to pop
would leave the wrong map current. Reusing IDs would disconnect declarations
from their generated storage, even when the source spellings differ.

THE INTENTIONAL CURRENT-SCOPE LIMIT
resolveName() remains supplied and unchanged. It searches scopes_.back(),
rejects an absent name with unknown variable '<name>' in current scope,
rejects an undefined match with the own-initializer diagnostic, and otherwise
returns its existing ID. It does not search enclosing maps in this stage.
An inner local x can be used there; an outer-only x is unavailable inside an
otherwise empty block. Keep that expected rejection separate from leakage:
a finished child scope must never become accessible from its parent.

PROVIDED INFRASTRUCTURE
Identifier scanning, variable/assignment/block parsing and AST formatting,
C++ emission and runtime storage are complete. run() creates main's initial
scope. beginScope(), endScope() and resolveStatements() are provided. The
existing expression visitor binds reads and traverses grouping; the assignment
visitor binds its target then resolves its value. You use these supplied parts.
Resolution is an empty result object. The useful results are resolved_id
annotations written into the existing AST; no runtime environment is returned.

BASELINE AND ACCEPTANCE
inputs/starting_point.mini contains:
    fun main() { print 10; }
Run it from the package root:
    ./build/minic run inputs/starting_point.mini
Both versions print 10 and a newline, with empty stderr and exit 0. A bare
literal needs neither a declaration nor a nested block. This demonstrates the
working native execution path before you complete the resolver protocol.
Use your actual IDE/build executable path if different.

inputs/acceptance.mini declares outer x = 10, then inner x = 20, prints the
inner name, leaves its block and prints outer x. Its full source is in
ASSIGNMENT.txt. Here minic means your rebuilt executable, e.g. ./build/minic:
    minic ast inputs/acceptance.mini
    minic check inputs/acceptance.mini
    minic run inputs/acceptance.mini
ast succeeds in both versions. The start's check/run exit 1 with empty stdout
and resolve: TODO WEEK04: resolve variable declaration on stderr, preceded by
source path/location. The first incomplete operation is the outer declaration.
In the solution, check succeeds silently; run exits 0 with empty stderr and
prints exactly two newline-terminated lines:
    20
    10
Hand trace: outer x gets ID 0, inner x gets ID 1, inner print uses ID 1, and
after the pop final print uses ID 0. This is not an emitted CLI trace. ast does
not display IDs; minic cpp inputs/acceptance.mini exposes mini_variable_0 and
mini_variable_1. Leaving the block does not undo a value overwritten on outer
x: the two declarations have separate storage throughout execution.

EDGE CONTRACTS
The complete sources and exact start/solution expectations are visible in
ASSIGNMENT.txt, inputs/ and checks/CASES.json. Successful runs have exit 0,
empty stderr and newline-terminated output; rejected checks have exit 1,
empty stdout and a resolve diagnostic with path/location on stderr.
- Nested scopes print 3, 2, 10; siblings print 1, 2 and may reuse a spelling.
- A block-local y is unknown after its map is popped.
- Duplicate declarations fail both inside a block and in main.
- An outer-only assignment stays invalid under current-only lookup.
- An initializer cannot read its own undefined declaration.
- A literal-only block prints 7 then 10, isolating block traversal.
- Two main-scope declarations print 1 then 2 and generate distinct IDs 0 and 1.
- A later local assignment prints 2, proving the declaration became defined.
In the untouched start, cases beginning with a var reach the declaration
visitor TODO. Cases beginning with a nested block reach the block TODO.
Their more specific semantic results become reachable after completion.

REBUILD AND VERIFY
    cmake --build <your-build-directory> --config Debug
Use your actual build directory or IDE Build and stop if compilation fails.
Before completing the original start:
    python3 CHECK_PROJECT.py --mode start --compiler ./build/minic
After your edits, or when using this solution:
    python3 CHECK_PROJECT.py --mode solution --compiler ./build/minic
Replace the executable path as necessary. Python 3.8+ is only needed for this
optional runner; all manual commands remain available without Python.
The 15 checks cover the literal baseline, unchanged acceptance AST, acceptance
check/run, ten edge runs/checks and a generated-ID inspection. PASS means that
the expected output, error and exit status matched, including intentional
rejections. Use start mode before editing and solution mode after completing
your work. Explain why declaration, initialization and scope entry must happen
in the stated order; your implementation need not match this one byte for byte.

LANGUAGE BOUNDARIES AND NEXT EXERCISE
One parameterless main, literals, grouping, print, variables, assignments and
blocks are supported after completion. Operators, branches, loops, general
calls and explicit returns are absent. --help describes final Mini. Lowering
is identity. No interpreter, heap, garbage collector or runtime scope stack
is implemented here. Generated C++ already owns variable storage.
Keep the declaration and block protocol. Stage 05 retains these parts and
introduces TODOs for outward lookup, expression traversal and assignment
binding. Even local reads and grouped literals wait for that work, while its
bare literal demonstration still runs. We will reuse existing IDs rather
than repeat declaration allocation or implement runtime memory.

LOCAL MATERIALS
This package has 31 compiler/configuration source files, plus documentation,
inputs and check assets. ASSIGNMENT.txt states the work; READ_ME.txt explains
this solution; PROJECT_DOCUMENTATION.pdf describes the components. Keep src/,
runtime/ and CMakeLists.txt together, and use the build for this source folder.
