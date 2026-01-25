#pragma once

#include "primitives.h"

/**
 * EMBEDDED_STRING - Position-independent compile-time string literal embedding
 *
 * Eliminates .rdata section usage by materializing string literals directly in code.
 * Essential for shellcode, injection payloads, and strict PIC environments.
 *
 * COMMON USE CASES:
 *   - Shellcode & Position-Independent Code (PIC): Eliminates .rdata relocations
 *   - Kernel-Mode Drivers: Satisfies strict non-paged memory requirements
 *   - OS Development: Embedded systems and microkernels without data sections
 *   - Malware Development (Red Team/Research): Evades static string extraction
 *   - Bootloaders & Firmware: Pre-MMU environments without .rdata support
 */

// ============================================================================
// COMPILER REQUIREMENTS
// ============================================================================

/**
 * COMPILER OPTIMIZATION SUPPORT
 *
 * Tested and working: -O0, -O1, -O2, -O3, -Og, -Os, -Oz
 *
 * Implementation:
 *   The EMBEDDED_STRING class uses NOINLINE and DISABLE_OPTIMIZATION attributes
 *   to force runtime stack construction of strings. Characters are materialized
 *   one-by-one at runtime using immediate values embedded in code, avoiding
 *   .rdata section dependencies.
 *
 * Build requirements:
 *   - i386: -mno-sse -mno-sse2 (disables SSE to prevent .rdata generation)
 *   - x86_64: -mno-sse4.1 -mno-sse4.2 -mno-avx -mno-avx2 (limits to SSE2)
 *   - -fno-vectorize -fno-slp-vectorize (prevents auto-vectorization)
 *
 * Verification:
 *   - No .rdata section in final binary
 *   - No string literals or floating-point constants in .rdata
 *   - All strings embedded as immediate values in .text section
 */

// ============================================================================
// CHARACTER TYPE CONSTRAINT
// ============================================================================

// Restricts template to char or wchar_t
template <typename TChar>
concept TCHAR = __is_same_as(TChar, CHAR) || __is_same_as(TChar, WCHAR);

// ============================================================================
// EMBEDDED_STRING CLASS
// ============================================================================

template <TCHAR TChar, TChar... Cs>
class EMBEDDED_STRING
{
private:
    static constexpr USIZE N = sizeof...(Cs) + 1; // Includes null terminator

    // NOT aligned - prevents SSE optimization
    TChar data[N];

public:
    static constexpr USIZE Length = N - 1; // Excludes null terminator

    /**
     * Runtime Constructor - Forces string materialization on stack
     *
     * Using NOINLINE and DISABLE_OPTIMIZATION to prevent:
     * 1. Compile-time constant folding
     * 2. SSE vectorization
     * 3. Merging into .rdata section
     *
     * Characters are materialized one-by-one at runtime using immediate values.
     */
    NOINLINE DISABLE_OPTIMIZATION EMBEDDED_STRING() noexcept : data{}
    {
        // Write characters one by one at runtime using fold expression
        // The NOINLINE and DISABLE_OPTIMIZATION force this to execute at runtime
        // Characters are embedded as immediate operands in the code
        USIZE i = 0;
        ((data[i++] = Cs), ...);
        data[i] = (TChar)0;
    }

    /**
     * Implicit conversion to const pointer
     *
     * Zero-cost conversion with no relocations or runtime overhead.
     */
    constexpr operator const TChar *() const noexcept
    {
        return data;
    }

    /**
     * Array subscript operator for direct character access
     */
    constexpr const TChar &operator[](USIZE index) const noexcept
    {
        return data[index];
    }
};

// ============================================================================
// USER-DEFINED LITERAL OPERATOR
// ============================================================================

/**
 * Literal suffix for compile-time string embedding
 *
 * Usage:
 *   auto str = "Hello"_embed;       // char version
 *   auto wstr = L"Hello"_embed;     // wchar_t version
 *
 * The compiler expands the string literal into a character parameter pack,
 * and the EMBEDDED_STRING constructor materializes it at runtime on the stack.
 * No longer consteval to allow runtime construction.
 */
template <TCHAR TChar, TChar... Chars>
FORCE_INLINE auto operator""_embed() noexcept
{
    return EMBEDDED_STRING<TChar, Chars...>{};
}