#pragma once

#include "runtime.h"

class Uint64Tests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		Logger::Info<WCHAR>(L"Running UINT64 Tests..."_embed);

		// Test 1: Construction and accessors
		if (!TestConstruction())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Construction"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Construction"_embed);
		}

		// Test 2: Addition
		if (!TestAddition())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Addition"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Addition"_embed);
		}

		// Test 3: Subtraction
		if (!TestSubtraction())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Subtraction"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Subtraction"_embed);
		}

		// Test 4: Multiplication
		if (!TestMultiplication())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Multiplication"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Multiplication"_embed);
		}

		// Test 5: Division
		if (!TestDivision())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Division"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Division"_embed);
		}

		// Test 6: Modulo
		if (!TestModulo())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Modulo"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Modulo"_embed);
		}

		// Test 7: Bitwise operations
		if (!TestBitwise())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Bitwise operations"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Bitwise operations"_embed);
		}

		// Test 8: Shift operations
		if (!TestShifts())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Shift operations"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Shift operations"_embed);
		}

		// Test 9: Comparisons
		if (!TestComparisons())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Comparisons"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Comparisons"_embed);
		}

		// Test 10: Increment/Decrement
		if (!TestIncrementDecrement())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Increment/Decrement"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Increment/Decrement"_embed);
		}

		// Test 11: Overflow behavior
		if (!TestOverflow())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Overflow behavior"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Overflow behavior"_embed);
		}

		if (allPassed)
		{
			Logger::Info<WCHAR>(L"All UINT64 tests passed!"_embed);
		}
		else
		{
			Logger::Error<WCHAR>(L"Some UINT64 tests failed!"_embed);
		}

		return allPassed;
	}

