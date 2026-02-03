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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/stdlib/stdlib_functions.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/stdlib/custom_functions.pil");
        delete L;
        return result;
    }

    static BOOL TestPrintFunction()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/stdlib/print_function.pil");
        delete L;
        return result;
    }

    static BOOL TestTypeFunction()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/stdlib/type_function.pil");
        delete L;
        return result;
    }

    static BOOL TestStringFunctions()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/stdlib/string_functions.pil");
        delete L;
        return result;
    }

    static BOOL TestMathFunctions()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/stdlib/math_functions.pil");
        delete L;
        return result;
    }
};
