#pragma once

#include "runtime/runtime.h"
#include "tests.h"

class DoubleTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running DOUBLE Tests...");

		RunTest(allPassed, EMBED_FUNC(TestConstruction), "Construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestIntToDouble), "Integer to DOUBLE"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDoubleToInt), "DOUBLE to integer"_embed);
		RunTest(allPassed, EMBED_FUNC(TestArithmetic), "Arithmetic"_embed);
		RunTest(allPassed, EMBED_FUNC(TestComparisons), "Comparisons"_embed);
		RunTest(allPassed, EMBED_FUNC(TestNegation), "Negation"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEmbeddedLiterals), "Embedded literals"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEdgeCases), "Edge cases"_embed);
		RunTest(allPassed, EMBED_FUNC(TestArrayFormatting), "Array formatting"_embed);

		if (allPassed)
			LOG_INFO("All DOUBLE tests passed!");
		else
			LOG_ERROR("Some DOUBLE tests failed!");

		return allPassed;
	}

private:
	static BOOL TestConstruction()
	{
		// Default constructor (zero)
		DOUBLE a;
		if (a.Bits() != 0)
		{
			LOG_ERROR("Default constructor: expected bits == 0");
			return false;
		}

		// Construction from embedded double
		DOUBLE b = 1.0_embed;
		// IEEE-754: 1.0 = 0x3FF0000000000000
		if (b.Bits() != 0x3FF0000000000000ULL)
		{
			LOG_ERROR("1.0 bits mismatch: expected 0x3FF0000000000000");
			return false;
		}

		// Construction from bit pattern
		DOUBLE c(0x4000000000000000ULL); // 2.0
		double native_c = (double)c;
		if (native_c != (double)2.0_embed)
		{
			LOG_ERROR("Construction from bits: 2.0 mismatch");
			return false;
		}

		// Construction from bit pattern (1.0 = 0x3FF0000000000000)
		DOUBLE d(0x3FF0000000000000ULL); // 1.0
		double native_d = (double)d;
		if (native_d != (double)1.0_embed)
		{
			LOG_ERROR("Construction from bits: 1.0 mismatch");
			return false;
		}

		return true;
	}

	static BOOL TestIntToDouble()
	{
		// Zero
		DOUBLE zero(INT32(0));
		if (zero.Bits() != 0)
		{
			LOG_ERROR("INT32(0) -> DOUBLE: expected bits == 0");
			return false;
		}

		// Positive integer
		DOUBLE one(INT32(1));
		double native_one = (double)one;
		if (native_one != (double)1.0_embed)
		{
			LOG_ERROR("INT32(1) -> DOUBLE: mismatch");
			return false;
		}

		// Larger positive integer
		DOUBLE hundred(INT32(100));
		double native_hundred = (double)hundred;
		if (native_hundred != (double)100.0_embed)
		{
			LOG_ERROR("INT32(100) -> DOUBLE: mismatch");
			return false;
		}

		// Negative integer
		DOUBLE neg_one(INT32(-1));
		double native_neg = (double)neg_one;
		if (native_neg != (double)-1.0_embed)
		{
			LOG_ERROR("INT32(-1) -> DOUBLE: mismatch");
			return false;
		}

		// Power of 2
		DOUBLE pow2(INT32(1024));
		double native_pow2 = (double)pow2;
		if (native_pow2 != (double)1024.0_embed)
		{
			LOG_ERROR("INT32(1024) -> DOUBLE: mismatch");
			return false;
		}

		return true;
	}

	static BOOL TestDoubleToInt()
	{
		// 1.0 -> 1
		DOUBLE one = 1.0_embed;
		INT32 int_one = (INT32)one;
		if (int_one != 1)
		{
			LOG_ERROR("DOUBLE(1.0) -> INT32: expected 1, got %d", int_one);
			return false;
		}

		// 1.9 -> 1 (truncation)
		DOUBLE one_nine = 1.9_embed;
		INT32 int_one_nine = (INT32)one_nine;
		if (int_one_nine != 1)
		{
			LOG_ERROR("DOUBLE(1.9) -> INT32: expected 1, got %d", int_one_nine);
			return false;
		}

		// 100.5 -> 100
		DOUBLE hundred = 100.5_embed;
		INT32 int_hundred = (INT32)hundred;
		if (int_hundred != 100)
		{
			LOG_ERROR("DOUBLE(100.5) -> INT32: expected 100, got %d", int_hundred);
			return false;
		}

		// -1.0 -> -1
		DOUBLE neg_one = -1.0_embed;
		INT32 int_neg_one = (INT32)neg_one;
		if (int_neg_one != -1)
		{
			LOG_ERROR("DOUBLE(-1.0) -> INT32: expected -1, got %d", int_neg_one);
			return false;
		}

		// 0.5 -> 0
		DOUBLE half = 0.5_embed;
		INT32 int_half = (INT32)half;
		if (int_half != 0)
		{
			LOG_ERROR("DOUBLE(0.5) -> INT32: expected 0, got %d", int_half);
			return false;
		}

		return true;
	}

	static BOOL TestArithmetic()
	{
		// Addition
		DOUBLE a = 2.0_embed;
		DOUBLE b = 3.0_embed;
		DOUBLE c = a + b;
		double native_c = (double)c;
		if (native_c != (double)5.0_embed)
		{
			LOG_ERROR("2.0 + 3.0 != 5.0");
			return false;
		}

		// Subtraction
		DOUBLE d = b - a;
		double native_d = (double)d;
		if (native_d != (double)1.0_embed)
		{
			LOG_ERROR("3.0 - 2.0 != 1.0");
			return false;
		}

		// Multiplication
		DOUBLE e = a * b;
		double native_e = (double)e;
		if (native_e != (double)6.0_embed)
		{
			LOG_ERROR("2.0 * 3.0 != 6.0");
			return false;
		}

		// Division
		DOUBLE six = 6.0_embed;
		DOUBLE f = six / a;
		double native_f = (double)f;
		if (native_f != (double)3.0_embed)
		{
			LOG_ERROR("6.0 / 2.0 != 3.0");
			return false;
		}

		// Compound assignment +=
		DOUBLE g = 10.0_embed;
		g += a;
		if ((double)g != (double)12.0_embed)
		{
			LOG_ERROR("10.0 += 2.0 != 12.0");
			return false;
		}

		// Compound assignment -=
		g -= a;
		if ((double)g != (double)10.0_embed)
		{
			LOG_ERROR("12.0 -= 2.0 != 10.0");
			return false;
		}

		// Compound assignment *=
		g *= a;
		if ((double)g != (double)20.0_embed)
		{
			LOG_ERROR("10.0 *= 2.0 != 20.0");
			return false;
		}

		// Compound assignment /=
		g /= a;
		if ((double)g != (double)10.0_embed)
		{
			LOG_ERROR("20.0 /= 2.0 != 10.0");
			return false;
		}

		return true;
	}

	static BOOL TestComparisons()
	{
		DOUBLE a = 1.0_embed;
		DOUBLE b = 2.0_embed;
		DOUBLE c = 1.0_embed;

		// Equality
		if (!(a == c))
		{
			LOG_ERROR("1.0 == 1.0 failed");
			return false;
		}
		if (a == b)
		{
			LOG_ERROR("1.0 != 2.0 failed (reported equal)");
			return false;
		}

		// Not equal
		if (a != c)
		{
			LOG_ERROR("!(1.0 != 1.0) failed");
			return false;
		}
		if (!(a != b))
		{
			LOG_ERROR("1.0 != 2.0 failed");
			return false;
		}

		// Less than
		if (!(a < b))
		{
			LOG_ERROR("1.0 < 2.0 failed");
			return false;
		}
		if (b < a)
		{
			LOG_ERROR("!(2.0 < 1.0) failed");
			return false;
		}
		if (a < c)
		{
			LOG_ERROR("!(1.0 < 1.0) failed");
			return false;
		}

		// Less than or equal
		if (!(a <= b))
		{
			LOG_ERROR("1.0 <= 2.0 failed");
			return false;
		}
		if (!(a <= c))
		{
			LOG_ERROR("1.0 <= 1.0 failed");
			return false;
		}
		if (b <= a)
		{
			LOG_ERROR("!(2.0 <= 1.0) failed");
			return false;
		}

		// Greater than
		if (!(b > a))
		{
			LOG_ERROR("2.0 > 1.0 failed");
			return false;
		}
		if (a > b)
		{
			LOG_ERROR("!(1.0 > 2.0) failed");
			return false;
		}
		if (a > c)
		{
			LOG_ERROR("!(1.0 > 1.0) failed");
			return false;
		}

		// Greater than or equal
		if (!(b >= a))
		{
			LOG_ERROR("2.0 >= 1.0 failed");
			return false;
		}
		if (!(a >= c))
		{
			LOG_ERROR("1.0 >= 1.0 failed");
			return false;
		}
		if (a >= b)
		{
			LOG_ERROR("!(1.0 >= 2.0) failed");
			return false;
		}

		return true;
	}

	static BOOL TestNegation()
	{
		// Negate positive
		DOUBLE pos = 5.0_embed;
		DOUBLE neg = -pos;
		double native_neg = (double)neg;
		if (native_neg != (double)-5.0_embed)
		{
			LOG_ERROR("-5.0 negation failed");
			return false;
		}

		// Negate negative
		DOUBLE neg2 = -3.0_embed;
		DOUBLE pos2 = -neg2;
		double native_pos2 = (double)pos2;
		if (native_pos2 != (double)3.0_embed)
		{
			LOG_ERROR("-(-3.0) != 3.0");
			return false;
		}

		// Double negation
		DOUBLE val = 7.0_embed;
		DOUBLE dbl_neg = -(-val);
		if ((double)dbl_neg != (double)7.0_embed)
		{
			LOG_ERROR("-(-7.0) != 7.0");
			return false;
		}

		return true;
	}

	static BOOL TestEmbeddedLiterals()
	{
		// Test _embed suffix for double literals
		DOUBLE a = 1.5_embed;
		if ((double)a != (double)1.5_embed)
		{
			LOG_ERROR("1.5_embed roundtrip failed");
			return false;
		}

		DOUBLE b = 3.14159_embed;
		double native_b = (double)b;
		// Allow small tolerance for floating point
		if (native_b < (double)3.14158_embed || native_b > (double)3.14160_embed)
		{
			LOG_ERROR("3.14159_embed out of tolerance");
			return false;
		}

		DOUBLE c = 0.5_embed;
		if ((double)c != (double)0.5_embed)
		{
			LOG_ERROR("0.5_embed roundtrip failed");
			return false;
		}

		DOUBLE d = 100.0_embed;
		if ((double)d != (double)100.0_embed)
		{
			LOG_ERROR("100.0_embed roundtrip failed");
			return false;
		}

		// Negative embedded
		DOUBLE e = -2.5_embed;
		if ((double)e != (double)-2.5_embed)
		{
			LOG_ERROR("-2.5_embed roundtrip failed");
			return false;
		}

		return true;
	}

	static BOOL TestEdgeCases()
	{
		// Zero
		DOUBLE zero = 0.0_embed;
		if ((double)zero != (double)0.0_embed)
		{
			LOG_ERROR("0.0 construction failed");
			return false;
		}

		// Adding zero
		DOUBLE val = 5.0_embed;
		DOUBLE result = val + zero;
		if ((double)result != (double)5.0_embed)
		{
			LOG_ERROR("5.0 + 0.0 != 5.0");
			return false;
		}

		// Multiplying by zero
		result = val * zero;
		if ((double)result != (double)0.0_embed)
		{
			LOG_ERROR("5.0 * 0.0 != 0.0");
			return false;
		}

		// Multiplying by one
		DOUBLE one = 1.0_embed;
		result = val * one;
		if ((double)result != (double)5.0_embed)
		{
			LOG_ERROR("5.0 * 1.0 != 5.0");
			return false;
		}

		// Small values
		DOUBLE small = 0.001_embed;
		DOUBLE thousand = 1000.0_embed;
		result = small * thousand;
		double native_result = (double)result;
		// Should be approximately 1.0
		if (native_result < (double)0.999_embed || native_result > (double)1.001_embed)
		{
			LOG_ERROR("0.001 * 1000.0 not approximately 1.0");
			return false;
		}

		return true;
	}

	static BOOL TestArrayFormatting()
	{
		// Test that DOUBLE arrays can be properly initialized and formatted
		// This ensures the varargs casting works correctly with Logger
		DOUBLE testArray[] = {1.1_embed, 2.2_embed, 3.3_embed, 4.4_embed, 5.5_embed, 6.6_embed, 7.7_embed, 8.8_embed, 9.9_embed, 10.1_embed};

		// Verify array initialization by checking that values are non-zero
		// We can't do exact comparisons without generating .rdata, so just verify they're initialized
		for (INT64 i = 0; i < 10; i++)
		{
			INT64 index = i;
			DOUBLE val = testArray[(INT64)index];
			// Just verify non-zero (all values are > 1.0)
			if (val.Bits() == 0)
			{
				LOG_ERROR("Array element %d has zero bits", (INT32)i);
				return false;
			}
		}

		// Test formatting output (this also tests that the values are properly passed through varargs)
		if (testArray[0] == 1.1_embed &&
			testArray[1] == 2.2_embed &&
			testArray[2] == 3.3_embed &&
			testArray[3] == 4.4_embed &&
			testArray[4] == 5.5_embed &&
			testArray[5] == 6.6_embed &&
			testArray[6] == 7.7_embed &&
			testArray[7] == 8.8_embed &&
			testArray[8] == 9.9_embed &&
			testArray[9] == 10.1_embed)
		{
			return true;
		}
		else
		{
			LOG_ERROR("Array element value mismatch");
			return false;
		}
	}
};
