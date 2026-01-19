#pragma once

#include "runtime.h"

class Int64Tests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		Logger::Info<WCHAR>(L"Running INT64 Tests..."_embed);

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

		// Test 2: Sign extension
		if (!TestSignExtension())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Sign extension"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Sign extension"_embed);
		}

		// Test 3: Addition
		if (!TestAddition())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Addition"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Addition"_embed);
		}

		// Test 4: Subtraction
		if (!TestSubtraction())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Subtraction"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Subtraction"_embed);
		}

		// Test 5: Unary negation
		if (!TestNegation())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Unary negation"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Unary negation"_embed);
		}

		// Test 6: Multiplication
		if (!TestMultiplication())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Multiplication"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Multiplication"_embed);
		}

		// Test 7: Division
		if (!TestDivision())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Division"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Division"_embed);
		}

		// Test 8: Modulo
		if (!TestModulo())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Modulo"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Modulo"_embed);
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

		// Test 10: Shift operations
		if (!TestShifts())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Shift operations"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Shift operations"_embed);
		}

		// Test 11: Increment/Decrement
		if (!TestIncrementDecrement())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Increment/Decrement"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Increment/Decrement"_embed);
		}

		// Test 12: Array initialization and formatting
		if (!TestArrayFormatting())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Array formatting"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Array formatting"_embed);
		}

		// Test 13: Type casting between INT64 and UINT64
		if (!TestTypeCasting())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Type casting"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Type casting"_embed);
		}

		if (allPassed)
		{
			Logger::Info<WCHAR>(L"All INT64 tests passed!"_embed);
		}
		else
		{
			Logger::Error<WCHAR>(L"Some INT64 tests failed!"_embed);
		}

		return allPassed;
	}

