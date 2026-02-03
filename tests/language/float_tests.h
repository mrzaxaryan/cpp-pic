#pragma once

#include "test_framework.h"

// ============================================================================
// FLOAT TESTS
// ============================================================================

class FloatTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = TRUE;
        LOG_INFO("Running Float Tests...");

        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/float/float_literals.pil"_embed,    "Float literals",           CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/float/mixed_arithmetic.pil"_embed,  "Mixed arithmetic",         CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/float/float_comparisons.pil"_embed, "Float comparisons",        CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/float/division.pil"_embed,          "Division",                 CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/float/modulo_integers.pil"_embed,   "Integer modulo",           CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/float/modulo_floats.pil"_embed,     "Float modulo (error)",     CFG_STDLIB_EXPECT_FAIL);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/float/str_function.pil"_embed,      "str() with floats",        CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/float/num_function.pil"_embed,      "num() with floats",        CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/float/floor_ceil_int.pil"_embed,    "floor/ceil/int functions", CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/float/min_max_abs.pil"_embed,       "min/max/abs with floats",  CFG_STDLIB);
        RUN_SCRIPT_TEST(allPassed, L"tests/language/scripts/float/negation.pil"_embed,          "Float negation",           CFG_STDLIB);

        if (allPassed)
            LOG_INFO("All Float Tests passed!");
        else
            LOG_ERROR("Some Float Tests failed!");

        return allPassed;
    }
};
