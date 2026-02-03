#pragma once

#include "test_framework.h"

// ============================================================================
// STATE TESTS
// ============================================================================

// Custom function: double(n) - doubles a number
static script::Value StateTest_Func_Double(script::FunctionContext& ctx)
{
    if (ctx.CheckArgs(1) && ctx.IsNumber(0))
        return script::Value::Number(ctx.ToNumber(0) * 2);
    return script::Value::Number(0);
}

// Custom function: square(n) - squares a number
static script::Value StateTest_Func_Square(script::FunctionContext& ctx)
{
    if (ctx.CheckArgs(1) && ctx.IsNumber(0))
    {
        INT64 n = ctx.ToNumber(0);
        return script::Value::Number(n * n);
    }
    return script::Value::Number(0);
}

class StateTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = TRUE;
        LOG_INFO("Running State Tests...");

        RUN_TEST(allPassed, TestManualRegistration, "Manual function registration");
        RUN_TEST(allPassed, TestGlobalVariables,    "Global variables");
        RUN_TEST(allPassed, TestMinimalSetup,       "Minimal setup");
        RUN_TEST(allPassed, TestStateLifecycle,     "State lifecycle");

        if (allPassed)
            LOG_INFO("All State Tests passed!");
        else
            LOG_ERROR("Some State Tests failed!");

        return allPassed;
    }

private:
    static BOOL TestManualRegistration()
    {
        script::State* L = CreateScriptState();
        // Register ONLY the functions we need - NO standard library
        L->Register("print"_embed, EMBED_FUNC(script::StdLib_Print));
        L->Register("double"_embed, EMBED_FUNC(StateTest_Func_Double));
        L->Register("square"_embed, EMBED_FUNC(StateTest_Func_Square));
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/state/manual_registration.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestGlobalVariables()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        // Set global variables from C++
        L->SetGlobalNumber("PI"_embed, 2, 314);
        L->SetGlobalString("version"_embed, 7, "1.0.0"_embed, 5);
        L->SetGlobalBool("debug"_embed, 5, TRUE);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/state/global_variables.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestMinimalSetup()
    {
        script::State* L = CreateScriptState();
        // Register ONLY print - absolutely minimal
        L->Register("print"_embed, EMBED_FUNC(script::StdLib_Print));
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/state/minimal_setup.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestStateLifecycle()
    {
        // Create multiple states to verify proper lifecycle management
        for (INT32 i = 0; i < 3; i++)
        {
            script::State* L = CreateScriptState();
            L->Register("print"_embed, EMBED_FUNC(script::StdLib_Print));
            if (!RunScriptAndCheckResult(L, L"tests/pil/scripts/state/lifecycle.pil"_embed))
            {
                delete L;
                return FALSE;
            }
            delete L;
        }
        return TRUE;
    }
};
