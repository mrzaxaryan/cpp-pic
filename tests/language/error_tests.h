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

        RunScriptTest(allPassed, L"tests/language/scripts/error/missing_semicolon.pil"_embed,     L"Missing semicolon error detection"_embed,    CFG_STDLIB_EXPECT_FAIL);
        RunScriptTest(allPassed, L"tests/language/scripts/error/undefined_variable.pil"_embed,    L"Undefined variable error detection"_embed,   CFG_STDLIB_EXPECT_FAIL);
        RunScriptTest(allPassed, L"tests/language/scripts/error/syntax_error.pil"_embed,          L"Syntax error in expression detection"_embed, CFG_STDLIB_EXPECT_FAIL);
        RunScriptTest(allPassed, L"tests/language/scripts/error/valid_script.pil"_embed,          L"Valid script execution"_embed,               CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/error/break_outside_loop.pil"_embed,    L"Break outside loop error"_embed,             CFG_STDLIB_EXPECT_FAIL);
        RunScriptTest(allPassed, L"tests/language/scripts/error/continue_outside_loop.pil"_embed, L"Continue outside loop error"_embed,          CFG_STDLIB_EXPECT_FAIL);

        // Custom tests that need special setup
        RunTest(allPassed, EMBED_FUNC(TestUndefinedFunction), L"Undefined function error detection"_embed);
        RunTest(allPassed, EMBED_FUNC(TestErrorMessageRetrieval), L"Error message retrieval"_embed);

        if (allPassed)
            LOG_INFO("All Error Tests passed!");
        else
            LOG_ERROR("Some Error Tests failed!");

        return allPassed;
    }

private:
    static BOOL TestUndefinedFunction()
    {
        PIL::State* L = CreateScriptState();
        // Note: NOT registering any functions, not even print
        BOOL result = !RunScriptFile(L, L"tests/language/scripts/error/undefined_function.pil"_embed);
        if (result)
            LOG_INFO("    Error detected: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestErrorMessageRetrieval()
    {
        PIL::State* L = CreateScriptState();
        PIL::OpenStdLib(*L);

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
