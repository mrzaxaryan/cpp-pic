#pragma once

#include "ral/script/script.h"
#include "pal/io/logger.h"

// ============================================================================
// CUSTOM C++ FUNCTIONS FOR STATE TESTS
// ============================================================================

// Custom function: double(n) - doubles a number
static script::Value StateTest_Func_Double(script::FunctionContext& ctx)
{
    if (ctx.CheckArgs(1) && ctx.IsNumber(0))
    {
        return script::Value::Number(ctx.ToNumber(0) * 2);
    }
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

// ============================================================================
// STATE TESTS CLASS
// ============================================================================

class StateTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = TRUE;

        LOG_INFO("Running State Tests...");

        // Test 1: Manual function registration
        if (!TestManualRegistration())
        {
            allPassed = FALSE;
            LOG_ERROR("  FAILED: Manual function registration");
        }
        else
        {
            LOG_INFO("  PASSED: Manual function registration");
        }

        // Test 2: Global variables
        if (!TestGlobalVariables())
        {
            allPassed = FALSE;
            LOG_ERROR("  FAILED: Global variables");
        }
        else
        {
            LOG_INFO("  PASSED: Global variables");
        }

        // Test 3: Minimal setup (print only)
        if (!TestMinimalSetup())
        {
            allPassed = FALSE;
            LOG_ERROR("  FAILED: Minimal setup");
        }
        else
        {
            LOG_INFO("  PASSED: Minimal setup");
        }

        // Test 4: State creation and deletion
        if (!TestStateLifecycle())
        {
            allPassed = FALSE;
            LOG_ERROR("  FAILED: State lifecycle");
        }
        else
        {
            LOG_INFO("  PASSED: State lifecycle");
        }

        if (allPassed)
        {
            LOG_INFO("All State tests passed!");
        }
        else
        {
            LOG_ERROR("Some State tests failed!");
        }

        return allPassed;
    }

private:
    static BOOL TestManualRegistration()
    {
        script::State* L = new script::State();

        // Register ONLY the functions we need - NO standard library
        L->Register("print"_embed, EMBED_FUNC(script::StdLib_Print) );
        L->Register("double"_embed, EMBED_FUNC(StateTest_Func_Double) );
        L->Register("square"_embed, EMBED_FUNC(StateTest_Func_Square) );

        auto source = R"(print("Only print, double, square are available");
print("double(5) =", double(5));
print("square(4) =", square(4));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestGlobalVariables()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        // Set global variables from C++
        L->SetGlobalNumber("PI"_embed, 2, 314);
        L->SetGlobalString("version"_embed, 7, "1.0.0"_embed, 5);
        L->SetGlobalBool("debug"_embed, 5, TRUE);

        auto source = R"(print("PI (x100) =", PI);
print("Version:", version);
if (debug) {
    print("Debug mode is ON");
}
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestMinimalSetup()
    {
        script::State* L = new script::State();

        // Register ONLY print - absolutely minimal
        L->Register("print"_embed, EMBED_FUNC(script::StdLib_Print) );

        auto source = R"(var x = 10;
var y = 20;
print("x + y =", x + y);
print("x * y =", x * y);
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestStateLifecycle()
    {
        // Create multiple states to verify proper lifecycle management
        for (INT32 i = 0; i < 3; i++)
        {
            script::State* L = new script::State();
            L->Register("print"_embed, EMBED_FUNC(script::StdLib_Print) );

            auto source = R"(var x = 42;
print("State test iteration");
)"_embed;

            if (!L->DoString(source))
            {
                delete L;
                return FALSE;
            }
            delete L;
        }
        return TRUE;
    }
};
