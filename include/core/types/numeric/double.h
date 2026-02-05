/**
 * double.h - Position-Independent Double Precision Floating-Point
 *
 * IEEE-754 double stored as UINT64 bit pattern with compile-time literal
 * embedding via _embed suffix. No .rdata section references (PIC-safe).
 *
 * Usage: DOUBLE x = 3.14_embed;
 */
#pragma once

#include "primitives.h"

/**
 * Position-independent IEEE-754 double precision floating-point
 *
 * Stored as UINT64 bit pattern. Use _embed suffix for compile-time literals
 * to avoid .rdata section references. Private consteval constructor ensures
 * compile-time evaluation for literals, public constexpr for runtime conversion.
 */
class DOUBLE
{
private:
    UINT64 bits;

    static constexpr INT32 SIGN_SHIFT = 63;
    static constexpr INT32 EXP_SHIFT = 52;

    static constexpr UINT64 GetSignMask() noexcept { return 0x8000000000000000ULL; }
    static constexpr UINT64 GetExpMask() noexcept { return 0x7FF0000000000000ULL; }
    static constexpr UINT64 GetMantissaMask() noexcept { return 0x000FFFFFFFFFFFFFULL; }

public:
    constexpr DOUBLE() noexcept : bits(0ULL) {}
    constexpr DOUBLE(const DOUBLE &) noexcept = default;
    constexpr explicit DOUBLE(UINT64 bitPattern) noexcept : bits(bitPattern) {}

    constexpr DOUBLE(double val) noexcept
    {
        UINT64 ull = __builtin_bit_cast(UINT64, val);
        bits = ull;
    }

private:
    // Private consteval constructor for _embed literals (prevents .rdata)
    struct CompileTimeLiteral {};
    consteval DOUBLE(double val, CompileTimeLiteral) noexcept
    {
        UINT64 ull = __builtin_bit_cast(UINT64, val);
        bits = ull;
    }

    friend consteval DOUBLE operator""_embed(long double v);
    friend consteval DOUBLE operator""_embed(UINT64 value);

    // Comparison operation enum for shared helper
    enum CmpOp { CMP_EQ, CMP_LT, CMP_LE, CMP_GT, CMP_GE };

    // Shared comparison helper to reduce code duplication
    NOINLINE DISABLE_OPTIMIZATION BOOL Compare(const DOUBLE &other, CmpOp op) const noexcept
    {
        UINT64 ull_a = bits;
        UINT64 ull_b = other.bits;
        double a = __builtin_bit_cast(double, ull_a);
        double b = __builtin_bit_cast(double, ull_b);
        switch (op)
        {
            case CMP_EQ: return a == b;
            case CMP_LT: return a < b;
            case CMP_LE: return a <= b;
            case CMP_GT: return a > b;
            case CMP_GE: return a >= b;
            default: return FALSE;
        }
    }

    // Arithmetic operation enum for shared helper
    enum ArithOp { OP_ADD, OP_SUB, OP_MUL, OP_DIV };

