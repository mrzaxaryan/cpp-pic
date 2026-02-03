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
        RUN_TEST(allPassed, TestArrayLiterals, "Array literals");
        RUN_TEST(allPassed, TestArrayAccess, "Array access and assignment");
        RUN_TEST(allPassed, TestArrayPushPop, "Array push and pop");
        RUN_TEST(allPassed, TestStringIndexing, "String indexing");

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

    static BOOL TestArrayLiterals()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(// Empty array
var empty = [];
print("Empty array:", empty);
print("Empty length:", len(empty));

// Array with elements
var nums = [1, 2, 3, 4, 5];
print("Numbers:", nums);
print("Numbers length:", len(nums));

// Mixed types
var mixed = [1, "hello", true, nil];
print("Mixed array:", mixed);

// Array type
print("Type of array:", type(nums));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestArrayAccess()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(var arr = [10, 20, 30, 40, 50];

// Access elements
print("arr[0] =", arr[0]);
print("arr[2] =", arr[2]);
print("arr[4] =", arr[4]);

// Modify elements
arr[0] = 100;
arr[2] = 300;
print("After modification:", arr);

// Compound assignment
arr[1] += 5;
print("arr[1] after += 5:", arr[1]);

// Loop through array
var sum = 0;
for (var i = 0; i < len(arr); i = i + 1) {
    sum = sum + arr[i];
}
print("Sum of array:", sum);
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestArrayPushPop()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(var arr = [1, 2, 3];
print("Initial array:", arr);

// Push elements
push(arr, 4);
push(arr, 5);
print("After push 4, 5:", arr);
print("Length:", len(arr));

// Pop elements
var last = pop(arr);
print("Popped:", last);
print("After pop:", arr);

last = pop(arr);
print("Popped:", last);
print("After pop:", arr);

print("Final length:", len(arr));
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }

    static BOOL TestStringIndexing()
    {
        script::State* L = new script::State();
        script::OpenStdLib(*L);

        auto source = R"(var s = "Hello";

// Access individual characters
print("s[0] =", s[0]);
print("s[1] =", s[1]);
print("s[4] =", s[4]);

// Loop through string
for (var i = 0; i < len(s); i = i + 1) {
    print("char", i, "=", s[i]);
}
)"_embed;

        BOOL result = L->DoString(source);
        delete L;
        return result;
    }
};
