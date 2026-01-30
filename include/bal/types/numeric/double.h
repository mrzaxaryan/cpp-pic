/**
 * double.h - Position-Independent Double Precision Floating-Point
 *
 * Provides a unified DOUBLE class that handles both compile-time literal
 * embedding and runtime floating-point operations without creating .rdata
 * section references. Essential for position-independent code (PIC).
 *
 * Key Features:
 *   - Compile-time embedding via _embed suffix (no .rdata)
 *   - Full IEEE-754 double precision arithmetic
 *   - Architecture-independent (i386, x86_64, ARM)
 *   - Type conversions to/from integers and native types
 *
 * Usage:
 *   DOUBLE x = 3.14_embed;        // Compile-time embedded
 *   DOUBLE y = x * 2.0_embed;     // Arithmetic
 *   int z = (int)x;                // Conversion
 */
#pragma once

#include "uint64.h"

// Forward declaration
class INT64;

/**
 * DOUBLE - Unified position-independent double precision floating-point class
 *
 * Handles both compile-time literal embedding and runtime floating-point operations
 * without .rdata section references. Stores IEEE-754 double as UINT64 bit pattern
 * and performs operations through bit manipulation and controlled SSE instructions.
 *
 * Architecture:
 *   - consteval constructor: Private constructor for _embed literals (no .rdata)
 *   - constexpr constructor: Public constructor for runtime conversions
 *   - Runtime operations: Full arithmetic, comparisons, and type conversions
 *
 * Usage:
 *   DOUBLE x = 3.14_embed;        // Compile-time embedded, no .rdata
 *   DOUBLE y = x + 2.0_embed;     // Arithmetic operations
 *   int z = (int)x;                // Type conversions
 *
 * This unified design eliminates both the separate EMBEDDED_DOUBLE class and the
 * complex EmbeddedHelper word-packing, reducing ~120 lines of code while maintaining
 * full position-independence through simple consteval constructors.
 */
class DOUBLE
{
private:
    UINT64 bits; // IEEE-754 double precision bit pattern

    // IEEE-754 double precision format helpers - Using constexpr functions to avoid .rdata
    static constexpr int SIGN_SHIFT = 63;
    static constexpr int EXP_SHIFT = 52;

    // Use constexpr functions instead of static constexpr variables to avoid .rdata section
    static constexpr UINT64 GetSignMask() noexcept { return UINT64(0x80000000U, 0x00000000U); }
    static constexpr UINT64 GetExpMask() noexcept { return UINT64(0x7FF00000U, 0x00000000U); }
    static constexpr UINT64 GetMantissaMask() noexcept { return UINT64(0x000FFFFFU, 0xFFFFFFFFU); }

public:
    // ========================================================================
    // Constructors
    // ========================================================================

    /**
     * Default constructor - Creates DOUBLE with value 0.0
     */
    constexpr DOUBLE() noexcept : bits(0, 0) {}

    /**
     * Copy constructor
     */
    constexpr DOUBLE(const DOUBLE &) noexcept = default;

    /**
     * Constructor from UINT64 bit pattern
     *
     * Directly uses the provided bits as IEEE-754 representation.
     * Useful for reconstruction from stored bit patterns.
     *
     * @param bitPattern 64-bit IEEE-754 double precision bit pattern
     */
    constexpr explicit DOUBLE(UINT64 bitPattern) noexcept : bits(bitPattern) {}

    /**
     * Constructor from two 32-bit words
     *
     * Useful for i386 architecture where 64-bit values are stored as pairs.
     *
     * @param high Upper 32 bits of IEEE-754 pattern
     * @param low Lower 32 bits of IEEE-754 pattern
     */
    constexpr DOUBLE(UINT32 high, UINT32 low) noexcept : bits(high, low) {}

    /**
     * Constructor from native double (runtime use)
     *
     * For runtime conversions from native double to DOUBLE.
     * For compile-time literals, use the _embed suffix instead.
     *
     * @param val Native C++ double value
     */
    constexpr DOUBLE(double val) noexcept
    {
        unsigned long long ull = __builtin_bit_cast(unsigned long long, val);
        bits = UINT64(ull);
    }

private:
    /**
     * Private compile-time literal constructor
     *
     * Used exclusively by the _embed literal operators. The consteval
     * specifier forces compile-time evaluation, preventing .rdata creation.
     * Private to ensure users go through the _embed suffix for literals.
     *
     * @param val Double literal value
     * @param dummy Unused tag parameter to distinguish from runtime constructor
     */
    struct CompileTimeLiteral {};
    consteval DOUBLE(double val, CompileTimeLiteral) noexcept
    {
        unsigned long long ull = __builtin_bit_cast(unsigned long long, val);
        bits = UINT64(ull);
    }

