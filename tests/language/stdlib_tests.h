#pragma once

#include "test_framework.h"

// ============================================================================
// STDLIB TESTS
// ============================================================================

// Custom function: greet(name) - prints a greeting
static PIL::Value StdLibTest_Func_Greet(PIL::FunctionContext& ctx)
{
    if (ctx.CheckArgs(1) && ctx.IsString(0))
        LOG_INFO("Hello, %s!", ctx.ToString(0));
    else
        LOG_INFO("Hello, World!");
    return PIL::Value::Nil();
}

// Custom function: sum(...) - sums all numeric arguments
static PIL::Value StdLibTest_Func_Sum(PIL::FunctionContext& ctx)
{
    INT64 total = 0;
    for (UINT8 i = 0; i < ctx.GetArgCount(); i++)
    {
        if (ctx.IsNumber(i))
            total += ctx.ToNumber(i);
    }
    return PIL::Value::Number(total);
}

class StdLibTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = TRUE;
        LOG_INFO("Running StdLib Tests...");

        RunScriptTest(allPassed, L"tests/language/scripts/stdlib/stdlib_functions.pil"_embed, L"Standard library functions"_embed, CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/stdlib/print_function.pil"_embed,   L"Print function"_embed,             CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/stdlib/type_function.pil"_embed,    L"Type function"_embed,              CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/stdlib/string_functions.pil"_embed, L"String functions"_embed,           CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/stdlib/math_functions.pil"_embed,   L"Math functions"_embed,             CFG_STDLIB);

        // Custom test with registered C++ functions
        RunTest(allPassed, EMBED_FUNC(TestCustomFunctionsWithStdLib), L"Custom functions with StdLib"_embed);

        if (allPassed)
            LOG_INFO("All StdLib Tests passed!");
        else
            LOG_ERROR("Some StdLib Tests failed!");

        return allPassed;
    }

private:
    static BOOL TestCustomFunctionsWithStdLib()
    {
        PIL::State* L = CreateScriptState();
        PIL::OpenStdLib(*L);
        L->Register("greet"_embed, EMBED_FUNC(StdLibTest_Func_Greet));
        L->Register("sum"_embed, EMBED_FUNC(StdLibTest_Func_Sum));
        BOOL result = RunScriptAndCheckResult(L, L"tests/language/scripts/stdlib/custom_functions.pil"_embed);
        delete L;
        return result;
    }
};
