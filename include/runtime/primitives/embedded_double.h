#pragma once

#include "primitives.h"

// Forward declaration - DOUBLE class is defined in double.h
class DOUBLE;

/**
 * EMBEDDED_DOUBLE — Position-independent compile-time floating-point literal embedding
 *
 * Stores IEEE-754 double precision values as integer words, eliminating .rdata
 * dependencies. Reconstructs the floating-point value at runtime through pure
 * bit manipulation without constant pool references.
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
 *   The DOUBLE class implements pure integer-based conversions to INT64/UINT64
 *   using bit manipulation of the IEEE-754 format. This eliminates the need for
 *   -mno-sse or -msoft-float flags. Floating-point arithmetic operations use
 *   SSE instructions with stack operands (no .rdata loads).
 *
 * Verification:
 *   - Floating-point literals embedded as 64-bit immediate values in code
 *   - Double-to-integer conversions use only bitwise operations
 *   - SSE arithmetic instructions (addsd, mulsd, etc.) use stack operands only
 *   - No floating-point constants stored in .rdata section
 */

// ============================================================================
// EMBEDDED_DOUBLE STRUCT
// ============================================================================

struct EMBEDDED_DOUBLE
{
    // Architecture-dependent word size (4 bytes on i386, 8 bytes on x64)
    static constexpr USIZE WordBytes = sizeof(USIZE);

    // Number of words needed to store 64-bit double (2 on i386, 1 on x64)
    static constexpr USIZE WordCount =
        (sizeof(UINT64) + WordBytes - 1) / WordBytes;

    // Integer storage for IEEE-754 bit pattern (no FP storage)
    alignas(8) USIZE words[WordCount]{};

    /**
     * Constructor - Converts double to packed integer representation
     *
     * Decomposes the IEEE-754 bit pattern into native words at compile time.
     * Byte-by-byte packing prevents the compiler from recognizing this as
     * a floating-point constant that could be pooled in .rdata.
     */
    consteval explicit EMBEDDED_DOUBLE(double v)
    {
        unsigned long long u = __builtin_bit_cast(unsigned long long, v);

        // Pack 64-bit pattern into native words (little-endian)
        for (USIZE byte = 0; byte < sizeof(unsigned long long); ++byte)
        {
            USIZE wordIndex = byte / WordBytes;
            USIZE shift = (byte % WordBytes) * 8u;

            words[wordIndex] |=
                (USIZE)((u >> (byte * 8u)) & 0xFFu) << shift;
        }
    }

    /**
     * Bits() - Reconstructs IEEE-754 bit pattern from integer words
     *
     * DISABLE_OPTIMIZATION prevents:
     *   - Constant folding back to FP literals
     *   - SSE-based re-materialization
     *   - Vectorized loads from hidden constant pools
     *
     * Returns raw unsigned long long suitable for bit_cast back to double.
     */
    NOINLINE DISABLE_OPTIMIZATION unsigned long long BitsULL() const noexcept
    {
        unsigned long long u = 0;

        for (USIZE byte = 0; byte < sizeof(unsigned long long); ++byte)
        {
            USIZE wordIndex = byte / WordBytes;
            USIZE shift = (byte % WordBytes) * 8u;

            unsigned long long b =
                (unsigned long long)((words[wordIndex] >> shift) & (USIZE)0xFFu);

            u |= b << (byte * 8u);
        }

        return u;
    }

    /**
     * Bits() - Returns UINT64 representation of the bit pattern
     */
    NOINLINE DISABLE_OPTIMIZATION UINT64 Bits() const noexcept
    {
        return UINT64(BitsULL());
    }

    /**
     * Implicit conversion to DOUBLE class
     *
     * Pure bit reinterpretation with no .rdata loads or FP constant references.
     * Safe under -mno-sse as it operates entirely on integer bits.
     * Converts to our custom DOUBLE class instead of native double.
     */
    operator DOUBLE() const noexcept
    {
        return DOUBLE(Bits());
    }

    /**
     * Implicit conversion to native double (for compatibility)
     *
     * Pure bit reinterpretation with no .rdata loads or FP constant references.
     * Safe under -mno-sse as it operates entirely on integer bits.
     *
     * NOINLINE DISABLE_OPTIMIZATION prevents constant folding that would
     * place the reconstructed double in .rdata.
     */
    NOINLINE DISABLE_OPTIMIZATION operator double() const noexcept
    {
        return __builtin_bit_cast(double, BitsULL());
    }

    /*
     * Implicit conversion to UINT64
     * Pure bit reinterpretation with no .rdata loads or FP constant references.
     * Safe under -mno-sse as it operates entirely on integer bits.
     */
    operator UINT64() const noexcept
    {
        return Bits();
    }

    /**
     * Unary negation operator
     *
     * Flips the sign bit of the IEEE-754 representation.
     * This prevents the compiler from using xorpd with a .rdata constant
     * when negating embedded double literals like -5.0_embed.
     *
     * NOINLINE DISABLE_OPTIMIZATION prevents constant folding.
     */
    NOINLINE DISABLE_OPTIMIZATION EMBEDDED_DOUBLE operator-() const noexcept
    {
        EMBEDDED_DOUBLE result = *this;
        // Sign bit is in the MSB of the 64-bit value
        // On little-endian, this is in the high word
        if constexpr (WordBytes == 8)
        {
            // 64-bit: sign bit is bit 63 of words[0]
            result.words[0] ^= (USIZE)1ULL << 63;
        }
        else
        {
            // 32-bit: sign bit is bit 31 of words[1] (high word)
            result.words[1] ^= (USIZE)1U << 31;
        }
        return result;
    }
};

// ============================================================================
// USER-DEFINED LITERAL OPERATORS
// ============================================================================

/**
 * Literal suffix for compile-time floating-point embedding
 *
 * Usage:
 *   auto pi = 3.14159_embed;
 *   auto ratio = 42.0_embed;
 *   auto large = 1e308_embed;
 */
consteval auto operator""_embed(long double v)
{
    return EMBEDDED_DOUBLE(static_cast<double>(v));
}

/**
 * Literal suffix for integer-to-double embedding
 *
 * Usage:
 *   auto value = 42_embed;  // Stored as 42.0 (double)
 */
consteval auto operator""_embed(unsigned long long value)
{
    return EMBEDDED_DOUBLE(static_cast<double>(value));
}