    friend consteval DOUBLE operator""_embed(long double v);
    friend consteval DOUBLE operator""_embed(unsigned long long value);

public:

    // Constructor from integer
    constexpr DOUBLE(INT32 val) noexcept
    {
        // Convert integer to double bit pattern
        if (val == 0)
        {
            bits = UINT64(0, 0);
            return;
        }

        bool negative = val < 0;
        UINT32 absVal = negative ? (UINT32)(-val) : (UINT32)val;

        // Find the most significant bit
        int msb = 31;
        while (msb >= 0 && !((absVal >> msb) & 1))
            msb--;

        // Exponent is 1023 + msb (IEEE-754 bias)
        int exponent = 1023 + msb;

        // Mantissa is the remaining bits after the implicit 1
        UINT64 mantissa = UINT64(absVal);
        if (msb >= 52)
            mantissa = mantissa >> (msb - 52);
        else
            mantissa = mantissa << (52 - msb);

        // Clear the implicit 1 bit
        mantissa = mantissa & GetMantissaMask();

        // Combine sign, exponent, and mantissa
        UINT64 sign = negative ? GetSignMask() : UINT64(0, 0);
        UINT64 exp = UINT64((UINT32)exponent << 20, 0);
        bits = sign | exp | mantissa;
    }

    // Get the bit pattern
    constexpr UINT64 Bits() const noexcept { return bits; }

    // Pure integer-based conversion to INT32 (no FPU)
    NOINLINE DISABLE_OPTIMIZATION operator INT32() const noexcept
    {
        // Convert to INT64 first, then truncate to INT32
        INT64 val64 = (INT64)(*this);
        return (INT32)(val64.Low());
    }

    // Pure integer-based conversion to UINT32 (no FPU)
    NOINLINE DISABLE_OPTIMIZATION operator UINT32() const noexcept
    {
        // Convert to UINT64 first, then truncate to UINT32
        UINT64 val64 = (UINT64)(*this);
        return val64.Low();
    }

    // Pure integer-based conversion to INT64 (no FPU)
    NOINLINE DISABLE_OPTIMIZATION operator INT64() const noexcept
    {
        // Extract IEEE-754 components
        UINT64 sign_bit = bits & GetSignMask();
        UINT64 exp_bits = bits & GetExpMask();
        UINT64 mantissa_bits = bits & GetMantissaMask();

        // Extract exponent value (biased by 1023)
        int exponent = (int)((exp_bits >> EXP_SHIFT).Low()) - 1023;

        // Handle special cases
        if (exponent < 0)
        {
            // Value is less than 1.0, rounds to 0
            return INT64(0, 0);
        }

        if (exponent >= 63)
        {
            // Overflow: return max/min int64
            if (sign_bit.High() != 0)
                return INT64((INT32)0x80000000, 0); // INT64_MIN
            else
                return INT64((INT32)0x7FFFFFFF, 0xFFFFFFFF); // INT64_MAX
        }

        // Add implicit leading 1 bit to mantissa
        UINT64 mantissa_with_implicit_one = mantissa_bits | UINT64(0x00100000U, 0x00000000U);

        // Shift mantissa to align with integer representation
        // Mantissa has 52 bits after the implicit 1, so we have a 53-bit value
        // We need to shift it to align with the integer result
        UINT64 int_value;
        if (exponent <= 52)
        {
            // Shift right to get the integer part
            int_value = mantissa_with_implicit_one >> (52 - exponent);
        }
        else
        {
            // Shift left to scale up
            int_value = mantissa_with_implicit_one << (exponent - 52);
        }

        // Apply sign
        INT64 result = INT64((INT32)int_value.High(), int_value.Low());
        if (sign_bit.High() != 0)
        {
            result = -result;
        }

        return result;
    }

