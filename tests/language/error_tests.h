#pragma once

#include "test_framework.h"

// ============================================================================
// ERROR TESTS
// ============================================================================

class ErrorTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = TRUE;
        LOG_INFO("Running Error Tests...");

        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/error/missing_semicolon.pil"_embed,     "Missing semicolon error detection",    CFG_STDLIB_EXPECT_FAIL);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/error/undefined_variable.pil"_embed,    "Undefined variable error detection",   CFG_STDLIB_EXPECT_FAIL);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/error/syntax_error.pil"_embed,          "Syntax error in expression detection", CFG_STDLIB_EXPECT_FAIL);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/error/valid_script.pil"_embed,          "Valid script execution",               CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/error/break_outside_loop.pil"_embed,    "Break outside loop error",             CFG_STDLIB_EXPECT_FAIL);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/error/continue_outside_loop.pil"_embed, "Continue outside loop error",          CFG_STDLIB_EXPECT_FAIL);

        // Custom tests that need special setup
        RUN_TEST(allPassed, TestUndefinedFunction, "Undefined function error detection");
        RUN_TEST(allPassed, TestErrorMessageRetrieval, "Error message retrieval");

        if (allPassed)
            LOG_INFO("All Error Tests passed!");
        else
            LOG_ERROR("Some Error Tests failed!");

        return allPassed;
    }

private:
    static BOOL TestUndefinedFunction()
    {
        script::State* L = CreateScriptState();
        // Note: NOT registering any functions, not even print
        BOOL result = !RunScriptFile(L, L"tests/language/scripts/error/undefined_function.pil"_embed);
        if (result)
            LOG_INFO("    Error detected: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestErrorMessageRetrieval()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        // Script with error
        RunScriptFile(L, L"tests/language/scripts/error/error_message.pil"_embed);

        // GetError should return a non-empty string
        const CHAR* error = L->GetError();
        BOOL result = (error != nullptr && error[0] != '\0');

        if (result)
            LOG_INFO("    Retrieved error: %s", error);

        delete L;
        return result;
    }
};
