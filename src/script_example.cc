/**
 * script_example.cc - PICScript Usage Examples
 *
 * Demonstrates the Lua-like State API with manual function registration.
 * NO functions are built-in - all must be registered from C++.
 * StdLib_Print outputs directly to Console (no callback needed).
 */

#include "ral/script/script.h"
#include "pal/io/console.h"
#include "bal/types/embedded/embedded_string.h"

// ============================================================================
// CUSTOM C++ FUNCTIONS
// ============================================================================

// Custom function: double(n) - doubles a number
script::Value Func_Double(script::FunctionContext& ctx)
{
    if (ctx.CheckArgs(1) && ctx.IsNumber(0))
    {
        return script::Value::Number(ctx.ToNumber(0) * 2);
    }
    return script::Value::Number(0);
}

// Custom function: square(n) - squares a number
script::Value Func_Square(script::FunctionContext& ctx)
{
    if (ctx.CheckArgs(1) && ctx.IsNumber(0))
    {
        INT64 n = ctx.ToNumber(0);
        return script::Value::Number(n * n);
    }
    return script::Value::Number(0);
}

// Custom function: greet(name) - prints a greeting
script::Value Func_Greet(script::FunctionContext& ctx)
{
    Console::Write<CHAR>("Hello, "_embed);
    if (ctx.CheckArgs(1) && ctx.IsString(0))
    {
        Console::Write<CHAR>(ctx.ToString(0));
    }
    else
    {
        Console::Write<CHAR>("World"_embed);
    }
    Console::Write<CHAR>("!\n"_embed);

    return script::Value::Nil();
}

// Custom function: sum(...) - sums all numeric arguments
script::Value Func_Sum(script::FunctionContext& ctx)
{
    INT64 total = 0;
    for (UINT8 i = 0; i < ctx.GetArgCount(); i++)
    {
        if (ctx.IsNumber(i))
        {
            total += ctx.ToNumber(i);
        }
    }
    return script::Value::Number(total);
}

// ============================================================================
// EXAMPLE 1: Using Standard Library
// ============================================================================

 void Example_WithStdLib()
{
    Console::Write<CHAR>("=== Example 1: With Standard Library ===\n"_embed);

    script::State L;

    // Register standard library (print, len, str, num, type, abs, min, max)
    script::OpenStdLib(L);

    auto source = "(print(\"Hello from PICScript!\");print(\"1 + 2 =\", 1 + 2);print(\"Type of 42:\", type(42));print(\"len(hello):\", len(\"hello\"));)"_embed;

    L.DoString(source);
    Console::Write<CHAR>("\n"_embed);
}

// ============================================================================
// EXAMPLE 2: Manual Function Registration (No StdLib)
// ============================================================================

NOINLINE void Example_ManualRegistration()
{
    Console::Write<CHAR>("=== Example 2: Manual Registration Only ===\n"_embed);

    script::State L;

    // Register ONLY the functions we need - NO standard library
    L.Register("print"_embed, script::StdLib_Print);
    L.Register("double"_embed, Func_Double);
    L.Register("square"_embed, Func_Square);

    // Note: len, str, num, type are NOT available - not registered
    auto source = R"(print("Only print, double, square are available");
print("double(5) =", double(5));
print("square(4) =", square(4));
)"_embed;

    L.DoString(source);
    Console::Write<CHAR>("\n"_embed);
}

// ============================================================================
// EXAMPLE 3: Custom Functions
// ============================================================================

NOINLINE void Example_CustomFunctions()
{
    Console::Write<CHAR>("=== Example 3: Custom Functions ===\n"_embed);

    script::State L;
    script::OpenStdLib(L);

    // Register additional custom functions
    L.Register("greet"_embed, Func_Greet);
    L.Register("sum"_embed, Func_Sum);

    auto source = R"(greet("PICScript User");
print("sum(1,2,3,4,5) =", sum(1,2,3,4,5));
)"_embed;

    L.DoString(source);
    Console::Write<CHAR>("\n"_embed);
}

// ============================================================================
// EXAMPLE 4: Setting Global Variables from C++
// ============================================================================

