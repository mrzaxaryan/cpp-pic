/**
 * pil.h - PIL (Position Independent Language) Main Entry Point
 *
 * Provides a State-based API for script execution.
 * NO built-in functions - all functions must be registered from C++.
 *
 * Position-independent, no .rdata dependencies.
 *
 * Part of RAL (Runtime Abstraction Layer).
 *
 * USAGE:
 *   script::State L;
 *   L.SetOutput(MyOutputFunc);
 *
 *   // Register functions manually
 *   script::OpenStdLib(L);  // OR register individual functions
 *   L.Register("myFunc", MyCustomFunction);
 *
 *   L.DoString("print(myFunc(42));");
 */

#pragma once

// State-based API (includes all core components: token, lexer, ast, parser, value, interpreter)
#include "state.h"

// Standard library
#include "stdlib.h"

namespace script
{

// ============================================================================
// NO BUILT-IN FUNCTIONS
// ============================================================================
//
// PIL has NO built-in functions by default.
// All functions must be registered manually from C++.
//
// Use OpenStdLib(L) to register the standard library:
//   Core:    print, len, str, num, type
//   Math:    abs, min, max, floor, ceil, int, round, clamp, sign, pow, sqrt
//   String:  substr, indexOf, trim, upper, lower, startsWith, endsWith,
//            replace, char, ord
//   Array:   push, pop, contains, reverse
//
// Or register functions individually:
//   L.Register("print", StdLib_Print);
//   L.Register("myFunc", MyCustomFunction);
//
// ============================================================================

} // namespace script
