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
 *   at runtime using immediate values embedded in code, avoiding .rdata section
 *   dependencies. Uses binary-split recursion for O(log n) template depth,
 *   enabling support for large strings without exceeding compiler limits.
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

    // Compile-time character access - index is constexpr so result becomes immediate
    template <USIZE I>
    FORCE_INLINE static constexpr TChar get_char() noexcept
    {
        constexpr TChar chars[] = {Cs...};
        return chars[I];
    }

    // Binary-split initialization - O(log n) template depth instead of O(n)
    template <USIZE Start, USIZE End>
    FORCE_INLINE void init_range() noexcept
    {
        if constexpr (End - Start == 1) {
            data[Start] = get_char<Start>();
        } else if constexpr (End > Start) {
            constexpr USIZE Mid = Start + (End - Start) / 2;
            init_range<Start, Mid>();
            init_range<Mid, End>();
        }
    }

public:
    static constexpr USIZE Length() noexcept { return N - 1; }  // Excludes null terminator
    static constexpr USIZE Size() noexcept { return N; }        // Includes null terminator

    /**
     * Runtime Constructor - Forces string materialization on stack
     *
     * Using NOINLINE and DISABLE_OPTIMIZATION to prevent:
     * 1. Compile-time constant folding
     * 2. SSE vectorization
     * 3. Merging into .rdata section
     *
     * Characters are materialized one-by-one at runtime using immediate values.
     * Uses binary-split recursion for O(log n) template depth.
     */
    NOINLINE DISABLE_OPTIMIZATION EMBEDDED_STRING() noexcept : data{}
    {
        // Binary-split initialization avoids deep expression nesting
        init_range<0, sizeof...(Cs)>();
        data[N - 1] = (TChar)0;
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
 */
template <TCHAR TChar, TChar... Chars>
FORCE_INLINE auto operator""_embed() noexcept
{
    return EMBEDDED_STRING<TChar, Chars...>{};
}