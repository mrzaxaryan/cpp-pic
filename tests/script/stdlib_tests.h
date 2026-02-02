#pragma once

#include "ral/script/script.h"
#include "pal/io/logger.h"

// ============================================================================
// CUSTOM C++ FUNCTIONS FOR STDLIB TESTS
// ============================================================================

// Custom function: greet(name) - prints a greeting
static script::Value StdLibTest_Func_Greet(script::FunctionContext& ctx)
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
static script::Value StdLibTest_Func_Sum(script::FunctionContext& ctx)
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
// STDLIB TESTS CLASS
// ============================================================================

class StdLibTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = TRUE;

        LOG_INFO("Running StdLib Tests...");

        // Test 1: Standard library functions
        if (!TestStdLibFunctions())
        {
            allPassed = FALSE;
            LOG_ERROR("  FAILED: Standard library functions");
        }
        else
        {
            LOG_INFO("  PASSED: Standard library functions");
        }

        // Test 2: Custom functions with StdLib
        if (!TestCustomFunctionsWithStdLib())
        {
            allPassed = FALSE;
            LOG_ERROR("  FAILED: Custom functions with StdLib");
        }
        else
        {
            LOG_INFO("  PASSED: Custom functions with StdLib");
        }

        // Test 3: Print function
        if (!TestPrintFunction())
        {
            allPassed = FALSE;
            LOG_ERROR("  FAILED: Print function");
        }
        else
        {
            LOG_INFO("  PASSED: Print function");
        }

        // Test 4: Type function
        if (!TestTypeFunction())
        {
            allPassed = FALSE;
            LOG_ERROR("  FAILED: Type function");
        }
        else
        {
            LOG_INFO("  PASSED: Type function");
        }

        // Test 5: String functions
        if (!TestStringFunctions())
        {
            allPassed = FALSE;
            LOG_ERROR("  FAILED: String functions");
        }
        else
        {
            LOG_INFO("  PASSED: String functions");
        }

        // Test 6: Math functions
        if (!TestMathFunctions())
        {
            allPassed = FALSE;
            LOG_ERROR("  FAILED: Math functions");
        }
        else
        {
            LOG_INFO("  PASSED: Math functions");
        }

        if (allPassed)
        {
            LOG_INFO("All StdLib tests passed!");
        }
        else
        {
            LOG_ERROR("Some StdLib tests failed!");
        }

        return allPassed;
    }

private:
    static BOOL TestStdLibFunctions()
    {
        script::State* L = new script::State();

        // Register standard library (print, len, str, num, type, abs, min, max)
        script::OpenStdLib(*L);

        auto source = R"(print("Hello from PICScript!");
print("1 + 2 =", 1 + 2);
print("Type of 42:", type(42));
print("len(hello):", len("hello"));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestCustomFunctionsWithStdLib()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        // Register additional custom functions
        L->Register("greet"_embed, EMBED_FUNC(StdLibTest_Func_Greet) );
        L->Register("sum"_embed, EMBED_FUNC(StdLibTest_Func_Sum) );

        auto source = R"(greet("PICScript User");
print("sum(1,2,3,4,5) =", sum(1,2,3,4,5));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestPrintFunction()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(print("Single string");
print("Multiple", "arguments", "test");
print(42);
print(true);
print(nil);
print("Mixed:", 1, true, "end");
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestTypeFunction()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(print("type(42) =", type(42));
print("type(hello) =", type("hello"));
print("type(true) =", type(true));
print("type(nil) =", type(nil));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestStringFunctions()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(print("len(hello) =", len("hello"));
print("len(empty) =", len(""));
print("str(123) =", str(123));
print("str(true) =", str(true));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestMathFunctions()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(print("abs(-5) =", abs(-5));
print("abs(5) =", abs(5));
print("min(3, 7) =", min(3, 7));
print("max(3, 7) =", max(3, 7));
print("min(-1, 1) =", min(-1, 1));
print("max(-1, 1) =", max(-1, 1));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }
};
