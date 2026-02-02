/**
 * script_tests.h - Unified PICScript Test Suite Header
 *
 * This header exposes all test suite classes for the PICScript language.
 * Include this single header to access all test functionality.
 *
 * TEST SUITES:
 *   StateTests     - State API tests (registration, globals, lifecycle)
 *   StdLibTests    - Standard library function tests (print, len, str, num, type, abs, min, max)
 *   LanguageTests  - Language feature tests (control flow, recursion, operators, functions)
 *   ErrorTests     - Error handling and detection tests
 *
 * USAGE:
 *   #include "script_tests.h"
 *
 *   // Run all tests
 *   RunScriptTests();
 *
 *   // Or run individual suites
 *   StateTests::RunAll();
 *   StdLibTests::RunAll();
 *   LanguageTests::RunAll();
 *   ErrorTests::RunAll();
 */

#pragma once

#include "state_tests.h"
#include "stdlib_tests.h"
#include "language_tests.h"
#include "error_tests.h"

static BOOL RunScriptTests()
{
    BOOL allPassed = TRUE;

    LOG_INFO("=== PICScript Test Suite ===");
    LOG_INFO("   (No built-in functions)");
    LOG_INFO("");

    // State API tests
    RUN_TEST_SUITE(allPassed, StateTests);

    // Standard library tests
    RUN_TEST_SUITE(allPassed, StdLibTests);

    // Language feature tests
    RUN_TEST_SUITE(allPassed, LanguageTests);

    // Error handling tests
    RUN_TEST_SUITE(allPassed, ErrorTests);

    // Final summary
    LOG_INFO("=== PICScript Test Suite Complete ===");
    if (allPassed)
        LOG_INFO("ALL SCRIPT TESTS PASSED!");
    else
        LOG_ERROR("SOME SCRIPT TESTS FAILED!");

    return allPassed;
}
