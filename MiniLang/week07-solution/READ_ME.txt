STAGE 07 SOLUTION GUIDE — Expression precedence and left associativity

This exercise builds on expression theory.

1. HOW TO USE THIS SOLUTION

Read this guide after trying the task in your starting project. Compare your
term() and unary() with the implementations below, then trace how their trees
reach the generated program. ASSIGNMENT.txt describes the task and all test inputs;
PROJECT_DOCUMENTATION.pdf explains the surrounding compiler and source files.

For this exercise you change src/parser.cpp, in term() and unary(). You
complete an additive loop and a recursive prefix-operator branch. The other
compiler phases are already connected, so you can observe the effect of this edit
all the way from an AST to a running program. The inputs and CHECK_PROJECT.py
let you check both the starting behavior and the completed behavior.

2. WHAT YOUR STARTING PROJECT ALREADY DOES

Your project already has unary and binary AST nodes, the other precedence
levels, traversal of expression operands, ordered C++ generation and operations
on dynamic values. Variable declarations, assignments and nested lexical
scopes also work. Name resolution records which declaration each name uses.
Your parser change builds trees that these existing phases can process.

The lexer already recognizes +, -, * and slash. Recognition alone does not
promise a grammar production: slash remains unsupported by factor().
The supplied operators include multiplication, comparisons, equality and
short-circuit and/or. Both -2 and 2 - 1 reach deliberate TODOs in the start,
but at different grammar levels. Unary nodes and their later-phase behavior
are provided; constructing their syntax is now part of your work.
Only the one parameterless main function is available; if/for and general
functions/calls have not arrived. Strings can be printed, but + is numeric.

3. THE EXACT SOURCE CHANGE

The original function reads one factor. If the next token is Plus or Minus,
it throws a location-bearing parse error with TODO WEEK07. It otherwise
returns the factor unchanged, which is why the multiplication baseline runs.
The completed function replaces that guard with an accumulating loop:

  Expression term() {
    auto left = factor();
    while (check(TokenKind::Plus) || check(TokenKind::Minus)) {
      const auto &operator_token = advance();
      const auto operation = operator_token.kind == TokenKind::Plus
                                 ? BinaryOperator::Add
                                 : BinaryOperator::Subtract;
      auto right = factor();
      left = binaryExpression(std::move(left), operation, std::move(right));
    }
    return left;
  }

The second TODO is the prefix branch of unary(). Its completed form is:

  Expression unary() {
    if (check(TokenKind::Bang) || check(TokenKind::Minus)) {
      const auto &operator_token = advance();
      const auto operation = operator_token.kind == TokenKind::Bang
                                 ? UnaryOperator::LogicalNot
                                 : UnaryOperator::Negate;
      auto operand = unary();
      const auto span = spanFrom(operator_token.span, operand.span);
      return Expression{
          span, UnaryExpression{
                    operation,
                    std::make_unique<Expression>(std::move(operand))}};
    }
    return primary();
  }

Remove both TODO throws/comments. Keep the provided pass-through statements,
factor(), binaryExpression(), operator enumerations and runtime. The changes
belong in the two operator branches; keep their function signatures and the
provided pass-through statements.
Deleting a throw without consuming and representing the operator is incomplete.

4. WHY THE ALGORITHM HAS THIS SHAPE

On entry, the token cursor points at the beginning of an additive expression.
factor() consumes one complete multiplicative operand. At every loop boundary,
left is the tree for the additive prefix already consumed. The next token is
either another additive operator or the first token owned by the caller.

advance() consumes exactly the recognized + or -. The next factor() consumes
its right operand, including any multiplication. binaryExpression combines
the old left tree, selected operation and new right tree. Assigning that new
tree to left maintains the invariant for another iteration.

The loop is necessary because additive chains have no fixed operator count.
A single if leaves later operators unconsumed. Calling term() for the right
operand would instead consume a whole additive suffix and can build a
right-associated tree. Calling expression() would also cross precedence levels.
The parser must stop without swallowing a token such as a semicolon or ).

The children contain unique_ptr-owned subtrees. std::move transfers ownership
into the new binary node; it does not assign a value in the Mini program.
The supplied binaryExpression helper constructs children and the combined
SourceSpan. Keep that responsibility there rather than duplicating it.

unary() consumes a prefix operator before calling itself. The recursive call
therefore starts at the operand, including any further prefixes, and eventually
reaches primary(). --3 constructs Negate(Negate(3)); !!true nests two logical
negations. Calling primary() directly would reject the second prefix. Calling
expression() would let unary minus absorb a weaker additive suffix. The span
starts at the first operator and ends at the complete operand; the child is
owned by the resulting UnaryExpression. No value is computed during parsing.

5. WORKED TRACE: THE TREE EXPLAINS THE NUMBERS

inputs/acceptance.mini contains this complete program:
  fun main() { print 2 + 3 * 4; print 10 - 3 - 2; }

For 2 + 3 * 4, the initial factor is 2. After +, the right factor consumes
3 * 4 as one subtree. term combines 2 with that multiplication. In schematic
notation, the result is Add(2, Multiply(3, 4)), not Multiply(Add(2, 3), 4).
These schematic names explain the shape; ast uses parenthesized binary nodes.

For 10 - 3 - 2, the first iteration creates Subtract(10, 3). The next iteration
uses that whole tree as its left child: Subtract(Subtract(10, 3), 2).
At runtime that gives 7 - 2 = 5. Right association would give 10 - 1 = 9.
The parser does not calculate either result; it stores syntax and source spans.

