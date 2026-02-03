#pragma once

#include "test_framework.h"

// ============================================================================
// STDLIB TESTS
// ============================================================================

// Custom function: greet(name) - prints a greeting
static script::Value StdLibTest_Func_Greet(script::FunctionContext& ctx)
{
    if (ctx.CheckArgs(1) && ctx.IsString(0))
        LOG_INFO("Hello, %s!", ctx.ToString(0));
    else
        LOG_INFO("Hello, World!");
    return script::Value::Nil();
}

// Custom function: sum(...) - sums all numeric arguments
static script::Value StdLibTest_Func_Sum(script::FunctionContext& ctx)
{
    INT64 total = 0;
    for (UINT8 i = 0; i < ctx.GetArgCount(); i++)
    {
        if (ctx.IsNumber(i))
            total += ctx.ToNumber(i);
    }
    return script::Value::Number(total);
}

class StdLibTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = TRUE;
        LOG_INFO("Running StdLib Tests...");

        RUN_SCRIPT_TEST(allPassed, L"tests/pil/scripts/stdlib/stdlib_functions.pil"_embed, "Standard library functions", CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/pil/scripts/stdlib/print_function.pil"_embed,   "Print function",             CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/pil/scripts/stdlib/type_function.pil"_embed,    "Type function",              CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/pil/scripts/stdlib/string_functions.pil"_embed, "String functions",           CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/pil/scripts/stdlib/math_functions.pil"_embed,   "Math functions",             CFG_STDLIB);

        // Custom test with registered C++ functions
        RUN_TEST(allPassed, TestCustomFunctionsWithStdLib, "Custom functions with StdLib");

        if (allPassed)
            LOG_INFO("All StdLib Tests passed!");
        else
            LOG_ERROR("Some StdLib Tests failed!");

        return allPassed;
    }

private:
    static BOOL TestCustomFunctionsWithStdLib()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        L->Register("greet"_embed, EMBED_FUNC(StdLibTest_Func_Greet));
        L->Register("sum"_embed, EMBED_FUNC(StdLibTest_Func_Sum));
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/stdlib/custom_functions.pil"_embed);
        delete L;
        return result;
    }
};