    // Shared arithmetic helper to reduce code duplication
    NOINLINE DISABLE_OPTIMIZATION DOUBLE Arithmetic(const DOUBLE &other, ArithOp op) const noexcept
    {
        UINT64 ull_a = bits;
        UINT64 ull_b = other.bits;
        double a = __builtin_bit_cast(double, ull_a);
        double b = __builtin_bit_cast(double, ull_b);
        double result;
        switch (op)
        {
            case OP_ADD: result = a + b; break;
            case OP_SUB: result = a - b; break;
            case OP_MUL: result = a * b; break;
            case OP_DIV: result = a / b; break;
            default: result = 0.0; break;
        }
        UINT64 result_ull = __builtin_bit_cast(UINT64, result);
        return DOUBLE(result_ull);
    }

public:
    /**
     * Parse a string to DOUBLE
     * @param s String to parse (supports sign, integer, and fractional parts)
     * @return Parsed DOUBLE value
     */
    static DOUBLE Parse(PCCHAR s) noexcept
    {
        // Initialize result variables (use INT32 constructor to avoid .rdata)
        DOUBLE sign = DOUBLE(INT32(1));
        DOUBLE result = DOUBLE(INT32(0));
        DOUBLE frac = DOUBLE(INT32(0));
        DOUBLE base = DOUBLE(INT32(1));
        DOUBLE tenDouble = DOUBLE(INT32(10));
        DOUBLE minusOne = DOUBLE(INT32(-1));
        // sign
        if (*s == '-')
        {
            sign = minusOne;
            s++;
        }
        else if (*s == '+')
        {
            s++;
        }

        // integer part
        while (*s >= '0' && *s <= '9')
        {
            result = result * tenDouble + DOUBLE(INT32(*s - '0'));
            s++;
        }

        // fractional part
        if (*s == '.')
        {
            s++; // skip the decimal point
            while (*s >= '0' && *s <= '9')
            {
                frac = frac * tenDouble + DOUBLE(INT32(*s - '0'));
                base = base * tenDouble;
                s++;
            }
        }

        return sign * (result + frac / base);
    }

    constexpr DOUBLE(INT32 val) noexcept
    {
        if (val == 0)
        {
            bits = 0ULL;
            return;
        }

        BOOL negative = val < 0;
        UINT32 absVal = negative ? (UINT32)(-val) : (UINT32)val;

        INT32 msb = 31;
        while (msb >= 0 && !((absVal >> msb) & 1))
            msb--;

        INT32 exponent = 1023 + msb;

        UINT64 mantissa = (UINT64)absVal;
        if (msb >= 52)
            mantissa = mantissa >> (msb - 52);
        else
            mantissa = mantissa << (52 - msb);

        mantissa = mantissa & GetMantissaMask();

        UINT64 sign = negative ? GetSignMask() : 0ULL;
        UINT64 exp = (UINT64)exponent << 52;
        bits = sign | exp | mantissa;
    }

    constexpr DOUBLE(INT64 val) noexcept
    {
        if (val == 0)
        {
            bits = 0ULL;
            return;
        }

        BOOL negative = val < 0;
        // Handle INT64_MIN specially to avoid overflow in negation
        UINT64 absVal;
        if (val == (INT64)0x8000000000000000LL)
        {
            absVal = 0x8000000000000000ULL;
        }
        else
        {
            absVal = negative ? (UINT64)(-val) : (UINT64)val;
        }

        // Find the MSB position (highest set bit)
        INT32 msb = 63;
        while (msb >= 0 && !((absVal >> msb) & 1))
            msb--;

        INT32 exponent = 1023 + msb;

        // Shift mantissa to fit in 52 bits (may lose precision for large values)
        UINT64 mantissa = absVal;
        if (msb >= 52)
            mantissa = mantissa >> (msb - 52);
        else
            mantissa = mantissa << (52 - msb);

        mantissa = mantissa & GetMantissaMask();

        UINT64 sign = negative ? GetSignMask() : 0ULL;
        UINT64 exp = (UINT64)exponent << 52;
        bits = sign | exp | mantissa;
    }

    constexpr UINT64 Bits() const noexcept { return bits; }

    NOINLINE DISABLE_OPTIMIZATION operator INT32() const noexcept
    {
        INT64 val64 = (INT64)(*this);
        return (INT32)val64;
    }

    NOINLINE DISABLE_OPTIMIZATION operator UINT32() const noexcept
    {
        UINT64 val64 = (UINT64)(*this);
        return (UINT32)val64;
    }

