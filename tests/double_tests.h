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

		RunTest(allPassed, &TestBasicArithmetic, "Basic arithmetic");
		RunTest(allPassed, &TestComparisons, "Comparisons");
		RunTest(allPassed, &TestLiterals, "Literals");
		RunTest(allPassed, &TestConversions, "Conversions");
		RunTest(allPassed, &TestTypedef, "Typedef");

		if (allPassed)
			LOG_INFO("All DOUBLE tests passed!");
		else
			LOG_ERROR("Some DOUBLE tests failed!");

		return allPassed;
	}

private:
	static BOOL TestBasicArithmetic()
	{
		double a = 2.0;
		double b = 3.0;

		if (a + b != 5.0)
		{
			LOG_ERROR("2.0 + 3.0 != 5.0");
			return false;
		}
		if (b - a != 1.0)
		{
			LOG_ERROR("3.0 - 2.0 != 1.0");
			return false;
		}
		if (a * b != 6.0)
		{
			LOG_ERROR("2.0 * 3.0 != 6.0");
			return false;
		}
		if (6.0 / a != 3.0)
		{
			LOG_ERROR("6.0 / 2.0 != 3.0");
			return false;
		}

		return true;
	}

	static BOOL TestComparisons()
	{
		double a = 1.0;
		double b = 2.0;

		if (!(a < b))
		{
			LOG_ERROR("1.0 < 2.0 failed");
			return false;
		}
		if (!(a <= b))
		{
			LOG_ERROR("1.0 <= 2.0 failed");
			return false;
		}
		if (!(b > a))
		{
			LOG_ERROR("2.0 > 1.0 failed");
			return false;
		}
		if (!(a == 1.0))
		{
			LOG_ERROR("1.0 == 1.0 failed");
			return false;
		}
		if (a == b)
		{
			LOG_ERROR("1.0 != 2.0 failed");
			return false;
		}

		return true;
	}

	static BOOL TestLiterals()
	{
		double pi = 3.14159;
		if (pi < 3.14158 || pi > 3.14160)
		{
			LOG_ERROR("3.14159 out of tolerance");
			return false;
		}

		double half = 0.5;
		if (half != 0.5)
		{
			LOG_ERROR("0.5 roundtrip failed");
			return false;
		}

		double neg = -2.5;
		if (neg != -2.5)
		{
			LOG_ERROR("-2.5 roundtrip failed");
			return false;
		}

		return true;
	}

	static BOOL TestConversions()
	{
		double val = 100.5;
		INT32 intVal = (INT32)val;
		if (intVal != 100)
		{
			LOG_ERROR("(INT32)100.5 expected 100, got %d", intVal);
			return false;
		}

		double fromInt = (double)42;
		if (fromInt != 42.0)
		{
			LOG_ERROR("(double)42 != 42.0");
			return false;
		}

		return true;
	}

	static BOOL TestTypedef()
	{
		// Verify DOUBLE typedef works as native double
		DOUBLE a = 3.14;
		double b = 3.14;
		if (a != b)
		{
			LOG_ERROR("DOUBLE typedef mismatch with native double");
			return false;
		}

		PDOUBLE ptr = &a;
		*ptr = 2.71;
		if (a != 2.71)
		{
			LOG_ERROR("PDOUBLE pointer access failed");
			return false;
		}

		return true;
	}
};