NOINLINE void Example_GlobalVariables()
{
    Console::Write<CHAR>("=== Example 4: Global Variables ===\n"_embed);

    script::State L;
    script::OpenStdLib(L);

    // Set global variables from C++
    L.SetGlobalNumber("PI"_embed, 2, 314);
    L.SetGlobalString("version"_embed, 7, "1.0.0"_embed, 5);
    L.SetGlobalBool("debug"_embed, 5, TRUE);

    auto source = R"(print("PI (x100) =", PI);
print("Version:", version);
if (debug) {
    print("Debug mode is ON");
}
)"_embed;
    L.DoString(source);
    Console::Write<CHAR>("\n"_embed);
}

// ============================================================================
// EXAMPLE 5: FizzBuzz
// ============================================================================

NOINLINE void Example_FizzBuzz()
{
    Console::Write<CHAR>("=== Example 5: FizzBuzz ===\n"_embed);

    script::State L;
    script::OpenStdLib(L);

    auto source = R"(fn fizzbuzz(n) {
    for (var i = 1; i <= n; i = i + 1) {
        if (i % 15 == 0) {
            print("FizzBuzz");
        } else if (i % 3 == 0) {
            print("Fizz");
        } else if (i % 5 == 0) {
            print("Buzz");
        } else {
            print(i);
        }
    }
}
fizzbuzz(15);
)"_embed;
    L.DoString(source);
    Console::Write<CHAR>("\n"_embed);
}

// ============================================================================
// EXAMPLE 6: Recursive Functions
// ============================================================================

NOINLINE void Example_Recursion()
{
    Console::Write<CHAR>("=== Example 6: Recursive Functions ===\n"_embed);

    script::State L;
    script::OpenStdLib(L);

    auto source = R"(fn factorial(n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
for (var i = 1; i <= 10; i = i + 1) {
    print("factorial(", i, ") =", factorial(i));
}
)"_embed;
    L.DoString(source);
    Console::Write<CHAR>("\n"_embed);
}

// ============================================================================
// EXAMPLE 7: Error Handling
// ============================================================================

NOINLINE void Example_ErrorHandling()
{
    Console::Write<CHAR>("=== Example 7: Error Handling ===\n"_embed);

    script::State L;
    script::OpenStdLib(L);

    // Script with syntax error (missing semicolon)
    auto source = R"(var x = 10
print(x);
)"_embed;

    if (!L.DoString(source))
    {
        Console::Write<CHAR>("Error: "_embed);
        Console::Write<CHAR>(L.GetError());
        Console::Write<CHAR>("\n"_embed);
    }

    Console::Write<CHAR>("\n"_embed);
}

// ============================================================================
// EXAMPLE 8: Minimal Setup (print only)
// ============================================================================

NOINLINE void Example_MinimalSetup()
{
    Console::Write<CHAR>("=== Example 8: Minimal Setup (print only) ===\n"_embed);

    script::State L;

    // Register ONLY print - absolutely minimal
    L.Register("print"_embed, script::StdLib_Print);

    auto source = R"(var x = 10;
var y = 20;
print("x + y =", x + y);
print("x * y =", x * y);
)"_embed;
    L.DoString(source);
    Console::Write<CHAR>("\n"_embed);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

NOINLINE void RunAllScriptTests()
{
    Console::Write<CHAR>("\n"_embed);
    Console::Write<CHAR>("========================================\n"_embed);
    Console::Write<CHAR>("   PICScript Test Suite\n"_embed);
    Console::Write<CHAR>("   (No built-in functions)\n"_embed);
    Console::Write<CHAR>("========================================\n\n"_embed);

    Example_WithStdLib();
    Example_ManualRegistration();
    Example_CustomFunctions();
    Example_GlobalVariables();
    Example_FizzBuzz();
    Example_Recursion();
    Example_ErrorHandling();
    Example_MinimalSetup();

    Console::Write<CHAR>("========================================\n"_embed);
    Console::Write<CHAR>("   All Tests Complete!\n"_embed);
    Console::Write<CHAR>("========================================\n"_embed);
}