    NOINLINE DISABLE_OPTIMIZATION operator INT64() const noexcept
    {
        UINT64 sign_bit = bits & GetSignMask();
        UINT64 exp_bits = bits & GetExpMask();
        UINT64 mantissa_bits = bits & GetMantissaMask();

        INT32 exponent = (INT32)(UINT32)(exp_bits >> EXP_SHIFT) - 1023;

        if (exponent < 0)
            return 0LL;

        if (exponent >= 63)
        {
            if ((sign_bit >> 32) != 0)
                return 0x8000000000000000LL; // INT64_MIN
            else
                return 0x7FFFFFFFFFFFFFFFLL; // INT64_MAX
        }

        UINT64 mantissa_with_implicit_one = mantissa_bits | 0x0010000000000000ULL;

        UINT64 int_value;
        if (exponent <= 52)
            int_value = mantissa_with_implicit_one >> (52 - exponent);
        else
            int_value = mantissa_with_implicit_one << (exponent - 52);

        INT64 result = (INT64)int_value;
        if ((sign_bit >> 32) != 0)
            result = -result;

        return result;
    }

    NOINLINE DISABLE_OPTIMIZATION operator UINT64() const noexcept
    {
        UINT64 sign_bit = bits & GetSignMask();
        UINT64 exp_bits = bits & GetExpMask();
        UINT64 mantissa_bits = bits & GetMantissaMask();

        INT32 exponent = (INT32)(UINT32)(exp_bits >> EXP_SHIFT) - 1023;

        if ((sign_bit >> 32) != 0)
            return 0ULL;

        if (exponent < 0)
            return 0ULL;

        if (exponent >= 64)
            return 0xFFFFFFFFFFFFFFFFULL; // UINT64_MAX

        UINT64 mantissa_with_implicit_one = mantissa_bits | 0x0010000000000000ULL;

        UINT64 int_value;
        if (exponent <= 52)
            int_value = mantissa_with_implicit_one >> (52 - exponent);
        else
            int_value = mantissa_with_implicit_one << (exponent - 52);

        return int_value;
    }

    NOINLINE DISABLE_OPTIMIZATION operator double() const noexcept
    {
        UINT64 ull = bits;
        return __builtin_bit_cast(double, ull);
    }

    constexpr DOUBLE &operator=(const DOUBLE &other) noexcept
    {
        bits = other.bits;
        return *this;
    }

    constexpr DOUBLE &operator=(double val) noexcept
    {
        UINT64 ull = __builtin_bit_cast(UINT64, val);
        bits = ull;
        return *this;
    }

    BOOL operator==(const DOUBLE &other) const noexcept { return Compare(other, CMP_EQ); }
    BOOL operator!=(const DOUBLE &other) const noexcept { return !Compare(other, CMP_EQ); }
    BOOL operator<(const DOUBLE &other) const noexcept { return Compare(other, CMP_LT); }
    BOOL operator<=(const DOUBLE &other) const noexcept { return Compare(other, CMP_LE); }
    BOOL operator>(const DOUBLE &other) const noexcept { return Compare(other, CMP_GT); }
    BOOL operator>=(const DOUBLE &other) const noexcept { return Compare(other, CMP_GE); }

    DOUBLE operator+(const DOUBLE &other) const noexcept { return Arithmetic(other, OP_ADD); }
    DOUBLE operator-(const DOUBLE &other) const noexcept { return Arithmetic(other, OP_SUB); }
    DOUBLE operator*(const DOUBLE &other) const noexcept { return Arithmetic(other, OP_MUL); }
    DOUBLE operator/(const DOUBLE &other) const noexcept { return Arithmetic(other, OP_DIV); }

    NOINLINE DISABLE_OPTIMIZATION DOUBLE operator-() const noexcept
    {
        UINT64 newBits = bits ^ GetSignMask();
        return DOUBLE(newBits);
    }

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

    NOINLINE DISABLE_OPTIMIZATION BOOL operator<(INT32 val) const noexcept
    {
        return *this < DOUBLE(val);
    }

    NOINLINE DISABLE_OPTIMIZATION DOUBLE operator-(UINT64 val) const noexcept
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

// Compile-time literal operators (consteval prevents .rdata)
consteval DOUBLE operator""_embed(long double v)
{
    return DOUBLE(static_cast<double>(v), DOUBLE::CompileTimeLiteral{});
}

consteval DOUBLE operator""_embed(UINT64 value)
{
    return DOUBLE(static_cast<double>(value), DOUBLE::CompileTimeLiteral{});
}

typedef DOUBLE *PDOUBLE;