private:
	static BOOL TestConstruction()
	{
		// Default constructor
		INT64 a;
		if (a.High() != 0 || a.Low() != 0)
			return FALSE;

		// Two-arg constructor (positive)
		INT64 b(0x12345678, 0x9ABCDEF0);
		if (b.High() != 0x12345678 || b.Low() != 0x9ABCDEF0)
			return FALSE;

		// Positive INT32 constructor
		INT64 c(100);
		if (c.High() != 0 || c.Low() != 100)
			return FALSE;

		// Negative INT32 constructor (sign extension)
		INT64 d(-1);
		if (d.High() != -1 || d.Low() != 0xFFFFFFFF)
			return FALSE;

		// Native signed long long constructor
		INT64 e(0x123456789ABCDEF0LL);
		if (e.High() != 0x12345678 || e.Low() != 0x9ABCDEF0)
			return FALSE;

		// Conversion to signed long long
		signed long long sll = (signed long long)e;
		if (sll != 0x123456789ABCDEF0LL)
			return FALSE;

		return TRUE;
	}

	static BOOL TestSignExtension()
	{
		// Positive value has high = 0
		INT64 pos(42);
		if (pos.High() != 0)
			return FALSE;

		// Negative value has high = -1 (all 1s)
		INT64 neg(-42);
		if (neg.High() != -1)
			return FALSE;

		// Zero
		INT64 zero(0);
		if (zero.High() != 0 || zero.Low() != 0)
			return FALSE;

		// INT32_MIN
		INT64 minVal((INT32)0x80000000);
		if (minVal.High() != -1)
			return FALSE;

		// INT32_MAX
		INT64 maxVal(0x7FFFFFFF);
		if (maxVal.High() != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestAddition()
	{
		// Simple positive addition
		INT64 a(50);
		INT64 b(30);
		INT64 c = a + b;
		if (c.High() != 0 || c.Low() != 80)
			return FALSE;

		// Negative + Positive
		INT64 d(-10);
		INT64 e(30);
		INT64 f = d + e;
		if (f.High() != 0 || f.Low() != 20)
			return FALSE;

		// Negative + Negative
		INT64 g(-10);
		INT64 h(-20);
		INT64 i = g + h;
		// -30 = 0xFFFFFFFF FFFFFFE2
		if (i.High() != -1 || i.Low() != (UINT32)-30)
			return FALSE;

		// Addition with carry
		INT64 j(0, 0xFFFFFFFF);
		INT64 k(0, 1);
		INT64 l = j + k;
		if (l.High() != 1 || l.Low() != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestSubtraction()
	{
		// Simple subtraction
		INT64 a(100);
		INT64 b(30);
		INT64 c = a - b;
		if (c.High() != 0 || c.Low() != 70)
			return FALSE;

		// Result negative
		INT64 d(30);
		INT64 e(100);
		INT64 f = d - e;
		// -70 = 0xFFFFFFFF FFFFFFBA
		if (f.High() != -1 || f.Low() != (UINT32)-70)
			return FALSE;

		// Subtract negative (adds)
		INT64 g(50);
		INT64 h(-30);
		INT64 i = g - h;
		if (i.High() != 0 || i.Low() != 80)
			return FALSE;

		return TRUE;
	}

	static BOOL TestNegation()
	{
		// Negate positive
		INT64 a(42);
		INT64 b = -a;
		if (b.High() != -1 || b.Low() != (UINT32)-42)
			return FALSE;

		// Negate negative
		INT64 c(-42);
		INT64 d = -c;
		if (d.High() != 0 || d.Low() != 42)
			return FALSE;

		// Negate zero
		INT64 zero(0);
		INT64 negZero = -zero;
		if (negZero.High() != 0 || negZero.Low() != 0)
			return FALSE;

		// Double negation
		INT64 e(123);
		INT64 f = -(-e);
		if (f.High() != 0 || f.Low() != 123)
			return FALSE;

		return TRUE;
	}

	static BOOL TestMultiplication()
	{
		// Simple multiplication
		INT64 a(10);
		INT64 b(20);
		INT64 c = a * b;
		if (c.High() != 0 || c.Low() != 200)
			return FALSE;

		// Positive * Negative
		INT64 d(10);
		INT64 e(-5);
		INT64 f = d * e;
		// -50 = 0xFFFFFFFF FFFFFFCE
		if (f.High() != -1 || f.Low() != (UINT32)-50)
			return FALSE;

		// Negative * Negative
		INT64 g(-10);
		INT64 h(-5);
		INT64 i = g * h;
		if (i.High() != 0 || i.Low() != 50)
			return FALSE;

		// Multiply by 0
		INT64 j(12345);
		INT64 zero(0);
		INT64 k = j * zero;
		if (k.High() != 0 || k.Low() != 0)
			return FALSE;

		// Multiply by 1
		INT64 one(1);
		INT64 l = j * one;
		if (l.High() != 0 || l.Low() != 12345)
			return FALSE;

		return TRUE;
	}

	static BOOL TestDivision()
	{
		// Simple division
		INT64 a(100);
		INT64 b(10);
		INT64 c = a / b;
		if (c.High() != 0 || c.Low() != 10)
			return FALSE;

		// Negative / Positive
		INT64 d(-100);
		INT64 e(10);
		INT64 f = d / e;
		if (f.High() != -1 || f.Low() != (UINT32)-10)
			return FALSE;

		// Positive / Negative
		INT64 g(100);
		INT64 h(-10);
		INT64 i = g / h;
		if (i.High() != -1 || i.Low() != (UINT32)-10)
			return FALSE;

		// Negative / Negative
		INT64 j(-100);
		INT64 k(-10);
		INT64 l = j / k;
		if (l.High() != 0 || l.Low() != 10)
			return FALSE;

		// Division by 1
		INT64 m(42);
		INT64 one(1);
		INT64 n = m / one;
		if (n.High() != 0 || n.Low() != 42)
			return FALSE;

		// Division by zero returns 0
		INT64 zero(0);
		INT64 o = m / zero;
		if (o.High() != 0 || o.Low() != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestModulo()
	{
		// Simple modulo
		INT64 a(100);
		INT64 b(30);
		INT64 c = a % b;
		if (c.High() != 0 || c.Low() != 10)
			return FALSE;

		// Modulo with no remainder
		INT64 d(100);
		INT64 e(10);
		INT64 f = d % e;
		if (f.High() != 0 || f.Low() != 0)
			return FALSE;

		// Negative modulo
		INT64 g(-100);
		INT64 h(30);
		INT64 i = g % h;
		// -100 % 30 = -10
		if (i.High() != -1 || i.Low() != (UINT32)-10)
			return FALSE;

		return TRUE;
	}

	static BOOL TestComparisons()
	{
		INT64 pos(100);
		INT64 neg(-100);
		INT64 zero(0);
		INT64 pos2(100);

		// Equality
		if (!(pos == pos2))
			return FALSE;
		if (pos == neg)
			return FALSE;
		if (pos != pos2)
			return FALSE;

		// Less than (signed comparison)
		if (!(neg < pos))
			return FALSE; // -100 < 100
		if (pos < neg)
			return FALSE;
		if (!(neg < zero))
			return FALSE; // -100 < 0
		if (!(zero < pos))
			return FALSE; // 0 < 100

		// Less than or equal
		if (!(pos <= pos2))
			return FALSE;
		if (!(neg <= pos))
			return FALSE;
		if (pos <= neg)
			return FALSE;

		// Greater than
		if (!(pos > neg))
			return FALSE;
		if (neg > pos)
			return FALSE;
		if (!(pos > zero))
			return FALSE;
		if (!(zero > neg))
			return FALSE;

		// Greater than or equal
		if (!(pos >= pos2))
			return FALSE;
		if (!(pos >= neg))
			return FALSE;
		if (neg >= pos)
			return FALSE;

		return TRUE;
	}

	static BOOL TestShifts()
	{
		// Left shift positive
		INT64 a(1);
		INT64 b = a << 4;
		if (b.High() != 0 || b.Low() != 16)
			return FALSE;

		// Left shift to high word
		INT64 c(0, 0x80000000);
		INT64 d = c << 1;
		if (d.High() != 1 || d.Low() != 0)
			return FALSE;

		// Right shift positive (logical for positive)
		INT64 e(16);
		INT64 f = e >> 2;
		if (f.High() != 0 || f.Low() != 4)
			return FALSE;

		// Right shift negative (arithmetic shift - sign extension)
		INT64 g(-16);
		INT64 h = g >> 2;
		// Arithmetic right shift preserves sign
		if (h.High() != -1 || h.Low() != (UINT32)-4)
			return FALSE;

		// Shift by 0
		INT64 i(42);
		if ((i << 0).Low() != 42 || (i >> 0).Low() != 42)
			return FALSE;

		return TRUE;
	}

	static BOOL TestIncrementDecrement()
	{
		// Prefix increment
		INT64 a(5);
		++a;
		if (a.High() != 0 || a.Low() != 6)
			return FALSE;

		// Postfix increment
		INT64 b(5);
		INT64 c = b++;
		if (c.High() != 0 || c.Low() != 5)
			return FALSE;
		if (b.High() != 0 || b.Low() != 6)
			return FALSE;

		// Prefix decrement
		INT64 d(5);
		--d;
		if (d.High() != 0 || d.Low() != 4)
			return FALSE;

		// Postfix decrement
		INT64 e(5);
		INT64 f = e--;
		if (f.High() != 0 || f.Low() != 5)
			return FALSE;
		if (e.High() != 0 || e.Low() != 4)
			return FALSE;

		// Increment negative toward zero
		INT64 g(-1);
		++g;
		if (g.High() != 0 || g.Low() != 0)
			return FALSE;

		// Decrement zero to negative
		INT64 h(0);
		--h;
		if (h.High() != -1 || h.Low() != 0xFFFFFFFF)
			return FALSE;

		return TRUE;
	}

	static BOOL TestArrayFormatting()
	{
		// Test that INT64 arrays can be properly initialized and formatted
		// This ensures the varargs casting works correctly with Logger
		INT64 testArray[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };

		// Verify array initialization
		for (INT64 i = 0; i < 20; i++)
		{
			if (testArray[(signed long long)i] != (i + INT64(1)))
				return FALSE;
		}

		// Test formatting output (this also tests that the values are properly passed through varargs)
		for (INT64 i = 0; i < 20; i++)
		{
			// The Logger::Info call exercises the varargs casting
			Logger::Info<WCHAR>(L"    INT64 Array Value [%lld]: %lld"_embed, (signed long long)i, (signed long long)testArray[(signed long long)i]);
		}

		return TRUE;
	}

	static BOOL TestTypeCasting()
	{
		// Test INT64 to UINT64 conversion (implicit via operator)
		INT64 signedPos(12345);
		UINT64 unsignedFromPos = (UINT64)signedPos;
		if (unsignedFromPos.High() != 0 || unsignedFromPos.Low() != 12345)
			return FALSE;

		// Test INT64 negative to UINT64 (preserves bit representation)
		INT64 signedNeg(-1);
		UINT64 unsignedFromNeg = (UINT64)signedNeg;
		if (unsignedFromNeg.High() != 0xFFFFFFFF || unsignedFromNeg.Low() != 0xFFFFFFFF)
			return FALSE;

		// Test UINT64 to INT64 conversion (explicit constructor)
		UINT64 unsignedVal(0x12345678, 0x9ABCDEF0);
		INT64 signedFromUnsigned = INT64(unsignedVal);
		if (signedFromUnsigned.High() != (INT32)0x12345678 || signedFromUnsigned.Low() != 0x9ABCDEF0)
			return FALSE;

		// Test roundtrip: INT64 -> UINT64 -> INT64
		INT64 original(42);
		UINT64 intermediate = (UINT64)original;
		INT64 roundtrip = INT64(intermediate);
		if (roundtrip.High() != original.High() || roundtrip.Low() != original.Low())
			return FALSE;

		// Test roundtrip with negative value
		INT64 negOriginal(-42);
		UINT64 negIntermediate = (UINT64)negOriginal;
		INT64 negRoundtrip = INT64(negIntermediate);
		if (negRoundtrip.High() != negOriginal.High() || negRoundtrip.Low() != negOriginal.Low())
			return FALSE;

		// Test edge case: INT64::MAX
		INT64 maxSigned = INT64::MAX();
		UINT64 maxAsUnsigned = (UINT64)maxSigned;
		if (maxAsUnsigned.High() != 0x7FFFFFFF || maxAsUnsigned.Low() != 0xFFFFFFFF)
			return FALSE;

		// Test edge case: INT64::MIN
		INT64 minSigned = INT64::MIN();
		UINT64 minAsUnsigned = (UINT64)minSigned;
		if (minAsUnsigned.High() != 0x80000000 || minAsUnsigned.Low() != 0x00000000)
			return FALSE;

		// Test UINT64::MAX to INT64 (will be negative when interpreted as signed)
		UINT64 maxUnsigned = UINT64::MAX();
		INT64 maxAsNegative = INT64(maxUnsigned);
		if (maxAsNegative.High() != -1 || maxAsNegative.Low() != 0xFFFFFFFF)
			return FALSE;

		// Test zero conversion both ways
		INT64 zeroSigned(0);
		UINT64 zeroUnsigned(0ULL);
		UINT64 zeroToUnsigned = (UINT64)zeroSigned;
		INT64 zeroToSigned = INT64(zeroUnsigned);
		if (zeroToUnsigned.High() != 0 || zeroToUnsigned.Low() != 0)
			return FALSE;
		if (zeroToSigned.High() != 0 || zeroToSigned.Low() != 0)
			return FALSE;

		return TRUE;
	}
};
