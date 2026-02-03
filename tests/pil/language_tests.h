#pragma once

#include "pil/pil.h"
#include "tests.h"

// ============================================================================
// LANGUAGE TESTS CLASS
// ============================================================================

class LanguageTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = TRUE;

        LOG_INFO("Running Language Tests...");

        RUN_TEST(allPassed, TestRecursion, "Recursive functions");
        RUN_TEST(allPassed, TestArithmeticOperators, "Arithmetic operators");
        RUN_TEST(allPassed, TestComparisonOperators, "Comparison operators");
        RUN_TEST(allPassed, TestLogicalOperators, "Logical operators");
        RUN_TEST(allPassed, TestWhileLoop, "While loop");
        RUN_TEST(allPassed, TestAssignmentOperators, "Assignment operators");
        RUN_TEST(allPassed, TestStringConcatenation, "String concatenation");
        RUN_TEST(allPassed, TestNestedFunctions, "Nested functions");
        RUN_TEST(allPassed, TestFirstClassFunctions, "First-class functions");
        RUN_TEST(allPassed, TestArrayLiterals, "Array literals");
        RUN_TEST(allPassed, TestArrayAccess, "Array access and assignment");
        RUN_TEST(allPassed, TestArrayPushPop, "Array push and pop");
        RUN_TEST(allPassed, TestStringIndexing, "String indexing");
        RUN_TEST(allPassed, TestForEachArray, "For-each over arrays");
        RUN_TEST(allPassed, TestForEachString, "For-each over strings");
        RUN_TEST(allPassed, TestForEachWithIndex, "For-each with index");
        RUN_TEST(allPassed, TestBreakInWhile, "Break in while loop");
        RUN_TEST(allPassed, TestContinueInWhile, "Continue in while loop");
        RUN_TEST(allPassed, TestBreakInForEach, "Break in for-each loop");
        RUN_TEST(allPassed, TestContinueInForEach, "Continue in for-each loop");
        RUN_TEST(allPassed, TestNestedLoopBreak, "Break in nested loops");

        if (allPassed)
            LOG_INFO("All Language tests passed!");
        else
            LOG_ERROR("Some Language tests failed!");

        return allPassed;
    }

private:
    static BOOL TestRecursion()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/recursion.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestArithmeticOperators()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/arithmetic_operators.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestComparisonOperators()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/comparison_operators.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestLogicalOperators()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/logical_operators.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestWhileLoop()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/while_loop.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestAssignmentOperators()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/assignment_operators.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestStringConcatenation()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/string_concatenation.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestNestedFunctions()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/nested_functions.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestFirstClassFunctions()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/first_class_functions.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestArrayLiterals()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/array_literals.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestArrayAccess()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/array_access.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestArrayPushPop()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/array_push_pop.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestStringIndexing()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/string_indexing.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestForEachArray()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/for_each_array.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestForEachString()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/for_each_string.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestForEachWithIndex()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/for_each_with_index.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestBreakInWhile()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/break_in_while.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestContinueInWhile()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/continue_in_while.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestBreakInForEach()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/break_in_for_each.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestContinueInForEach()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/continue_in_for_each.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestNestedLoopBreak()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/language/nested_loop_break.pil"_embed);
        delete L;
        return result;
    }
};
