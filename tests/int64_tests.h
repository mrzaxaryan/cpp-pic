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

		// Test 12: Type casting between INT64 and UINT64
		if (!TestTypeCasting())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Type casting"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Type casting"_embed);
		}

		// Test 14: Bitwise operations
		if (!TestBitwise())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Bitwise operations"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Bitwise operations"_embed);
		}

		// Test 15: Compound assignment operators
		if (!TestCompoundAssignments())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Compound assignments"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Compound assignments"_embed);
		}

		// Test 16: Scalar arithmetic operations
		if (!TestScalarArithmetic())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Scalar arithmetic"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Scalar arithmetic"_embed);
		}

		// Test 17: Scalar comparisons
		if (!TestScalarComparisons())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Scalar comparisons"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Scalar comparisons"_embed);
		}

		// Test 18: Shift assignments and edge cases
		if (!TestShiftAssignments())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Shift assignments"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Shift assignments"_embed);
		}

		// Test 19: Overflow behavior
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

		// Right shift positive
		INT64 e(16);
		INT64 f = e >> 2;
		if (f.High() != 0 || f.Low() != 4)
			return FALSE;

		// Right shift negative (arithmetic shift - sign extension)
		INT64 g(-16);
		INT64 h = g >> 2;
		if (h.High() != -1 || h.Low() != (UINT32)-4)
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

	static BOOL TestBitwise()
	{
		INT64 a(0x12345678, 0x9ABCDEF0);
		INT64 b(0xF0F0F0F0, 0x0F0F0F0F);

		// AND
		INT64 c = a & b;
		if (c.High() != 0x10305070 || c.Low() != 0x0A0C0E00)
			return FALSE;

		// OR
		INT64 d = a | b;
		if (d.High() != (INT32)0xF2F4F6F8 || d.Low() != 0x9FBFDFFF)
			return FALSE;

		// XOR
		INT64 e = a ^ b;
		if (e.High() != (INT32)0xE2C4A688 || e.Low() != 0x95B3D1FF)
			return FALSE;

		// NOT
		INT64 f(0x12345678, 0x9ABCDEF0);
		INT64 g = ~f;
		if (g.High() != (INT32)0xEDCBA987 || g.Low() != 0x6543210F)
			return FALSE;

		// Bitwise with scalar (INT32)
		INT64 h(0x00000000, 0xFFFFFFFF);
		INT64 i = h & INT64(0x0000FFFF);
		if (i.High() != 0 || i.Low() != 0x0000FFFF)
			return FALSE;

		return TRUE;
	}

	static BOOL TestCompoundAssignments()
	{
		// += operator
		INT64 a(100);
		a += INT64(50);
		if (a.High() != 0 || a.Low() != 150)
			return FALSE;

		// -= operator
		INT64 b(200);
		b -= INT64(50);
		if (b.High() != 0 || b.Low() != 150)
			return FALSE;

		// *= operator
		INT64 c(10);
		c *= INT64(5);
		if (c.High() != 0 || c.Low() != 50)
			return FALSE;

		// /= operator
		INT64 d(100);
		d /= INT64(10);
		if (d.High() != 0 || d.Low() != 10)
			return FALSE;

		// %= operator
		INT64 e(100);
		e %= INT64(30);
		if (e.High() != 0 || e.Low() != 10)
			return FALSE;

		// &= operator
		INT64 f(0xFF, 0xFFFFFFFF);
		f &= INT64(0x0F, 0x0000FFFF);
		if (f.High() != 0x0F || f.Low() != 0x0000FFFF)
			return FALSE;

		// |= operator
		INT64 g(0xF0, 0xF0F0F0F0);
		g |= INT64(0x0F, 0x0F0F0F0F);
		if (g.High() != 0xFF || g.Low() != 0xFFFFFFFF)
			return FALSE;

		// ^= operator
		INT64 h(0xFF, 0xFFFFFFFF);
		h ^= INT64(0xFF, 0xFFFFFFFF);
		if (h.High() != 0 || h.Low() != 0)
			return FALSE;

		// Compound assignment with INT32
		INT64 j(100);
		j += 50;
		if (j.High() != 0 || j.Low() != 150)
			return FALSE;

		INT64 k(100);
		k -= 50;
		if (k.High() != 0 || k.Low() != 50)
			return FALSE;

		INT64 l(10);
		l *= 5;
		if (l.High() != 0 || l.Low() != 50)
			return FALSE;

		INT64 m(100);
		m /= 10;
		if (m.High() != 0 || m.Low() != 10)
			return FALSE;

		INT64 n(100);
		n %= 30;
		if (n.High() != 0 || n.Low() != 10)
			return FALSE;

		// Compound assignment with UINT64
		INT64 p(100);
		p += UINT64(50);
		if (p.High() != 0 || p.Low() != 150)
			return FALSE;

		INT64 q(100);
		q -= UINT64(50);
		if (q.High() != 0 || q.Low() != 50)
			return FALSE;

		return TRUE;
	}

	static BOOL TestScalarArithmetic()
	{
		// operator+(INT32)
		INT64 a(100);
		INT64 b = a + 50;
		if (b.High() != 0 || b.Low() != 150)
			return FALSE;

		// operator-(INT32)
		INT64 c(100);
		INT64 d = c - 30;
		if (d.High() != 0 || d.Low() != 70)
			return FALSE;

		// operator*(INT32)
		INT64 e(10);
		INT64 f = e * 5;
		if (f.High() != 0 || f.Low() != 50)
			return FALSE;

		// operator/(INT32)
		INT64 g(100);
		INT64 h = g / 10;
		if (h.High() != 0 || h.Low() != 10)
			return FALSE;

		// operator%(INT32)
		INT64 i(100);
		INT64 j = i % 30;
		if (j.High() != 0 || j.Low() != 10)
			return FALSE;

		// Test with negative INT32
		INT64 k(100);
		INT64 l = k + (-50);
		if (l.High() != 0 || l.Low() != 50)
			return FALSE;

		INT64 m(50);
		INT64 n = m - (-50);
		if (n.High() != 0 || n.Low() != 100)
			return FALSE;

		INT64 o(10);
		INT64 p = o * (-5);
		if (p.High() != -1 || p.Low() != (UINT32)-50)
			return FALSE;

		return TRUE;
	}

	static BOOL TestScalarComparisons()
	{
		INT64 pos(100);
		INT64 neg(-100);

		// operator<(INT32)
		if (!(neg < 50))
			return FALSE;
		if (pos < 50)
			return FALSE;

		// operator<=(INT32)
		if (!(pos <= 100))
			return FALSE;
		if (!(neg <= 0))
			return FALSE;
		if (pos <= 50)
			return FALSE;

		// operator>(INT32)
		if (!(pos > 50))
			return FALSE;
		if (neg > 0)
			return FALSE;

		// operator>=(INT32)
		if (!(pos >= 100))
			return FALSE;
		if (!(pos >= 50))
			return FALSE;
		if (neg >= 0)
			return FALSE;

		// operator==(INT32)
		if (!(pos == 100))
			return FALSE;
		if (pos == 99)
			return FALSE;

		// operator!=(INT32)
		if (!(pos != 99))
			return FALSE;
		if (pos != 100)
			return FALSE;

		return TRUE;
	}

	static BOOL TestShiftAssignments()
	{
		// <<= operator
		INT64 a(1);
		a <<= 4;
		if (a.High() != 0 || a.Low() != 16)
			return FALSE;

		// Shift from low to high
		INT64 b(0, 0x00000001);
		b <<= 32;
		if (b.High() != 1 || b.Low() != 0)
			return FALSE;

		// >>= operator
		INT64 c(16);
		c >>= 2;
		if (c.High() != 0 || c.Low() != 4)
			return FALSE;

		// Arithmetic right shift with negative
		INT64 d(-16);
		d >>= 2;
		if (d.High() != -1 || d.Low() != (UINT32)-4)
			return FALSE;

		// Shift by 64 or more (should result in 0 for left, sign-extended for right)
		INT64 e(0x12345678, 0x9ABCDEF0);
		e <<= 64;
		if (e.High() != 0 || e.Low() != 0)
			return FALSE;

		// Right shift by >= 64 sign-extends: positive becomes 0
		INT64 f(0x12345678, 0x9ABCDEF0);
		f >>= 64;
		if (f.High() != 0 || f.Low() != 0)
			return FALSE;

		// Negative value shifted by >= 64
		INT64 g(-100);
		g >>= 64;
		if (g.High() != -1 || g.Low() != 0xFFFFFFFF)
			return FALSE;

		// Shift by 0 (no change)
		INT64 h(42);
		h <<= 0;
		if (h.High() != 0 || h.Low() != 42)
			return FALSE;

		h >>= 0;
		if (h.High() != 0 || h.Low() != 42)
			return FALSE;

		// Shift by negative (should be no-op or return 0)
		INT64 i(42);
		i <<= -5;
		if (i.High() != 0 || i.Low() != 0)
			return FALSE;

		INT64 j(42);
		j >>= -5;
		if (j.High() != 0 || j.Low() != 42)
			return FALSE;

		return TRUE;
	}

	static BOOL TestOverflow()
	{
		// Test MAX + 1 overflow
		INT64 maxVal = INT64::MAX();
		INT64 result = maxVal + INT64(1);
		// 0x7FFFFFFFFFFFFFFF + 1 = 0x8000000000000000 (MIN)
		if (result.High() != (INT32)0x80000000 || result.Low() != 0)
			return FALSE;

		// Test MIN - 1 underflow
		INT64 minVal = INT64::MIN();
		result = minVal - INT64(1);
		// 0x8000000000000000 - 1 = 0x7FFFFFFFFFFFFFFF (MAX)
		if (result.High() != 0x7FFFFFFF || result.Low() != 0xFFFFFFFF)
			return FALSE;

		// Test addition overflow with carry
		INT64 a(0x7FFFFFFF, 0xFFFFFFFF); // MAX
		INT64 b(0, 2);
		INT64 c = a + b;
		// Should wrap to negative
		if (c.High() != (INT32)0x80000000 || c.Low() != 1)
			return FALSE;

		// Test subtraction underflow with borrow
		INT64 d((INT32)0x80000000, 0); // MIN
		INT64 e(0, 2);
		INT64 f = d - e;
		// Should wrap to positive
		if (f.High() != 0x7FFFFFFF || f.Low() != 0xFFFFFFFE)
			return FALSE;

		// Test multiplication overflow
		INT64 g = INT64::MAX();
		INT64 h = g * INT64(2);
		// Overflow in multiplication: 0x7FFFFFFFFFFFFFFF * 2 = 0xFFFFFFFFFFFFFFFE
		if (h.High() != -1 || h.Low() != 0xFFFFFFFE)
			return FALSE;

		// Test increment at MAX
		INT64 i = INT64::MAX();
		++i;
		if (i.High() != (INT32)0x80000000 || i.Low() != 0)
			return FALSE;

		// Test decrement at MIN
		INT64 j = INT64::MIN();
		--j;
		if (j.High() != 0x7FFFFFFF || j.Low() != 0xFFFFFFFF)
			return FALSE;

		// Test negation of MIN (wraps to MIN since -MIN > MAX)
		INT64 k = INT64::MIN();
		INT64 negK = -k;
		if (negK.High() != (INT32)0x80000000 || negK.Low() != 0)
			return FALSE;

		return TRUE;
	}
};