    // Pure integer-based conversion to UINT64 (no FPU)
    NOINLINE DISABLE_OPTIMIZATION operator UINT64() const noexcept
    {
        // Extract IEEE-754 components
        UINT64 sign_bit = bits & GetSignMask();
        UINT64 exp_bits = bits & GetExpMask();
        UINT64 mantissa_bits = bits & GetMantissaMask();

        // Extract exponent value (biased by 1023)
        int exponent = (int)((exp_bits >> EXP_SHIFT).Low()) - 1023;

        // Handle negative values
        if (sign_bit.High() != 0)
        {
            // Negative value converts to 0 for unsigned
            return UINT64(0, 0);
        }

        // Handle values less than 1.0
        if (exponent < 0)
        {
            return UINT64(0, 0);
        }

        // Handle overflow
        if (exponent >= 64)
        {
            return UINT64(0xFFFFFFFF, 0xFFFFFFFF); // UINT64_MAX
        }

        // Add implicit leading 1 bit to mantissa
        UINT64 mantissa_with_implicit_one = mantissa_bits | UINT64(0x00100000U, 0x00000000U);

        // Shift mantissa to align with integer representation
        UINT64 int_value;
        if (exponent <= 52)
        {
            // Shift right to get the integer part
            int_value = mantissa_with_implicit_one >> (52 - exponent);
        }
        else
        {
            // Shift left to scale up
            int_value = mantissa_with_implicit_one << (exponent - 52);
        }

        return int_value;
    }

    // Pure integer-based conversion to unsigned long long (no FPU)
    NOINLINE DISABLE_OPTIMIZATION operator unsigned long long() const noexcept
    {
        return (unsigned long long)((UINT64)(*this));
    }

    // Pure integer-based conversion to signed long long (no FPU)
    NOINLINE DISABLE_OPTIMIZATION operator signed long long() const noexcept
    {
        return (signed long long)((INT64)(*this));
    }

    // Pure integer-based conversion to unsigned long (no FPU)
    // Required for Linux where unsigned long is a distinct 64-bit type from unsigned long long
    NOINLINE DISABLE_OPTIMIZATION operator unsigned long() const noexcept
    {
        return (unsigned long)((UINT64)(*this));
    }

    // Pure integer-based conversion to signed long (no FPU)
    // Required for Linux where signed long is a distinct 64-bit type from signed long long
    NOINLINE DISABLE_OPTIMIZATION operator signed long() const noexcept
    {
        return (signed long)((INT64)(*this));
    }

    // Conversion to native double (use sparingly - only when needed for actual FP ops)
    NOINLINE DISABLE_OPTIMIZATION operator double() const noexcept
    {
        // Cannot use __builtin_bit_cast with non-trivially-copyable types
        // Use manual bit manipulation via unsigned long long
        unsigned long long ull = (unsigned long long)bits;
        return __builtin_bit_cast(double, ull);
    }

    // Assignment operators
    constexpr DOUBLE &operator=(const DOUBLE &other) noexcept
    {
        bits = other.bits;
        return *this;
    }

    constexpr DOUBLE &operator=(double val) noexcept
    {
        unsigned long long ull = __builtin_bit_cast(unsigned long long, val);
        bits = UINT64(ull);
        return *this;
    }

    // Comparison operators - use bit_cast to native double for comparisons
    NOINLINE DISABLE_OPTIMIZATION bool operator==(const DOUBLE &other) const noexcept
    {
        unsigned long long ull_a = (unsigned long long)bits;
        unsigned long long ull_b = (unsigned long long)other.bits;
        double a = __builtin_bit_cast(double, ull_a);
        double b = __builtin_bit_cast(double, ull_b);
        return a == b;
    }

    NOINLINE DISABLE_OPTIMIZATION bool operator!=(const DOUBLE &other) const noexcept
    {
        return !(*this == other);
    }

    NOINLINE DISABLE_OPTIMIZATION bool operator<(const DOUBLE &other) const noexcept
    {
        unsigned long long ull_a = (unsigned long long)bits;
        unsigned long long ull_b = (unsigned long long)other.bits;
        double a = __builtin_bit_cast(double, ull_a);
        double b = __builtin_bit_cast(double, ull_b);
        return a < b;
    }

    NOINLINE DISABLE_OPTIMIZATION bool operator<=(const DOUBLE &other) const noexcept
    {
        unsigned long long ull_a = (unsigned long long)bits;
        unsigned long long ull_b = (unsigned long long)other.bits;
        double a = __builtin_bit_cast(double, ull_a);
        double b = __builtin_bit_cast(double, ull_b);
        return a <= b;
    }

