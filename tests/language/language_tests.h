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

        RunScriptTest(allPassed, L"tests/language/scripts/language/recursion.pil"_embed,            L"Recursive functions"_embed,         CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/arithmetic_operators.pil"_embed, L"Arithmetic operators"_embed,        CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/comparison_operators.pil"_embed, L"Comparison operators"_embed,        CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/logical_operators.pil"_embed,    L"Logical operators"_embed,           CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/while_loop.pil"_embed,           L"While loop"_embed,                  CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/assignment_operators.pil"_embed, L"Assignment operators"_embed,        CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/string_concatenation.pil"_embed, L"String concatenation"_embed,        CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/nested_functions.pil"_embed,     L"Nested functions"_embed,            CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/first_class_functions.pil"_embed,L"First-class functions"_embed,       CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/array_literals.pil"_embed,       L"Array literals"_embed,              CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/array_access.pil"_embed,         L"Array access and assignment"_embed, CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/array_push_pop.pil"_embed,       L"Array push and pop"_embed,          CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/string_indexing.pil"_embed,      L"String indexing"_embed,             CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/for_each_array.pil"_embed,       L"For-each over arrays"_embed,        CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/for_each_string.pil"_embed,      L"For-each over strings"_embed,       CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/for_each_with_index.pil"_embed,  L"For-each with index"_embed,         CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/break_in_while.pil"_embed,       L"Break in while loop"_embed,         CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/continue_in_while.pil"_embed,    L"Continue in while loop"_embed,      CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/break_in_for_each.pil"_embed,    L"Break in for-each loop"_embed,      CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/continue_in_for_each.pil"_embed, L"Continue in for-each loop"_embed,   CFG_STDLIB);
        RunScriptTest(allPassed, L"tests/language/scripts/language/nested_loop_break.pil"_embed,    L"Break in nested loops"_embed,       CFG_STDLIB);

        if (allPassed)
            LOG_INFO("All Language Tests passed!");
        else
            LOG_ERROR("Some Language Tests failed!");

        return allPassed;
    }
};
