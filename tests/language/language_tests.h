#pragma once

#include "test_framework.h"

// ============================================================================
// LANGUAGE TESTS
// ============================================================================

class LanguageTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = TRUE;
        LOG_INFO("Running Language Tests...");

        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/recursion.pil"_embed,            "Recursive functions",       CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/arithmetic_operators.pil"_embed, "Arithmetic operators",      CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/comparison_operators.pil"_embed, "Comparison operators",      CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/logical_operators.pil"_embed,    "Logical operators",         CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/while_loop.pil"_embed,           "While loop",                CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/assignment_operators.pil"_embed, "Assignment operators",      CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/string_concatenation.pil"_embed, "String concatenation",      CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/nested_functions.pil"_embed,     "Nested functions",          CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/first_class_functions.pil"_embed,"First-class functions",     CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/array_literals.pil"_embed,       "Array literals",            CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/array_access.pil"_embed,         "Array access and assignment", CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/array_push_pop.pil"_embed,       "Array push and pop",        CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/string_indexing.pil"_embed,      "String indexing",           CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/for_each_array.pil"_embed,       "For-each over arrays",      CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/for_each_string.pil"_embed,      "For-each over strings",     CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/for_each_with_index.pil"_embed,  "For-each with index",       CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/break_in_while.pil"_embed,       "Break in while loop",       CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/continue_in_while.pil"_embed,    "Continue in while loop",    CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/break_in_for_each.pil"_embed,    "Break in for-each loop",    CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/continue_in_for_each.pil"_embed, "Continue in for-each loop", CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/language/nested_loop_break.pil"_embed,    "Break in nested loops",     CFG_STDLIB);

        if (allPassed)
            LOG_INFO("All Language Tests passed!");
        else
            LOG_ERROR("Some Language Tests failed!");

        return allPassed;
    }
};