    NOINLINE DISABLE_OPTIMIZATION bool operator>(const DOUBLE &other) const noexcept
    {
        unsigned long long ull_a = (unsigned long long)bits;
        unsigned long long ull_b = (unsigned long long)other.bits;
        double a = __builtin_bit_cast(double, ull_a);
        double b = __builtin_bit_cast(double, ull_b);
        return a > b;
    }

    NOINLINE DISABLE_OPTIMIZATION bool operator>=(const DOUBLE &other) const noexcept
    {
        unsigned long long ull_a = (unsigned long long)bits;
        unsigned long long ull_b = (unsigned long long)other.bits;
        double a = __builtin_bit_cast(double, ull_a);
        double b = __builtin_bit_cast(double, ull_b);
        return a >= b;
    }

    // Arithmetic operators - use bit_cast for operations
    NOINLINE DISABLE_OPTIMIZATION DOUBLE operator+(const DOUBLE &other) const noexcept
    {
        unsigned long long ull_a = (unsigned long long)bits;
        unsigned long long ull_b = (unsigned long long)other.bits;
        double a = __builtin_bit_cast(double, ull_a);
        double b = __builtin_bit_cast(double, ull_b);
        double result = a + b;
        unsigned long long result_ull = __builtin_bit_cast(unsigned long long, result);
        return DOUBLE(UINT64(result_ull));
    }

    NOINLINE DISABLE_OPTIMIZATION DOUBLE operator-(const DOUBLE &other) const noexcept
    {
        unsigned long long ull_a = (unsigned long long)bits;
        unsigned long long ull_b = (unsigned long long)other.bits;
        double a = __builtin_bit_cast(double, ull_a);
        double b = __builtin_bit_cast(double, ull_b);
        double result = a - b;
        unsigned long long result_ull = __builtin_bit_cast(unsigned long long, result);
        return DOUBLE(UINT64(result_ull));
    }

    NOINLINE DISABLE_OPTIMIZATION DOUBLE operator*(const DOUBLE &other) const noexcept
    {
        unsigned long long ull_a = (unsigned long long)bits;
        unsigned long long ull_b = (unsigned long long)other.bits;
        double a = __builtin_bit_cast(double, ull_a);
        double b = __builtin_bit_cast(double, ull_b);
        double result = a * b;
        unsigned long long result_ull = __builtin_bit_cast(unsigned long long, result);
        return DOUBLE(UINT64(result_ull));
    }

    NOINLINE DISABLE_OPTIMIZATION DOUBLE operator/(const DOUBLE &other) const noexcept
    {
        unsigned long long ull_a = (unsigned long long)bits;
        unsigned long long ull_b = (unsigned long long)other.bits;
        double a = __builtin_bit_cast(double, ull_a);
        double b = __builtin_bit_cast(double, ull_b);
        double result = a / b;
        unsigned long long result_ull = __builtin_bit_cast(unsigned long long, result);
        return DOUBLE(UINT64(result_ull));
    }

    NOINLINE DISABLE_OPTIMIZATION DOUBLE operator-() const noexcept // Unary minus
    {
        // Flip the sign bit
        UINT64 newBits = bits ^ GetSignMask();
        return DOUBLE(newBits);
    }

    // Compound assignment operators
    NOINLINE DISABLE_OPTIMIZATION DOUBLE &operator+=(const DOUBLE &other) noexcept
    {
        *this = *this + other;
        return *this;
    }

    NOINLINE DISABLE_OPTIMIZATION DOUBLE &operator-=(const DOUBLE &other) noexcept
    {
        *this = *this - other;
        return *this;
    }

    NOINLINE DISABLE_OPTIMIZATION DOUBLE &operator*=(const DOUBLE &other) noexcept
    {
        *this = *this * other;
        return *this;
    }

    NOINLINE DISABLE_OPTIMIZATION DOUBLE &operator/=(const DOUBLE &other) noexcept
    {
        *this = *this / other;
        return *this;
    }

    // Operators with integer literals
    NOINLINE DISABLE_OPTIMIZATION bool operator<(INT32 val) const noexcept
    {
        return *this < DOUBLE(val);
    }

    NOINLINE DISABLE_OPTIMIZATION DOUBLE operator-(UINT64 val) const noexcept
    {
        // Interpret UINT64 as integer value, not as bit pattern
        unsigned long long ull_val = (unsigned long long)val;
        DOUBLE d_val = DOUBLE((INT32)ull_val); // Convert integer to DOUBLE
        return *this - d_val;
    }

