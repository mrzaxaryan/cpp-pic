# PIC Script Language Implementation TODO

A minimal scripting language for the cpp-pic runtime (position-independent, no stdlib).

## Current State: LEVEL 8 - COMPLETE

---

## Language Overview

**Name:** PICScript
**Type:** Dynamically typed, interpreted
**Target:** Embedded in cpp-pic runtime (no .rdata, no CRT)

### Syntax Preview
```
// Variables
var x = 10;
var name = "hello";

// Arithmetic
var result = (x + 5) * 2;

// Control flow
if (x > 5) {
    print(x);
} else {
    print(0);
}

// Loops
while (x > 0) {
    print(x);
    x = x - 1;
}

// Functions
fn add(a, b) {
    return a + b;
}
var sum = add(3, 4);
```

---

## Implementation Levels

### LEVEL 1: Token & Lexer [COMPLETE]
- [x] Token types enum
- [x] Token struct (type, value, line, column)
- [x] Lexer class
  - [x] Single char tokens: `( ) { } [ ] ; , . : + - * / % = < > !`
  - [x] Two char tokens: `== != <= >= && || += -= *= /=`
  - [x] Keywords: `var if else while for fn return true false nil`
  - [x] Identifiers
  - [x] Numbers (integers)
  - [x] Strings with escape sequences
- [x] Single-line and multi-line comments
- [x] Error handling for invalid tokens

### LEVEL 2: AST Nodes [COMPLETE]
- [x] Expression nodes:
  - [x] NumberLiteral
  - [x] StringLiteral
  - [x] BoolLiteral
  - [x] NilLiteral
  - [x] Identifier
  - [x] BinaryExpr (left, op, right)
  - [x] UnaryExpr (op, operand)
  - [x] CallExpr (callee, args)
  - [x] AssignExpr (name, value)
  - [x] IndexExpr (object, index)
  - [x] LogicalExpr (&&, ||)
- [x] Statement nodes:
  - [x] ExprStmt
  - [x] VarDecl (name, initializer)
  - [x] BlockStmt (statements)
  - [x] IfStmt (condition, then_branch, else_branch)
  - [x] WhileStmt (condition, body)
  - [x] ForStmt (desugared to while)
  - [x] FnDecl (name, params, body)
  - [x] ReturnStmt (value)
- [x] AST Allocator (fixed-size pool, no dynamic allocation)

### LEVEL 3: Parser [COMPLETE]
- [x] Recursive descent parser
- [x] Operator precedence:
  1. `= += -= *= /=` (assignment, right-to-left)
  2. `||` (or)
  3. `&&` (and)
  4. `== !=` (equality)
  5. `< > <= >=` (comparison)
  6. `+ -` (term)
  7. `* / %` (factor)
  8. `! -` (unary)
  9. `() []` (call, index)
- [x] Error recovery with synchronization
- [x] Parse program (list of declarations/statements)

### LEVEL 4: Value System [COMPLETE]
- [x] Value union type
  - [x] Nil
  - [x] Bool
  - [x] Number (INT64)
  - [x] String (stack-allocated buffer)
  - [x] Function reference
  - [x] Native function pointer
- [x] Type checking helpers
- [x] Value equality comparison
- [x] Truthiness evaluation

### LEVEL 5: Environment [COMPLETE]
- [x] Variable scope management
- [x] Scope stack for nested blocks (max 32 depth)
- [x] Variable lookup (local -> enclosing -> global)
- [x] Variable definition
- [x] Variable assignment

### LEVEL 6: Interpreter [COMPLETE]
- [x] Tree-walking interpreter
- [x] Expression evaluation
- [x] Statement execution
- [x] Control flow (if, while, for)
- [x] Function calls with scope handling
- [x] Return value propagation
- [x] Short-circuit evaluation for && and ||
- [x] String concatenation with +

### LEVEL 7: Built-in Functions [COMPLETE]
- [x] `print(value...)` - console output (variadic)
- [x] `len(string)` - string length
- [x] `str(value)` - convert to string
- [x] `num(value)` - convert to number
- [x] `type(value)` - get type name

### LEVEL 8: Integration [COMPLETE]
- [x] Script class facade
- [x] Error reporting with line numbers
- [x] Output callback for platform integration
- [x] Native function registration API

---

## File Structure