private:
	static BOOL TestConstruction()
	{
		// Default constructor
		UINT64 a;
		if (a.High() != 0 || a.Low() != 0)
			return FALSE;

		// Two-arg constructor
		UINT64 b(0x12345678, 0x9ABCDEF0);
		if (b.High() != 0x12345678 || b.Low() != 0x9ABCDEF0)
			return FALSE;

		// Single UINT32 constructor
		UINT64 c(UINT32(0xDEADBEEF));
		if (c.High() != 0 || c.Low() != 0xDEADBEEF)
			return FALSE;

		// Native unsigned long long constructor
		UINT64 d(0x123456789ABCDEF0ULL);
		if (d.High() != 0x12345678 || d.Low() != 0x9ABCDEF0)
			return FALSE;

		// Conversion to unsigned long long
		unsigned long long ull = (unsigned long long)d;
		if (ull != 0x123456789ABCDEF0ULL)
			return FALSE;

		return TRUE;
	}

	static BOOL TestAddition()
	{
		// Simple addition (no carry)
		UINT64 a(0, 100);
		UINT64 b(0, 50);
		UINT64 c = a + b;
		if (c.High() != 0 || c.Low() != 150)
			return FALSE;

		// Addition with carry from low to high
		UINT64 d(0, 0xFFFFFFFF);
		UINT64 e(0, 1);
		UINT64 f = d + e;
		if (f.High() != 1 || f.Low() != 0)
			return FALSE;

		// Large number addition
		UINT64 g(0x00000001, 0x00000000);
		UINT64 h(0x00000001, 0x00000000);
		UINT64 i = g + h;
		if (i.High() != 0x00000002 || i.Low() != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestSubtraction()
	{
		// Simple subtraction
		UINT64 a(0, 150);
		UINT64 b(0, 50);
		UINT64 c = a - b;
		if (c.High() != 0 || c.Low() != 100)
			return FALSE;

		// Subtraction with borrow
		UINT64 d(1, 0);
		UINT64 e(0, 1);
		UINT64 f = d - e;
		if (f.High() != 0 || f.Low() != 0xFFFFFFFF)
			return FALSE;

		// Larger subtraction
		UINT64 g(2, 0);
		UINT64 h(1, 1);
		UINT64 i = g - h;
		if (i.High() != 0 || i.Low() != 0xFFFFFFFF)
			return FALSE;

		return TRUE;
	}

	static BOOL TestMultiplication()
	{
		// Simple multiplication
		UINT64 a(0, 10);
		UINT64 b(0, 20);
		UINT64 c = a * b;
		if (c.High() != 0 || c.Low() != 200)
			return FALSE;

		// Multiplication producing high word
		UINT64 d(0, 0x10000);
		UINT64 e(0, 0x10000);
		UINT64 f = d * e;
		if (f.High() != 1 || f.Low() != 0)
			return FALSE;

		// Multiply by 1
		UINT64 g(0x12345678, 0x9ABCDEF0);
		UINT64 one(0, 1);
		UINT64 h = g * one;
		if (h.High() != g.High() || h.Low() != g.Low())
			return FALSE;

		// Multiply by 0
		UINT64 zero(0, 0);
		UINT64 i = g * zero;
		if (i.High() != 0 || i.Low() != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestDivision()
	{
		// Simple division
		UINT64 a(0, 100);
		UINT64 b(0, 10);
		UINT64 c = a / b;
		if (c.High() != 0 || c.Low() != 10)
			return FALSE;

		// Division with high word
		UINT64 d(1, 0); // 0x100000000
		UINT64 e(0, 2);
		UINT64 f = d / e;
		if (f.High() != 0 || f.Low() != 0x80000000)
			return FALSE;

		// Divide by 1
		UINT64 g(0x12345678, 0x9ABCDEF0);
		UINT64 one(0, 1);
		UINT64 h = g / one;
		if (h.High() != g.High() || h.Low() != g.Low())
			return FALSE;

		// Divide by self
		UINT64 i = g / g;
		if (i.High() != 0 || i.Low() != 1)
			return FALSE;

		// Division by zero returns 0
		UINT64 zero(0, 0);
		UINT64 j = g / zero;
		if (j.High() != 0 || j.Low() != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestModulo()
	{
		// Simple modulo
		UINT64 a(0, 100);
		UINT64 b(0, 30);
		UINT64 c = a % b;
		if (c.High() != 0 || c.Low() != 10)
			return FALSE;

		// Modulo with no remainder
		UINT64 d(0, 100);
		UINT64 e(0, 10);
		UINT64 f = d % e;
		if (f.High() != 0 || f.Low() != 0)
			return FALSE;

		// Modulo with large numbers
		UINT64 g(1, 0); // 0x100000000
		UINT64 h(0, 3);
		UINT64 i = g % h;
		// 0x100000000 % 3 = 4294967296 % 3 = 1
		if (i.High() != 0 || i.Low() != 1)
			return FALSE;

		return TRUE;
	}

	static BOOL TestBitwise()
	{
		UINT64 a(0xF0F0F0F0, 0x0F0F0F0F);
		UINT64 b(0xFF00FF00, 0x00FF00FF);

		// AND
		UINT64 c = a & b;
		if (c.High() != 0xF000F000 || c.Low() != 0x000F000F)
			return FALSE;

		// OR
		UINT64 d = a | b;
		if (d.High() != 0xFFF0FFF0 || d.Low() != 0x0FFF0FFF)
			return FALSE;

		// XOR
		UINT64 e = a ^ b;
		if (e.High() != 0x0FF00FF0 || e.Low() != 0x0FF00FF0)
			return FALSE;

		// NOT
		UINT64 f(0, 0);
		UINT64 g = ~f;
		if (g.High() != 0xFFFFFFFF || g.Low() != 0xFFFFFFFF)
			return FALSE;

		return TRUE;
	}

	static BOOL TestShifts()
	{
		// Left shift within low word
		UINT64 a(0, 1);
		UINT64 b = a << 4;
		if (b.High() != 0 || b.Low() != 16)
			return FALSE;

		// Left shift from low to high
		UINT64 c(0, 0x80000000);
		UINT64 d = c << 1;
		if (d.High() != 1 || d.Low() != 0)
			return FALSE;

		// Left shift by 32 (moves low to high entirely)
		UINT64 e(0, 0x12345678);
		UINT64 f = e << 32;
		if (f.High() != 0x12345678 || f.Low() != 0)
			return FALSE;

		// Right shift within low word
		UINT64 g(0, 16);
		UINT64 h = g >> 4;
		if (h.High() != 0 || h.Low() != 1)
			return FALSE;

		// Right shift from high to low
		UINT64 i(1, 0);
		UINT64 j = i >> 1;
		if (j.High() != 0 || j.Low() != 0x80000000)
			return FALSE;

		// Right shift by 32 (moves high to low entirely)
		UINT64 k(0x12345678, 0);
		UINT64 l = k >> 32;
		if (l.High() != 0 || l.Low() != 0x12345678)
			return FALSE;

		// Shift by 0 should not change value
		UINT64 m(0xABCD, 0x1234);
		if ((m << 0).High() != 0xABCD || (m << 0).Low() != 0x1234)
			return FALSE;
		if ((m >> 0).High() != 0xABCD || (m >> 0).Low() != 0x1234)
			return FALSE;

		// Shift by >= 64 should return 0
		UINT64 n(0xFFFFFFFF, 0xFFFFFFFF);
		if ((n << 64).High() != 0 || (n << 64).Low() != 0)
			return FALSE;
		if ((n >> 64).High() != 0 || (n >> 64).Low() != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestComparisons()
	{
		UINT64 a(0, 100);
		UINT64 b(0, 100);
		UINT64 c(0, 200);
		UINT64 d(1, 0);

		// Equality
		if (!(a == b))
			return FALSE;
		if (a != b)
			return FALSE;
		if (a == c)
			return FALSE;

		// Less than
		if (!(a < c))
			return FALSE;
		if (c < a)
			return FALSE;
		if (!(a < d))
			return FALSE; // High word matters more

		// Less than or equal
		if (!(a <= b))
			return FALSE;
		if (!(a <= c))
			return FALSE;
		if (c <= a)
			return FALSE;

		// Greater than
		if (!(c > a))
			return FALSE;
		if (a > c)
			return FALSE;
		if (!(d > a))
			return FALSE;

		// Greater than or equal
		if (!(a >= b))
			return FALSE;
		if (!(c >= a))
			return FALSE;
		if (a >= c)
			return FALSE;

		return TRUE;
	}

	static BOOL TestIncrementDecrement()
	{
		// Prefix increment
		UINT64 a(0, 5);
		++a;
		if (a.High() != 0 || a.Low() != 6)
			return FALSE;

		// Postfix increment
		UINT64 b(0, 5);
		UINT64 c = b++;
		if (c.High() != 0 || c.Low() != 5)
			return FALSE;
		if (b.High() != 0 || b.Low() != 6)
			return FALSE;

		// Prefix decrement
		UINT64 d(0, 5);
		--d;
		if (d.High() != 0 || d.Low() != 4)
			return FALSE;

		// Postfix decrement
		UINT64 e(0, 5);
		UINT64 f = e--;
		if (f.High() != 0 || f.Low() != 5)
			return FALSE;
		if (e.High() != 0 || e.Low() != 4)
			return FALSE;

		// Increment with carry
		UINT64 g(0, 0xFFFFFFFF);
		++g;
		if (g.High() != 1 || g.Low() != 0)
			return FALSE;

		// Decrement with borrow
		UINT64 h(1, 0);
		--h;
		if (h.High() != 0 || h.Low() != 0xFFFFFFFF)
			return FALSE;

		return TRUE;
	}

	static BOOL TestOverflow()
	{
		// Addition overflow wraps around
		UINT64 maxVal(0xFFFFFFFF, 0xFFFFFFFF);
		UINT64 one(0, 1);
		UINT64 overflow = maxVal + one;
		if (overflow.High() != 0 || overflow.Low() != 0)
			return FALSE;

		// Subtraction underflow wraps around
		UINT64 zero(0, 0);
		UINT64 underflow = zero - one;
		if (underflow.High() != 0xFFFFFFFF || underflow.Low() != 0xFFFFFFFF)
			return FALSE;

		return TRUE;
	}
};
