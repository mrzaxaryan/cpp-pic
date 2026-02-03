/**
 * Tests.h - Test Utilities and Macros
 *
 * Provides convenience macros for running tests with pass/fail logging.
 */

#pragma once

#include "pir/platform/io/logger.h"

/**
 * RUN_TEST - Convenience macro for running tests with pass/fail logging
 *
 * Executes a test function and logs the result. Updates the allPassed variable
 * if the test fails.
 *
 * USAGE:
 *   BOOL allPassed = TRUE;
 *   RUN_TEST(allPassed, TestFunction, "Test description");
 *
 * @param allPassedVar - Boolean variable to track overall test status
 * @param testFunc     - Test function to execute (must return BOOL)
 * @param description  - Human-readable description of the test
 */
#define RUN_TEST(allPassedVar, testFunc, description) \
	do { \
		if (!(testFunc())) { \
			allPassedVar = FALSE; \
			LOG_ERROR("  FAILED: " description); \
		} else { \
			LOG_INFO("  PASSED: " description); \
		} \
	} while (0)

/**
 * RUN_TEST_SUITE - Convenience macro for running test suites
 *
 * Executes a test suite's RunAll() method and updates the overall status.
 * Adds a blank line after the suite for visual separation.
 *
 * USAGE:
 *   BOOL allPassed = TRUE;
 *   RUN_TEST_SUITE(allPassed, MemoryTests);
 *   RUN_TEST_SUITE(allPassed, StringTests);
 *
 * @param allPassedVar - Boolean variable to track overall test status
 * @param testSuite    - Test suite class (must have static RunAll() method)
 */
#define RUN_TEST_SUITE(allPassedVar, testSuite) \
	do { \
		if (!testSuite::RunAll()) \
			allPassedVar = FALSE; \
		LOG_INFO(""); \
	} while (0)
