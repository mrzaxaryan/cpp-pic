/**
 * compiler.h - Compiler-Specific Definitions and Macros
 *
 * Provides compiler checks, optimization attributes, and calling conventions.
 * This file contains all compiler-specific macros and directives.
 *
 * Part of BAL (Base Abstraction Layer) - platform-independent.
 */

#pragma once

// =============================================================================
// COMPILER CHECKS
// =============================================================================

#if !defined(__llvm__) && !defined(__clang__)
#error "Only Clang/LLVM compiler is supported!"
#endif

// =============================================================================
// OPTIMIZATION ATTRIBUTES
// =============================================================================

#define FORCE_INLINE __attribute__((always_inline)) inline

#define NOINLINE __attribute__((noinline))
#define DISABLE_OPTIMIZATION __attribute__((optnone))

// Compiler runtime functions: prevent inlining, LTO, and optimization
// Used for ABI-mandated runtime functions called by the compiler
#define COMPILER_RUNTIME __attribute__((noinline, used, optnone))

// =============================================================================
// FUNCTION ATTRIBUTES
// =============================================================================

#define NO_RETURN extern "C" __attribute__((noreturn))

// Entry point macro
#define ENTRYPOINT extern "C" __attribute__((noreturn))