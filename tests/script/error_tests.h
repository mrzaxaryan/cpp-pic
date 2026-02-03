#pragma once

#include "ral/script/script.h"
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

        // Script with syntax error (missing semicolon)
        auto source = R"(var x = 10
print(x);
)"_embed;

        // Should return FALSE due to error
        BOOL result = !L->DoString(source);

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

        // Script with undefined variable
        auto source = R"(print(undefinedVar);
)"_embed;

        // Should return FALSE due to error
        BOOL result = !L->DoString(source);

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

        // Script calling undefined function
        auto source = R"(undefinedFunc(42);
)"_embed;

        // Should return FALSE due to error
        BOOL result = !L->DoString(source);

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

        // Script with invalid expression
        auto source = R"(var x = 5 + + 3;
)"_embed;

        // Should return FALSE due to error
        BOOL result = !L->DoString(source);

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

        // Valid script should succeed
        auto source = R"(var x = 10;
var y = 20;
print("Valid script: x + y =", x + y);
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestErrorMessageRetrieval()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        // Script with error
        auto source = R"(var x = 10
)"_embed;

        L->DoString(source);

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
};
