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
 *   BOOL allPassed = TRUE;
 *   RunTest(allPassed, TestFunction, L"Test description"_embed);
 *
 * @param allPassedVar - Boolean variable to track overall test status
 * @param testFunc     - Test function to execute (must return BOOL)
 * @param description  - Human-readable description of the test (wide embedded string)
 * @return TRUE if test passed, FALSE otherwise
 */
template <typename TestFunc>
inline BOOL RunTest(BOOL& allPassedVar, TestFunc testFunc, PCWCHAR description)
{
	if (!testFunc())
	{
		allPassedVar = FALSE;
		LOG_ERROR("  FAILED: %ls", description);
		return FALSE;
	}
	LOG_INFO("  PASSED: %ls", description);
	return TRUE;
}

/**
 * RunTestSuite - Run a test suite's RunAll() method
 *
 * Executes a test suite's RunAll() method and updates the overall status.
 * Adds a blank line after the suite for visual separation.
 *
 * USAGE:
 *   BOOL allPassed = TRUE;
 *   RunTestSuite<MemoryTests>(allPassed);
 *   RunTestSuite<StringTests>(allPassed);
 *
 * @param allPassedVar - Boolean variable to track overall test status
 * @return TRUE if all tests in suite passed, FALSE otherwise
 */
template <typename TestSuite>
inline BOOL RunTestSuite(BOOL& allPassedVar)
{
	BOOL result = TestSuite::RunAll();
	if (!result)
		allPassedVar = FALSE;
	LOG_INFO("");
	return result;
}
