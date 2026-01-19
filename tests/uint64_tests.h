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

		// Test 12: Type casting between UINT64 and INT64
		if (!TestTypeCasting())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Type casting"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Type casting"_embed);
		}

		// Test 13: Compound assignment operators
		if (!TestCompoundAssignments())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Compound assignments"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Compound assignments"_embed);
		}

		// Test 14: Scalar arithmetic operations
		if (!TestScalarArithmetic())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Scalar arithmetic"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Scalar arithmetic"_embed);
		}

		// Test 15: Scalar comparisons
		if (!TestScalarComparisons())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Scalar comparisons"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Scalar comparisons"_embed);
		}

		// Test 16: Bitwise operations with scalars
		if (!TestBitwiseScalars())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Bitwise with scalars"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Bitwise with scalars"_embed);
		}

		// Test 17: Shift assignments and edge cases
		if (!TestShiftAssignments())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Shift assignments"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Shift assignments"_embed);
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

		// Left shift by 32
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

		// Right shift by 32
		UINT64 k(0x12345678, 0);
		UINT64 l = k >> 32;
		if (l.High() != 0 || l.Low() != 0x12345678)
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
		// Addition overflow: MAX + 1 wraps to 0
		UINT64 maxVal = UINT64::MAX();
		UINT64 one(0, 1);
		UINT64 overflow = maxVal + one;
		if (overflow.High() != 0 || overflow.Low() != 0)
			return FALSE;

		// Subtraction underflow: 0 - 1 wraps to MAX
		UINT64 zero(0, 0);
		UINT64 underflow = zero - one;
		if (underflow.High() != 0xFFFFFFFF || underflow.Low() != 0xFFFFFFFF)
			return FALSE;

		// Addition with carry overflow
		UINT64 a(0xFFFFFFFF, 0xFFFFFFFF);
		UINT64 b(0, 2);
		UINT64 c = a + b;
		if (c.High() != 0 || c.Low() != 1)
			return FALSE;

		// Subtraction with borrow underflow
		UINT64 d(0, 0);
		UINT64 e(0, 2);
		UINT64 f = d - e;
		if (f.High() != 0xFFFFFFFF || f.Low() != 0xFFFFFFFE)
			return FALSE;

		// Multiplication overflow
		UINT64 g(0x10000, 0);
		UINT64 h(0x10000, 0);
		UINT64 i = g * h;
		// Result wraps (only keeps lower 64 bits)
		if (i.High() != 0 || i.Low() != 0)
			return FALSE;

		// Increment at MAX
		UINT64 j = UINT64::MAX();
		++j;
		if (j.High() != 0 || j.Low() != 0)
			return FALSE;

		// Decrement at 0
		UINT64 k(0, 0);
		--k;
		if (k.High() != 0xFFFFFFFF || k.Low() != 0xFFFFFFFF)
			return FALSE;

		// Large multiplication overflow
		UINT64 m(0xFFFFFFFF, 0xFFFFFFFF);
		UINT64 n = m * UINT64(2);
		if (n.High() != 0xFFFFFFFF || n.Low() != 0xFFFFFFFE)
			return FALSE;

		// Carry propagation in addition
		UINT64 p(0, 0xFFFFFFFF);
		p += UINT32(1);
		if (p.High() != 1 || p.Low() != 0)
			return FALSE;

		// Borrow propagation in subtraction
		UINT64 q(1, 0);
		q -= UINT32(1);
		if (q.High() != 0 || q.Low() != 0xFFFFFFFF)
			return FALSE;

		return TRUE;
	}

	static BOOL TestTypeCasting()
	{
		// Test UINT64 to INT64 conversion (explicit constructor)
		UINT64 unsignedSmall(0, 12345);
		INT64 signedFromSmall = INT64(unsignedSmall);
		if (signedFromSmall.High() != 0 || signedFromSmall.Low() != 12345)
			return FALSE;

		// Test UINT64 with high bit set to INT64 (will be negative)
		UINT64 unsignedLarge(0x80000000, 0);
		INT64 signedFromLarge = INT64(unsignedLarge);
		if (signedFromLarge.High() != (INT32)0x80000000 || signedFromLarge.Low() != 0)
			return FALSE;

		// Test INT64 to UINT64 conversion (implicit via operator)
		INT64 signedPos(42);
		UINT64 unsignedFromPos = (UINT64)signedPos;
		if (unsignedFromPos.High() != 0 || unsignedFromPos.Low() != 42)
			return FALSE;

		// Test INT64 negative to UINT64 (preserves bit pattern)
		INT64 signedNeg(-1);
		UINT64 unsignedFromNeg = (UINT64)signedNeg;
		// -1 in INT64 is 0xFFFFFFFF_FFFFFFFF which equals UINT64::MAX
		if (unsignedFromNeg.High() != 0xFFFFFFFF || unsignedFromNeg.Low() != 0xFFFFFFFF)
			return FALSE;

		// Test roundtrip: UINT64 -> INT64 -> UINT64
		UINT64 original(0x12345678, 0x9ABCDEF0);
		INT64 intermediate = INT64(original);
		UINT64 roundtrip = (UINT64)intermediate;
		if (roundtrip.High() != original.High() || roundtrip.Low() != original.Low())
			return FALSE;

		// Test edge case: UINT64::MAX to INT64 (becomes -1)
		UINT64 maxUnsigned = UINT64::MAX();
		INT64 maxAsSigned = INT64(maxUnsigned);
		if (maxAsSigned.High() != -1 || maxAsSigned.Low() != 0xFFFFFFFF)
			return FALSE;

		// Test edge case: Half of UINT64 range (INT64::MAX + 1)
		UINT64 halfRange(0x80000000, 0x00000000);
		INT64 halfAsSigned = INT64(halfRange);
		// This is INT64::MIN in two's complement
		if (halfAsSigned.High() != (INT32)0x80000000 || halfAsSigned.Low() != 0)
			return FALSE;

		// Test zero conversion both ways
		UINT64 zeroUnsigned(0ULL);
		INT64 zeroSigned(0);
		INT64 zeroToSigned = INT64(zeroUnsigned);
		UINT64 zeroToUnsigned = (UINT64)zeroSigned;
		if (zeroToSigned.High() != 0 || zeroToSigned.Low() != 0)
			return FALSE;
		if (zeroToUnsigned.High() != 0 || zeroToUnsigned.Low() != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestCompoundAssignments()
	{
		// += operator
		UINT64 a(0, 100);
		a += UINT64(0, 50);
		if (a.High() != 0 || a.Low() != 150)
			return FALSE;

		// += with carry
		UINT64 b(0, 0xFFFFFFFF);
		b += UINT64(0, 2);
		if (b.High() != 1 || b.Low() != 1)
			return FALSE;

		// -= operator
		UINT64 c(0, 200);
		c -= UINT64(0, 50);
		if (c.High() != 0 || c.Low() != 150)
			return FALSE;

		// -= with borrow
		UINT64 d(1, 0);
		d -= UINT64(0, 1);
		if (d.High() != 0 || d.Low() != 0xFFFFFFFF)
			return FALSE;

		// *= operator
		UINT64 e(0, 10);
		e *= UINT64(0, 5);
		if (e.High() != 0 || e.Low() != 50)
			return FALSE;

		// /= operator
		UINT64 f(0, 100);
		f /= UINT64(0, 10);
		if (f.High() != 0 || f.Low() != 10)
			return FALSE;

		// %= operator
		UINT64 g(0, 100);
		g %= UINT64(0, 30);
		if (g.High() != 0 || g.Low() != 10)
			return FALSE;

		// &= operator
		UINT64 h(0xFF, 0xFFFFFFFF);
		h &= UINT64(0x0F, 0x0000FFFF);
		if (h.High() != 0x0F || h.Low() != 0x0000FFFF)
			return FALSE;

		// |= operator
		UINT64 i(0xF0, 0xF0F0F0F0);
		i |= UINT64(0x0F, 0x0F0F0F0F);
		if (i.High() != 0xFF || i.Low() != 0xFFFFFFFF)
			return FALSE;

		// ^= operator
		UINT64 j(0xFF, 0xFFFFFFFF);
		j ^= UINT64(0xFF, 0xFFFFFFFF);
		if (j.High() != 0 || j.Low() != 0)
			return FALSE;

		// Compound assignment with UINT32
		UINT64 k(0, 100);
		k += UINT32(50);
		if (k.High() != 0 || k.Low() != 150)
			return FALSE;

		UINT64 l(0, 100);
		l -= UINT32(50);
		if (l.High() != 0 || l.Low() != 50)
			return FALSE;

		UINT64 m(0, 10);
		m *= UINT32(5);
		if (m.High() != 0 || m.Low() != 50)
			return FALSE;

		UINT64 n(0, 100);
		n /= UINT32(10);
		if (n.High() != 0 || n.Low() != 10)
			return FALSE;

		UINT64 o(0, 100);
		o %= UINT32(30);
		if (o.High() != 0 || o.Low() != 10)
			return FALSE;

		return TRUE;
	}

	static BOOL TestScalarArithmetic()
	{
		// operator+(UINT32)
		UINT64 a(0, 100);
		UINT64 b = a + UINT32(50);
		if (b.High() != 0 || b.Low() != 150)
			return FALSE;

		// operator+(UINT32) with carry
		UINT64 c(0, 0xFFFFFFFF);
		UINT64 d = c + UINT32(2);
		if (d.High() != 1 || d.Low() != 1)
			return FALSE;

		// operator+(int)
		UINT64 e(0, 100);
		UINT64 f = e + 50;
		if (f.High() != 0 || f.Low() != 150)
			return FALSE;

		// operator-(UINT32)
		UINT64 g(0, 100);
		UINT64 h = g - UINT32(30);
		if (h.High() != 0 || h.Low() != 70)
			return FALSE;

		// operator-(UINT32) with borrow
		UINT64 i(1, 0);
		UINT64 j = i - UINT32(1);
		if (j.High() != 0 || j.Low() != 0xFFFFFFFF)
			return FALSE;

		// operator*(UINT32)
		UINT64 k(0, 10);
		UINT64 l = k * UINT32(5);
		if (l.High() != 0 || l.Low() != 50)
			return FALSE;

		// operator*(UINT32) with overflow to high word
		UINT64 m(0, 0x10000);
		UINT64 n = m * UINT32(0x10000);
		if (n.High() != 1 || n.Low() != 0)
			return FALSE;

		// operator/(UINT32)
		UINT64 o(0, 100);
		UINT64 p = o / UINT32(10);
		if (p.High() != 0 || p.Low() != 10)
			return FALSE;

		// operator/(int)
		UINT64 q(0, 100);
		UINT64 r = q / 10;
		if (r.High() != 0 || r.Low() != 10)
			return FALSE;

		// operator%(UINT32)
		UINT64 s(0, 100);
		UINT64 t = s % UINT32(30);
		if (t.High() != 0 || t.Low() != 10)
			return FALSE;

		// operator%(int)
		UINT64 u(0, 100);
		UINT64 v = u % 30;
		if (v.High() != 0 || v.Low() != 10)
			return FALSE;

		return TRUE;
	}

	static BOOL TestScalarComparisons()
	{
		UINT64 small(0, 100);
		UINT64 large(1, 0);

		// operator<(UINT32)
		if (!(small < UINT32(200)))
			return FALSE;
		if (small < UINT32(50))
			return FALSE;
		if (large < UINT32(100))  // large has high word set
			return FALSE;

		// operator<=(UINT32)
		if (!(small <= UINT32(100)))
			return FALSE;
		if (!(small <= UINT32(200)))
			return FALSE;
		if (small <= UINT32(50))
			return FALSE;

		// operator>(UINT32)
		if (!(small > UINT32(50)))
			return FALSE;
		if (small > UINT32(200))
			return FALSE;
		if (!(large > UINT32(100)))
			return FALSE;

		// operator>=(UINT32)
		if (!(small >= UINT32(100)))
			return FALSE;
		if (!(small >= UINT32(50)))
			return FALSE;
		if (small >= UINT32(200))
			return FALSE;

		// operator==(UINT32)
		if (!(small == UINT32(100)))
			return FALSE;
		if (small == UINT32(99))
			return FALSE;
		if (large == UINT32(0))
			return FALSE;

		// operator!=(UINT32)
		if (!(small != UINT32(99)))
			return FALSE;
		if (small != UINT32(100))
			return FALSE;

		// operator==(int)
		if (!(small == 100))
			return FALSE;
		if (small == 99)
			return FALSE;

		// operator!=(int)
		if (!(small != 99))
			return FALSE;
		if (small != 100)
			return FALSE;

		// operator<(int)
		if (!(small < 200))
			return FALSE;
		if (small < 50)
			return FALSE;

		// operator<=(int)
		if (!(small <= 100))
			return FALSE;
		if (small <= 50)
			return FALSE;

		// operator>(int)
		if (!(small > 50))
			return FALSE;
		if (small > 200)
			return FALSE;

		// operator>=(int)
		if (!(small >= 100))
			return FALSE;
		if (small >= 200)
			return FALSE;

		return TRUE;
	}

	static BOOL TestBitwiseScalars()
	{
		UINT64 a(0xF0F0F0F0, 0x0F0F0F0F);

		// operator&(UINT32)
		UINT64 b = a & UINT32(0x00FF00FF);
		if (b.High() != 0 || b.Low() != 0x000F000F)
			return FALSE;

		// operator&(int)
		UINT64 c = a & 0xFF;
		if (c.High() != 0 || c.Low() != 0x0F)
			return FALSE;

		// operator&(unsigned long long)
		UINT64 d = a & 0xFFFFFFFFULL;
		if (d.High() != 0 || d.Low() != 0x0F0F0F0F)
			return FALSE;

		// operator|(UINT32)
		UINT64 e(0, 0xF0F0F0F0);
		UINT64 f = e | UINT32(0x0F0F0F0F);
		if (f.High() != 0 || f.Low() != 0xFFFFFFFF)
			return FALSE;

		// operator|(unsigned long long)
		UINT64 g(0, 0xF0F0F0F0);
		UINT64 h = g | 0x0F0F0F0FULL;
		if (h.High() != 0 || h.Low() != 0xFFFFFFFF)
			return FALSE;

		// operator^(UINT32)
		UINT64 i(0, 0xFFFFFFFF);
		UINT64 j = i ^ UINT32(0xFFFFFFFF);
		if (j.High() != 0 || j.Low() != 0)
			return FALSE;

		// operator^(unsigned long long)
		UINT64 k(0, 0xAAAAAAAA);
		UINT64 l = k ^ 0x55555555ULL;
		if (l.High() != 0 || l.Low() != 0xFFFFFFFF)
			return FALSE;

		return TRUE;
	}

	static BOOL TestShiftAssignments()
	{
		// <<= operator
		UINT64 a(0, 1);
		a <<= 4;
		if (a.High() != 0 || a.Low() != 16)
			return FALSE;

		// Shift from low to high
		UINT64 b(0, 0x00000001);
		b <<= 32;
		if (b.High() != 1 || b.Low() != 0)
			return FALSE;

		// >>= operator
		UINT64 c(0, 16);
		c >>= 2;
		if (c.High() != 0 || c.Low() != 4)
			return FALSE;

		// Shift from high to low
		UINT64 d(1, 0);
		d >>= 32;
		if (d.High() != 0 || d.Low() != 1)
			return FALSE;

		// Shift by 64 or more
		UINT64 e(0x12345678, 0x9ABCDEF0);
		e <<= 64;
		if (e.High() != 0 || e.Low() != 0)
			return FALSE;

		UINT64 f(0x12345678, 0x9ABCDEF0);
		f >>= 64;
		if (f.High() != 0 || f.Low() != 0)
			return FALSE;

		// Shift by 0 (no change)
		UINT64 g(0x1234, 0x5678);
		g <<= 0;
		if (g.High() != 0x1234 || g.Low() != 0x5678)
			return FALSE;

		g >>= 0;
		if (g.High() != 0x1234 || g.Low() != 0x5678)
			return FALSE;

		// Shift by negative (should result in 0)
		UINT64 h(0, 42);
		h <<= -5;
		if (h.High() != 0 || h.Low() != 0)
			return FALSE;

		UINT64 i(0, 42);
		i >>= -5;
		if (i.High() != 0 || i.Low() != 0)
			return FALSE;

		// Test shift with unsigned long long parameter
		UINT64 j(0, 1);
		UINT64 k = j << 10ULL;
		if (k.High() != 0 || k.Low() != 1024)
			return FALSE;

		UINT64 l(0, 1024);
		UINT64 m = l >> 10ULL;
		if (m.High() != 0 || m.Low() != 1)
			return FALSE;

		return TRUE;
	}
};