```
include/ral/script/
├── token.h          # Token types and Token struct
├── lexer.h          # Lexer class
├── ast.h            # AST node definitions + allocator
├── parser.h         # Recursive descent parser
├── value.h          # Value type system + Environment
├── interpreter.h    # Tree-walking interpreter
├── builtins.h       # Built-in functions
└── script.h         # Main entry point / facade
```

---

## Constraints (cpp-pic compatibility)

1. **No .rdata**: All strings use `_embed` suffix for PIC safety
2. **No exceptions**: Use error codes/flags
3. **No dynamic allocation**: Fixed-size buffers and pools
4. **No STL**: Custom containers only
5. **Position-independent**: No function pointer issues (using native fn pointers is OK in interpreted context)

---

## PIC-Safe String Usage (`_embed`)

All string literals in the script engine use the `_embed` user-defined literal to avoid `.rdata` section placement:

```cpp
#include "bal/types/embedded/embedded_string.h"

// ERROR messages
RuntimeError("Division by zero"_embed, expr->line);

// Function registration
L.Register("print"_embed, StdLib_Print);

// Output
Console::Print("Hello"_embed);

// Setting global variables
L.SetGlobalString("version"_embed, 7, "1.0.0"_embed, 5);
```

The `_embed` suffix creates stack-allocated strings that are materialized at runtime, making the code fully position-independent.

---

## Progress Log

| Date       | Level | Status   | Notes |
|------------|-------|----------|-------|
| 2026-02-02 | 1     | COMPLETE | Token & Lexer |
| 2026-02-02 | 2     | COMPLETE | AST Nodes |
| 2026-02-02 | 3     | COMPLETE | Parser |
| 2026-02-02 | 4     | COMPLETE | Value System |
| 2026-02-02 | 5     | COMPLETE | Environment |
| 2026-02-02 | 6     | COMPLETE | Interpreter |
| 2026-02-02 | 7     | COMPLETE | Built-ins |
| 2026-02-02 | 8     | COMPLETE | Integration |

---

## Design Philosophy

**NO BUILT-IN FUNCTIONS** - All functions must be registered from C++.

This design:
- Gives complete control over what functions are available
- Allows minimal setups (register only what you need)
- Makes the script engine fully configurable
- Follows the Lua embedding philosophy

---

## Usage Examples

### Example 1: With Standard Library

```cpp
#include "ral/script/script.h"

void RunScript() {
    script::State L;

    // Register standard library functions
    script::OpenStdLib(L);  // print, len, str, num, type, abs, min, max

    CHAR source[] = "print(\"Hello!\", 1 + 2);";
    L.DoString(source);
}
```

### Example 2: Manual Registration (Minimal)

```cpp
#include "ral/script/script.h"

void RunScript() {
    script::State L;

    // Register ONLY print - nothing else available
    L.Register("print"_embed, script::StdLib_Print);

    CHAR source[] = "print(\"Minimal setup!\");";
    L.DoString(source);
}
```

### Example 3: Custom C++ Function

```cpp
#include "ral/script/script.h"

// Custom function: double(n)
script::Value MyDouble(script::FunctionContext& ctx) {
    if (ctx.CheckArgs(1) && ctx.IsNumber(0)) {
        return script::Value::Number(ctx.ToNumber(0) * 2);
    }
    return script::Value::Number(0);
}

void RunScript() {
    script::State L;
    script::OpenStdLib(L);

    // Register custom function using _embed for PIC safety
    L.Register("double"_embed, MyDouble);

    CHAR source[] = "print(double(21));";  // prints 42
    L.DoString(source);
}
```

---

## Future Enhancements

- [x] Floating-point numbers (using EMBEDDED_DOUBLE) - **COMPLETE**
- [x] Arrays (fixed-size) - **COMPLETE**
  - Array literals: `[1, 2, 3]`, `[]`, mixed types
  - Array access: `arr[i]`
  - Array assignment: `arr[i] = value`
  - String indexing: `str[i]` returns single character
  - `len(array)` - get array length
  - `push(array, value)` - add element to end
  - `pop(array)` - remove and return last element
  - Max 16 elements per array (MAX_ARRAY_SIZE)
  - Max 64 arrays in pool (MAX_ARRAY_POOL)
- [ ] For-each loops
- [ ] Break/continue statements
- [ ] More stdlib functions (substring, indexOf, etc.)
- [ ] File I/O functions (via PAL)
- [ ] Network functions (via PAL)
