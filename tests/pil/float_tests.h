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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/float/float_literals.pil");
        delete L;
        return result;
    }

    static BOOL TestMixedArithmetic()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/float/mixed_arithmetic.pil");
        delete L;
        return result;
    }

    static BOOL TestFloatComparisons()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/float/float_comparisons.pil");
        delete L;
        return result;
    }

    static BOOL TestDivision()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/float/division.pil");
        delete L;
        return result;
    }

    static BOOL TestModuloRestriction()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        // Modulo with integers should work
        BOOL result1 = RunScriptFile(L, L"tests/pil/scripts/float/modulo_integers.pil");

        // Modulo with floats should fail
        BOOL result2 = RunScriptFile(L, L"tests/pil/scripts/float/modulo_floats.pil");

        delete L;
        // First should pass, second should fail (expect error)
        return result1 && !result2;
    }

    static BOOL TestStrFunction()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/float/str_function.pil");
        delete L;
        return result;
    }

    static BOOL TestNumFunction()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/float/num_function.pil");
        delete L;
        return result;
    }

    static BOOL TestFloorCeilInt()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/float/floor_ceil_int.pil");
        delete L;
        return result;
    }

    static BOOL TestMinMaxAbs()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/float/min_max_abs.pil");
        delete L;
        return result;
    }

    static BOOL TestNegation()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/float/negation.pil");
        delete L;
        return result;
    }
};
