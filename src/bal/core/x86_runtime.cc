/**
 * x86_runtime.cc - x86 Compiler Runtime Support
 *
 * Provides 64-bit division, modulo, and shift operations for 32-bit x86.
 * These functions are called implicitly by the compiler when building with -nostdlib.
 *
 * Part of BAL (Base Abstraction Layer) - Core runtime support.
 */

#include "bal/core/compiler.h"
#include "bal/types/primitives.h"

#if defined(ARCHITECTURE_I386)

// =============================================================================
// 64-bit Division Helpers
// =============================================================================

/**
 * Internal 64-bit unsigned division with quotient and remainder
 * Uses binary long division algorithm with power-of-2 optimization
 */
static inline void udiv64_internal(UINT64 numerator, UINT64 denominator,
                                   UINT64 *quotient, UINT64 *remainder)
{
    if (denominator == 0)
    {
        *quotient = 0;
        *remainder = numerator;
        return;
    }

    // Optimize for power-of-2 divisors
    if ((denominator & (denominator - 1ULL)) == 0)
    {
        // Count trailing zeros to find power
        INT32 shift = 0;
        UINT64 temp = denominator;
        while ((temp & 1ULL) == 0)
        {
            temp >>= 1;
            shift++;
        }

        *quotient = numerator >> shift;
        *remainder = numerator & (denominator - 1ULL);
        return;
    }

    // Binary long division for general case
    UINT64 q = 0;
    UINT64 r = 0;

    for (INT32 i = 63; i >= 0; i--)
    {
        r <<= 1;
        if ((numerator >> i) & 1ULL)
            r |= 1ULL;

        if (r >= denominator)
        {
            r -= denominator;
            q |= (1ULL << i);
        }
    }

    *quotient = q;
    *remainder = r;
}

extern "C"
{
    // =========================================================================
    // x86: 64-bit Unsigned Division Functions
    // =========================================================================

    /**
     * __udivdi3 - Unsigned 64-bit division
     * Returns: quotient
     */
    COMPILER_RUNTIME UINT64 __udivdi3(UINT64 numerator, UINT64 denominator)
    {
        UINT64 quotient, remainder;
        udiv64_internal(numerator, denominator, &quotient, &remainder);
        return quotient;
    }

    /**
     * __umoddi3 - Unsigned 64-bit modulo
     * Returns: remainder
     */
    COMPILER_RUNTIME UINT64 __umoddi3(UINT64 numerator, UINT64 denominator)
    {
        UINT64 quotient, remainder;
        udiv64_internal(numerator, denominator, &quotient, &remainder);
        return remainder;
    }

    // =========================================================================
    // x86: 64-bit Signed Division Functions
    // =========================================================================

    /**
     * __divdi3 - Signed 64-bit division
     * Returns: quotient
     */
    COMPILER_RUNTIME INT64 __divdi3(INT64 numerator, INT64 denominator)
    {
        if (denominator == 0)
            return 0;

        // Determine result sign
        const INT32 sign_num = numerator < 0 ? -1 : 1;
        const INT32 sign_den = denominator < 0 ? -1 : 1;
        const INT32 sign_quot = sign_num * sign_den;

        // Convert to absolute values
        const UINT64 abs_num = (UINT64)(numerator < 0 ? -numerator : numerator);
        const UINT64 abs_den = (UINT64)(denominator < 0 ? -denominator : denominator);

        // Perform unsigned division
        UINT64 quotient, remainder;
        udiv64_internal(abs_num, abs_den, &quotient, &remainder);

        // Apply sign to quotient
        return sign_quot < 0 ? -(INT64)quotient : (INT64)quotient;
    }

    /**
     * __moddi3 - Signed 64-bit modulo
     * Returns: remainder (takes sign of numerator)
     */
    COMPILER_RUNTIME INT64 __moddi3(INT64 numerator, INT64 denominator)
    {
        if (denominator == 0)
            return numerator;

        // Remainder takes sign of numerator
        const INT32 sign_num = numerator < 0 ? -1 : 1;

        // Convert to absolute values
        const UINT64 abs_num = (UINT64)(numerator < 0 ? -numerator : numerator);
        const UINT64 abs_den = (UINT64)(denominator < 0 ? -denominator : denominator);

        // Perform unsigned division
        UINT64 quotient, remainder;
        udiv64_internal(abs_num, abs_den, &quotient, &remainder);

        // Apply sign to remainder
        return sign_num < 0 ? -(INT64)remainder : (INT64)remainder;
    }

    // =========================================================================
    // x86: 64-bit Shift Functions
    // =========================================================================

    /**
     * __lshrdi3 - 64-bit logical shift right
     */
    COMPILER_RUNTIME UINT64 __lshrdi3(UINT64 value, INT32 shift)
    {
        if (shift < 0 || shift >= 64)
            return 0;
        return value >> shift;
    }

    /**
     * __ashldi3 - 64-bit arithmetic shift left
     */
    COMPILER_RUNTIME INT64 __ashldi3(INT64 value, INT32 shift)
    {
        if (shift < 0 || shift >= 64)
            return 0;
        return value << shift;
    }

} // extern "C"

#endif // ARCHITECTURE_I386