Parentheses invoke expression parsing inside a GroupingExpression. The input
in inputs/grouped.mini is fun main() { print (2 + 3) * 4; }; the grouping makes the sum
an operand of multiplication, so its output is 20 instead of 14.
inputs/chain.mini extends the additive chain to 20 - 3 - 2 + 4 - 1, producing 18.
This checks repetition across both operator kinds rather than only one +.

6. BUILD, INSPECT AND RUN

Use the build belonging to this stage. From the package root, rebuild with:
  cmake --build <your-build-directory> --config Debug
or use IDE Build. The following assumes the resulting executable is
./build/minic; substitute the path from your own build, including an IDE's
configuration subdirectory when necessary. A stale executable hides edits.

Before editing the start:
  ./build/minic run inputs/starting_point.mini
  python3 CHECK_PROJECT.py --mode start --compiler ./build/minic

After editing and rebuilding, or while studying this solution:
  ./build/minic ast inputs/acceptance.mini
  ./build/minic lower inputs/acceptance.mini
  ./build/minic check inputs/acceptance.mini
  ./build/minic cpp inputs/acceptance.mini
  ./build/minic run inputs/acceptance.mini
  python3 CHECK_PROJECT.py --mode solution --compiler ./build/minic

The baseline prints 12 and exits 0 in both versions. tokens can already scan
the acceptance program in the start. ast and every later phase then fail at
the first binary + with a WEEK07 parse TODO, empty stdout and exit 1.
After completion, check is silent and exits 0. cpp prints source text; run
compiles it and prints 14 and 5, each followed by a newline, then exits 0.
lower and ast have identical structure because lowering is an identity here.

7. EXPECTED CHECKPOINT RESULTS

The supplied checker has 16 checks over eight inputs; all run in either mode.
The manifest checks the following observations, not just process success:

  Input / command                 Untouched start       Completed project
  starting_point / run            12; exit 0            same
  acceptance / tokens             PLUS, STAR, MINUS, EOF same; exit 0
  acceptance / ast, lower         parse TODO; exit 1    correct nested trees
  acceptance / check              parse TODO; exit 1    silent; exit 0
  acceptance / cpp                parse TODO; exit 1    arithmetic helper calls
  acceptance / run                parse TODO; exit 1    14, 5; exit 0
  grouped / ast, run              parse TODO; exit 1    group node; 20; exit 0
  chain / run                     parse TODO; exit 1    18; exit 0
  unsupported_division / check    parse error; exit 1   same rejection
  short_circuit / run             false, true; exit 0   same
  unary / ast                     unary TODO; exit 1   nested unary/binary trees
  unary / run                     unary TODO; exit 1   4, 13, 3, true; exit 0
  unary_type / check              unary TODO; exit 1   silent; exit 0
  unary_type / run                unary TODO; exit 1   runtime type error; exit 1

Every successful run above ends its output lines with newlines and has empty
stderr. Rejected inputs have empty stdout. Division reports expected ';'
after print value because slash cannot continue the parsed print expression.
The short-circuit input skips two ill-typed right comparisons; those runtime
errors must never execute. This preserves existing generator behavior.

inputs/unary.mini combines -2 * 3 + 10, 10 - -3, --3 and !!true. It separates
prefix precedence, binary versus unary minus, and recursive prefixes. Inspect
its AST as well as the four outputs. Start reports the location-bearing parse
message TODO WEEK07: implement unary operator parsing. The !1 expression in
inputs/unary_type.mini parses and passes check after completion, but run emits
runtime: operand of '!' must be bool and exits 1 with empty stdout. Checking
syntax and names does not execute the runtime's Boolean type check.

A PASS in start mode includes matched expected failures. It confirms that
this is the intended starting state, not that the parser task is complete.
After the solution checks pass, inspect the AST and your source change as well
as the numeric output. Add an expression of your own and predict its tree.
Python is used only for the optional checker, not to build the C++ compiler.

8. COMMON MISTAKES AND EXPLANATIONS

If 10 - 3 - 2 produces 9, inspect tree association and the right-operand call.
If 2 + 3 * 4 produces 20, factor() was not allowed to own the multiplication.
If a long chain fails after one operator, replace the single branch with a loop.
If --3 or !!true fails, check that unary() recursively reads its operand.
If -2 * 3 + 10 gives a wrong tree, check that unary() stops before the binary suffix.
If check still reports TODO after the edit, rebuild and confirm the executable
path. If tokens succeeds while ast fails, the scanner and parser are distinct.

Why is this not constant folding? term() must build the same shape for variable
operands whose runtime values are unknown. Its job is grammatical structure.
Why not add division now? That is a later coordinated stage, not a missing
case of this stage's public grammar. Token presence and --help are broader than it.

9. WHAT COMES NEXT

In Stage08 we will study if and for. We will use a starting project with the
new AST nodes, parsing and later-phase support connected. The exercise will
complete a recursive statement lowerer, including the full for-to-while
transformation. Keep this parser implementation for comparison; you do not
need to add those new nodes or statements to complete this task.

You should now be able to explain why the right operand uses factor(), why
the loop gives left association, why unary() recurses and where ownership moves.
Use the provided inputs to connect each explanation to an observable tree
and output. Numeric + remains separate from string concatenation, which we
will study in Stage11; division follows in Stage12.
