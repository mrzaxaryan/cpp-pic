#pragma once

#include "ral/script/script.h"
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

        if (allPassed)
            LOG_INFO("All Language tests passed!");
        else
            LOG_ERROR("Some Language tests failed!");

        return allPassed;
    }

private:
    static BOOL TestRecursion()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(fn factorial(n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
for (var i = 1; i <= 10; i = i + 1) {
    print("factorial(", i, ") =", factorial(i));
}
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestArithmeticOperators()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(print("5 + 3 =", 5 + 3);
print("10 - 4 =", 10 - 4);
print("6 * 7 =", 6 * 7);
print("20 / 4 =", 20 / 4);
print("17 % 5 =", 17 % 5);
print("-5 =", -5);
print("-(3 + 2) =", -(3 + 2));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestComparisonOperators()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(print("5 == 5:", 5 == 5);
print("5 != 3:", 5 != 3);
print("5 < 10:", 5 < 10);
print("5 > 3:", 5 > 3);
print("5 <= 5:", 5 <= 5);
print("5 >= 5:", 5 >= 5);
print("hello == hello:", "hello" == "hello");
print("hello != world:", "hello" != "world");
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestLogicalOperators()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(print("true && true:", true && true);
print("true && false:", true && false);
print("true || false:", true || false);
print("false || false:", false || false);
print("!true:", !true);
print("!false:", !false);
print("(5 > 3) && (2 < 4):", (5 > 3) && (2 < 4));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestWhileLoop()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(var i = 0;
var sum = 0;
while (i < 5) {
    sum = sum + i;
    i = i + 1;
}
print("Sum of 0..4 =", sum);
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestAssignmentOperators()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(var x = 10;
print("x =", x);
x += 5;
print("x += 5:", x);
x -= 3;
print("x -= 3:", x);
x *= 2;
print("x *= 2:", x);
x /= 4;
print("x /= 4:", x);
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestStringConcatenation()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(var greeting = "Hello";
var name = "World";
print(greeting + ", " + name + "!");
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestNestedFunctions()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(fn outer(x) {
    fn inner(y) {
        return y * 2;
    }
    return inner(x) + 1;
}
print("outer(5) =", outer(5));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestFirstClassFunctions()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(fn apply(f, x) {
    return f(x);
}
fn double(n) {
    return n * 2;
}
print("apply(double, 5) =", apply(double, 5));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }
};
