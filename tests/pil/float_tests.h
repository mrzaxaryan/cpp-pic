#pragma once

#include "pil/pil.h"
#include "tests.h"

// ============================================================================
// FLOAT TESTS CLASS
// ============================================================================

class FloatTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = TRUE;

        LOG_INFO("Running Float Tests...");

        RUN_TEST(allPassed, TestFloatLiterals, "Float literals");
        RUN_TEST(allPassed, TestMixedArithmetic, "Mixed arithmetic");
        RUN_TEST(allPassed, TestFloatComparisons, "Float comparisons");
        RUN_TEST(allPassed, TestDivision, "Division");
        RUN_TEST(allPassed, TestModuloRestriction, "Modulo restriction");
        RUN_TEST(allPassed, TestStrFunction, "str() with floats");
        RUN_TEST(allPassed, TestNumFunction, "num() with floats");
        RUN_TEST(allPassed, TestFloorCeilInt, "floor/ceil/int functions");
        RUN_TEST(allPassed, TestMinMaxAbs, "min/max/abs with floats");
        RUN_TEST(allPassed, TestNegation, "Float negation");

        if (allPassed)
            LOG_INFO("All Float tests passed!");
        else
            LOG_ERROR("Some Float tests failed!");

        return allPassed;
    }

private:
    static BOOL TestFloatLiterals()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/float/float_literals.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestMixedArithmetic()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/float/mixed_arithmetic.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestFloatComparisons()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/float/float_comparisons.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestDivision()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/float/division.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestModuloRestriction()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        // Test integer modulo - should succeed
        BOOL result1 = RunScriptAndCheckResult(L, L"tests/pil/scripts/float/modulo_integers.pil"_embed);

        // Test float modulo - should fail with runtime error (modulo restricted to integers)
        BOOL result2 = !RunScriptFile(L, L"tests/pil/scripts/float/modulo_floats.pil"_embed);

        delete L;
        return result1 && result2;
    }

    static BOOL TestStrFunction()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/float/str_function.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestNumFunction()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/float/num_function.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestFloorCeilInt()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/float/floor_ceil_int.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestMinMaxAbs()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/float/min_max_abs.pil"_embed);
        delete L;
        return result;
    }

    static BOOL TestNegation()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        BOOL result = RunScriptAndCheckResult(L, L"tests/pil/scripts/float/negation.pil"_embed);
        delete L;
        return result;
    }
};
