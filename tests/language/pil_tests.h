/**
 * pil_tests.h - Unified PIL Test Suite Header
 *
 * This header exposes all test suite classes for the PIL language.
 * Include this single header to access all test functionality.
 *
 * TEST SUITES:
 *   StateTests     - State API tests (registration, globals, lifecycle)
 *   StdLibTests    - Standard library function tests (print, len, str, num, type, abs, min, max)
 *   LanguageTests  - Language feature tests (control flow, recursion, operators, functions)
 *   ErrorTests     - Error handling and detection tests
 *   FloatTests     - Floating-point number tests
 *   FileIOTests    - File I/O function tests (fopen, fclose, fread, fwrite, etc.)
 *   NetworkIOTests - Network I/O function tests (sockets, DNS, HTTP)
 *
 * USAGE:
 *   #include "pil_tests.h"
 *
 *   // Run all tests
 *   RunPILTests();
 *
 *   // Or run individual suites
 *   StateTests::RunAll();
 *   StdLibTests::RunAll();
 *   LanguageTests::RunAll();
 *   ErrorTests::RunAll();
 *   FloatTests::RunAll();
 *   FileIOTests::RunAll();
 *   NetworkIOTests::RunAll();
 */

#pragma once

#include "state_tests.h"
#include "stdlib_tests.h"
#include "language_tests.h"
#include "error_tests.h"
#include "float_tests.h"
#include "fileio_tests.h"
#include "networkio_tests.h"

static BOOL RunPILTests()
{
    BOOL allPassed = TRUE;

    LOG_INFO("=== PIL Test Suite ===");
    LOG_INFO("   (No built-in functions)");
    LOG_INFO("");

    // State API tests
    RunTestSuite<StateTests>(allPassed);

    // Standard library tests
    RunTestSuite<StdLibTests>(allPassed);

    // Language feature tests
    RunTestSuite<LanguageTests>(allPassed);

    // Error handling tests
    RunTestSuite<ErrorTests>(allPassed);

    // Floating-point tests
    RunTestSuite<FloatTests>(allPassed);

    // File I/O tests
    RunTestSuite<FileIOTests>(allPassed);

    // Network I/O tests
    RunTestSuite<NetworkIOTests>(allPassed);

    // Final summary
    LOG_INFO("=== PIL Test Suite Complete ===");
    if (allPassed)
        LOG_INFO("ALL SCRIPT TESTS PASSED!");
    else
        LOG_ERROR("SOME SCRIPT TESTS FAILED!");

    return allPassed;
}
