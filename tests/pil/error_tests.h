#pragma once

#include "pil/pil.h"
#include "tests.h"

// ============================================================================
// ERROR TESTS CLASS
// ============================================================================

class ErrorTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = TRUE;

        LOG_INFO("Running Error Tests...");

        RUN_TEST(allPassed, TestMissingSemicolon, "Missing semicolon error detection");
        RUN_TEST(allPassed, TestUndefinedVariable, "Undefined variable error detection");
        RUN_TEST(allPassed, TestUndefinedFunction, "Undefined function error detection");
        RUN_TEST(allPassed, TestSyntaxErrorInExpression, "Syntax error in expression detection");
        RUN_TEST(allPassed, TestValidScript, "Valid script execution");
        RUN_TEST(allPassed, TestErrorMessageRetrieval, "Error message retrieval");
        RUN_TEST(allPassed, TestBreakOutsideLoop, "Break outside loop error");
        RUN_TEST(allPassed, TestContinueOutsideLoop, "Continue outside loop error");

        if (allPassed)
            LOG_INFO("All Error tests passed!");
        else
            LOG_ERROR("Some Error tests failed!");

        return allPassed;
    }

private:
    static BOOL TestMissingSemicolon()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        // Script with syntax error (missing semicolon) - should return FALSE
        BOOL result = !RunScriptFile(L, L"tests/pil/scripts/error/missing_semicolon.pil"_embed);

        if (result)
        {
            LOG_INFO("    Error detected: %s", L->GetError());
        }

        delete L;
        return result;
    }

    static BOOL TestUndefinedVariable()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        // Script with undefined variable - should return FALSE
        BOOL result = !RunScriptFile(L, L"tests/pil/scripts/error/undefined_variable.pil"_embed);

        if (result)
        {
            LOG_INFO("    Error detected: %s", L->GetError());
        }

        delete L;
        return result;
    }

    static BOOL TestUndefinedFunction()
    {
        script::State* L = CreateScriptState();
        // Note: NOT registering any functions, not even print

        // Script calling undefined function - should return FALSE
        BOOL result = !RunScriptFile(L, L"tests/pil/scripts/error/undefined_function.pil"_embed);

        if (result)
        {
            LOG_INFO("    Error detected: %s", L->GetError());
        }

        delete L;
        return result;
    }

    static BOOL TestSyntaxErrorInExpression()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        // Script with invalid expression - should return FALSE
        BOOL result = !RunScriptFile(L, L"tests/pil/scripts/error/syntax_error.pil"_embed);

        if (result)
        {
            LOG_INFO("    Error detected: %s", L->GetError());
        }

        delete L;
        return result;
    }

    static BOOL TestValidScript()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        // Valid script should succeed and set result=true
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/error/valid_script.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestErrorMessageRetrieval()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        // Script with error
        RunScriptFile(L, L"tests/pil/scripts/error/error_message.pil"_embed);

        // GetError should return a non-empty string
        const CHAR* error = L->GetError();
        BOOL result = (error != nullptr && error[0] != '\0');

        if (result)
        {
            LOG_INFO("    Retrieved error: %s", error);
        }

        delete L;
        return result;
    }

    static BOOL TestBreakOutsideLoop()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        // Script with break outside of loop - should return FALSE
        BOOL result = !RunScriptFile(L, L"tests/pil/scripts/error/break_outside_loop.pil"_embed);

        if (result)
        {
            LOG_INFO("    Error detected: %s", L->GetError());
        }

        delete L;
        return result;
    }

    static BOOL TestContinueOutsideLoop()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        // Script with continue outside of loop - should return FALSE
        BOOL result = !RunScriptFile(L, L"tests/pil/scripts/error/continue_outside_loop.pil"_embed);

        if (result)
        {
            LOG_INFO("    Error detected: %s", L->GetError());
        }

        delete L;
        return result;
    }
};