    NOINLINE DISABLE_OPTIMIZATION DOUBLE operator-(unsigned long long val) const noexcept
    {
        DOUBLE d_val = DOUBLE((INT32)val);
        return *this - d_val;
    }

    // Required for Linux where unsigned long is a distinct 64-bit type
    NOINLINE DISABLE_OPTIMIZATION DOUBLE operator-(unsigned long val) const noexcept
    {
        DOUBLE d_val = DOUBLE((INT32)val);
        return *this - d_val;
    }

    NOINLINE DISABLE_OPTIMIZATION DOUBLE operator-(UINT32 val) const noexcept
    {
        DOUBLE d_val = DOUBLE((INT32)val);
        return *this - d_val;
    }
};

// ============================================================================
// POSITION-INDEPENDENT LITERAL OPERATORS
// ============================================================================

/**
 * _embed literal suffix for floating-point values
 *
 * Embeds floating-point literals directly in code without .rdata section refs.
 * The consteval specifier forces compile-time evaluation, ensuring the literal
 * is embedded as immediate values in the final binary with no .rdata references.
 *
 * Process:
 *   1. Compiler invokes consteval operator at compile time
 *   2. DOUBLE consteval constructor converts double to UINT64 bit pattern
 *   3. Final binary contains only immediate mov instructions
 *
 * Usage:
 *   DOUBLE pi = 3.14159_embed;         // No .rdata reference
 *   DOUBLE ratio = 42.0_embed;         // No .rdata reference
 *   DOUBLE negated = -5.0_embed;       // Uses DOUBLE::operator-()
 *   DOUBLE sum = 1.0_embed + 2.0_embed;// Arithmetic operations
 *
 * @param v Floating-point literal value
 * @return DOUBLE with embedded bit pattern
 */
consteval DOUBLE operator""_embed(long double v)
{
    return DOUBLE(static_cast<double>(v), DOUBLE::CompileTimeLiteral{});
}

/**
 * _embed literal suffix for integer values
 *
 * Converts integer literals to DOUBLE at compile time without .rdata references.
 * The consteval specifier ensures compile-time evaluation.
 *
 * Usage:
 *   DOUBLE whole = 42_embed;    // Integer 42 as DOUBLE (42.0)
 *   DOUBLE zero = 0_embed;       // Zero as DOUBLE
 *
 * @param value Integer literal value
 * @return DOUBLE representing the integer as floating-point
 */
consteval DOUBLE operator""_embed(unsigned long long value)
{
    return DOUBLE(static_cast<double>(value), DOUBLE::CompileTimeLiteral{});
}

// ============================================================================
// POINTER TYPES
// ============================================================================

typedef DOUBLE *PDOUBLE;

/**
 * DESIGN NOTES: Evolution to Simplicity
 *
 * Version 1 (Original):
 *   - Separate EMBEDDED_DOUBLE struct with complex word-packing
 *   - DOUBLE class for runtime operations
 *   - ~200+ lines of duplicate code
 *   - Two types to understand and maintain
 *
 * Version 2 (First Refactor):
 *   - Single DOUBLE class
 *   - Nested EmbeddedHelper with word-packing
 *   - ~90 fewer lines of code
 *
 * Version 3 (Current - Simplified):
 *   - Single DOUBLE class with simple consteval constructor
 *   - No complex word-packing needed (consteval handles it)
 *   - ~120 fewer lines than original
 *   - Simpler API: one type, one approach
 *
 * Key Insight:
 *   Modern C++20 consteval eliminates the need for manual word-packing.
 *   The private consteval constructor with tag dispatch ensures literals
 *   are evaluated at compile time, preventing .rdata creation without any
 *   complex intermediate structures.
 *
 * Benefits:
 *   - Dramatically simplified implementation
 *   - Eliminated unnecessary complexity (EmbeddedHelper removed)
 *   - Same PIC guarantees with less code
 *   - Backward compatible (_embed suffix works identically)
 *   - Easier to understand and maintain
 *
 * PIC Safety:
 *   The consteval constructor forces compile-time evaluation. The compiler
 *   generates immediate mov instructions directly, never creating .rdata
 *   entries. The linker merges any intermediate sections via /MERGE:.rdata=.text,
 *   resulting in a truly position-independent binary with only a .text section.
 */
