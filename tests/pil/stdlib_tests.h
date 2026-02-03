#pragma once

#include "pil/pil.h"
#include "tests.h"

// ============================================================================
// CUSTOM C++ FUNCTIONS FOR STDLIB TESTS
// ============================================================================

// Custom function: greet(name) - prints a greeting
static script::Value StdLibTest_Func_Greet(script::FunctionContext& ctx)
{
    if (ctx.CheckArgs(1) && ctx.IsString(0))
    {
        LOG_INFO("Hello, %s!", ctx.ToString(0));
    }
    else
    {
        LOG_INFO("Hello, World!");
    }

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

        RUN_TEST(allPassed, TestStdLibFunctions, "Standard library functions");
        RUN_TEST(allPassed, TestCustomFunctionsWithStdLib, "Custom functions with StdLib");
        RUN_TEST(allPassed, TestPrintFunction, "Print function");
        RUN_TEST(allPassed, TestTypeFunction, "Type function");
        RUN_TEST(allPassed, TestStringFunctions, "String functions");
        RUN_TEST(allPassed, TestMathFunctions, "Math functions");

        if (allPassed)
            LOG_INFO("All StdLib tests passed!");
        else
            LOG_ERROR("Some StdLib tests failed!");

        return allPassed;
    }

private:
    static BOOL TestStdLibFunctions()
    {
        script::State* L = CreateScriptState();

        // Register standard library (print, len, str, num, type, abs, min, max)
        script::OpenStdLib(*L);

        auto source = R"(print("Hello from PIL!");
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
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        // Register additional custom functions
        L->Register("greet"_embed, EMBED_FUNC(StdLibTest_Func_Greet) );
        L->Register("sum"_embed, EMBED_FUNC(StdLibTest_Func_Sum) );

        auto source = R"(greet("PIL User");
print("sum(1,2,3,4,5) =", sum(1,2,3,4,5));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestPrintFunction()
    {
        script::State* L = CreateScriptState();
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
        script::State* L = CreateScriptState();
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
        script::State* L = CreateScriptState();
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
        script::State* L = CreateScriptState();
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
