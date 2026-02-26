/**
 * Tests.h - Test Utilities
 *
 * Provides template functions for running tests with pass/fail logging.
 * Uses function-based API for better IDE support and code highlighting.
 */

#pragma once

#include "logger.h"

/**
 * RunTest - Run a test function with pass/fail logging
 *
 * Executes a test function and logs the result. Updates the allPassed variable
 * if the test fails.
 *
 * USAGE:
 *   BOOL allPassed = true;
 *   RunTest(allPassed, TestFunction, L"Test description"_embed);
 *
 * @param allPassedVar - Boolean variable to track overall test status
 * @param testFunc     - Test function to execute (must return BOOL)
 * @param description  - Human-readable description of the test (wide embedded string)
 * @return true if test passed, false otherwise
 */
#if defined(ENABLE_LOGGING)
template <typename TestFunc>
inline BOOL RunTest(BOOL &allPassedVar, TestFunc testFunc, PCWCHAR description)
{
	if (!testFunc())
	{
		allPassedVar = false;
		LOG_ERROR("  FAILED: %ls", description);
		return false;
	}
	LOG_INFO("  PASSED: %ls", description);
	return true;
}
#else
template <typename TestFunc>
inline BOOL RunTest(BOOL &allPassedVar, TestFunc testFunc)
{
	if (!testFunc())
	{
		allPassedVar = false;
		return false;
	}
	return true;
}
// Strip the description argument at the call site to prevent embedded string construction
#define RunTest(allPassed, func, ...) RunTest(allPassed, func)
#endif

/**
 * RunTestSuite - Run a test suite's RunAll() method
 *
 * Executes a test suite's RunAll() method and updates the overall status.
 * Adds a blank line after the suite for visual separation.
 *
 * USAGE:
 *   BOOL allPassed = true;
 *   RunTestSuite<MemoryTests>(allPassed);
 *   RunTestSuite<StringTests>(allPassed);
 *
 * @param allPassedVar - Boolean variable to track overall test status
 * @return true if all tests in suite passed, false otherwise
 */
template <typename TestSuite>
inline BOOL RunTestSuite(BOOL &allPassedVar)
{
	BOOL result = TestSuite::RunAll();
	if (!result)
		allPassedVar = false;
	LOG_INFO("");
	return result;
}

/**
 * CompareBytes - Compare two byte arrays for equality
 */
inline BOOL CompareBytes(const UINT8 *a, const UINT8 *b, UINT32 length)
{
	for (UINT32 i = 0; i < length; i++)
	{
		if (a[i] != b[i])
			return false;
	}
	return true;
}

/**
 * IsAllZeros - Check if all bytes in a buffer are zero
 */
inline BOOL IsAllZeros(const UINT8 *data, UINT32 length)
{
	for (UINT32 i = 0; i < length; i++)
	{
		if (data[i] != 0)
			return false;
	}
	return true;
}
