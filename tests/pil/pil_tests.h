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
 *   RunScriptTests();
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

#include "pil/pil.h"
#include "pal/io/console.h"

// ============================================================================
// CONSOLE OUTPUT CALLBACK FOR SCRIPT TESTS
// ============================================================================

/**
 * Console output callback for script State.
 * Routes script output (print, etc.) directly to console.
 */
static void ScriptConsoleOutput(const CHAR* str, USIZE len)
{
    Console::Write(str, len);
}

/**
 * Helper to create a State with console output attached.
 */
static script::State* CreateScriptState()
{
    script::State* L = new script::State();
    L->SetOutput(EMBED_FUNC(ScriptConsoleOutput));
    return L;
}

#include "state_tests.h"
#include "stdlib_tests.h"
#include "language_tests.h"
#include "error_tests.h"
#include "float_tests.h"
#include "fileio_tests.h"
#include "networkio_tests.h"

static BOOL RunScriptTests()
{
    BOOL allPassed = TRUE;

    LOG_INFO("=== PIL Test Suite ===");
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

    // Floating-point tests
    RUN_TEST_SUITE(allPassed, FloatTests);

    // File I/O tests
    RUN_TEST_SUITE(allPassed, FileIOTests);

    // Network I/O tests
    RUN_TEST_SUITE(allPassed, NetworkIOTests);

    // Final summary
    LOG_INFO("=== PIL Test Suite Complete ===");
    if (allPassed)
        LOG_INFO("ALL SCRIPT TESTS PASSED!");
    else
        LOG_ERROR("SOME SCRIPT TESTS FAILED!");

    return allPassed;
}
