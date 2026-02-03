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

        auto source = R"(
var pi = 3.14159;
var half = 0.5;
var big = 1234.5678;
print("pi =", pi);
print("half =", half);
print("big =", big);
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestMixedArithmetic()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        auto source = R"(
print("42 + 3.14 =", 42 + 3.14);
print("10 * 0.5 =", 10 * 0.5);
print("7.5 - 2 =", 7.5 - 2);
print("100 / 2.5 =", 100 / 2.5);
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestFloatComparisons()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        auto source = R"(
print("3.14 > 3 =", 3.14 > 3);
print("2.5 < 3 =", 2.5 < 3);
print("1.0 == 1 =", 1.0 == 1);
print("1.5 != 1 =", 1.5 != 1);
print("2.5 >= 2.5 =", 2.5 >= 2.5);
print("2.4 <= 2.5 =", 2.4 <= 2.5);
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestDivision()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        auto source = R"(
print("7 / 2 =", 7 / 2);
print("10 / 4 =", 10 / 4);
print("1 / 3 =", 1 / 3);
print("22 / 7 =", 22 / 7);
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestModuloRestriction()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        // Modulo with integers should work
        auto source1 = R"(
print("17 % 5 =", 17 % 5);
print("10 % 3 =", 10 % 3);
)"_embed;

        BOOL result1 = L->DoString(source1);

        // Modulo with floats should fail
        auto source2 = R"(
print("7.5 % 2 =", 7.5 % 2);
)"_embed;

        BOOL result2 = L->DoString(source2);

        delete L;
        // First should pass, second should fail (expect error)
        return result1 && !result2;
    }

    static BOOL TestStrFunction()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        auto source = R"(
print("str(3.14159) =", str(3.14159));
print("str(42) =", str(42));
print("str(0.5) =", str(0.5));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestNumFunction()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        auto source = R"(
print("num(\"3.14\") =", num("3.14"));
print("num(\"42\") =", num("42"));
print("num(\"-2.5\") =", num("-2.5"));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestFloorCeilInt()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        auto source = R"(
print("floor(3.7) =", floor(3.7));
print("floor(-3.2) =", floor(-3.2));
print("ceil(3.2) =", ceil(3.2));
print("ceil(-3.7) =", ceil(-3.7));
print("int(3.7) =", int(3.7));
print("int(-3.7) =", int(-3.7));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestMinMaxAbs()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        auto source = R"(
print("min(3.14, 2.71) =", min(3.14, 2.71));
print("max(3.14, 2.71) =", max(3.14, 2.71));
print("abs(-3.14) =", abs(-3.14));
print("abs(2.71) =", abs(2.71));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestNegation()
    {
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);

        auto source = R"(
var x = 3.14;
print("-x =", -x);
print("-(-x) =", -(-x));
print("-(1.5 + 0.5) =", -(1.5 + 0.5));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }
};
