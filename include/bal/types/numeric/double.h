/**
 * double.h - Position-Independent Double Precision Floating-Point
 *
 * IEEE-754 double stored as UINT64 bit pattern with compile-time literal
 * embedding via _embed suffix. No .rdata section references (PIC-safe).
 *
 * Usage: DOUBLE x = 3.14_embed;
 */
#pragma once

#include "uint64.h"

// Forward declaration
class INT64;

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

    static constexpr int SIGN_SHIFT = 63;
    static constexpr int EXP_SHIFT = 52;

    static constexpr UINT64 GetSignMask() noexcept { return UINT64(0x80000000U, 0x00000000U); }
    static constexpr UINT64 GetExpMask() noexcept { return UINT64(0x7FF00000U, 0x00000000U); }
    static constexpr UINT64 GetMantissaMask() noexcept { return UINT64(0x000FFFFFU, 0xFFFFFFFFU); }

public:
    constexpr DOUBLE() noexcept : bits(0, 0) {}
    constexpr DOUBLE(const DOUBLE &) noexcept = default;
    constexpr explicit DOUBLE(UINT64 bitPattern) noexcept : bits(bitPattern) {}
    constexpr DOUBLE(UINT32 high, UINT32 low) noexcept : bits(high, low) {}

    constexpr DOUBLE(double val) noexcept
    {
        unsigned long long ull = __builtin_bit_cast(unsigned long long, val);
        bits = UINT64(ull);
    }

private:
    // Private consteval constructor for _embed literals (prevents .rdata)
    struct CompileTimeLiteral {};
    consteval DOUBLE(double val, CompileTimeLiteral) noexcept
    {
        unsigned long long ull = __builtin_bit_cast(unsigned long long, val);
        bits = UINT64(ull);
    }

    friend consteval DOUBLE operator""_embed(long double v);
    friend consteval DOUBLE operator""_embed(unsigned long long value);

public:
    /**
     * Parse a string to DOUBLE
     * @param s String to parse (supports sign, integer, and fractional parts)
     * @return Parsed DOUBLE value
     */
    static DOUBLE Parse(const char *s) noexcept
    {
        // Initialize result variables
        DOUBLE sign = 1.0;
        DOUBLE result = 0.0;
        DOUBLE frac = 0.0;
        DOUBLE base = 1.0;
        DOUBLE tenDouble = 10.0;
        DOUBLE minusOne = -1.0;
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
            bits = UINT64(0, 0);
            return;
        }

        bool negative = val < 0;
        UINT32 absVal = negative ? (UINT32)(-val) : (UINT32)val;

        int msb = 31;
        while (msb >= 0 && !((absVal >> msb) & 1))
            msb--;

        int exponent = 1023 + msb;

        UINT64 mantissa = UINT64(absVal);
        if (msb >= 52)
            mantissa = mantissa >> (msb - 52);
        else
            mantissa = mantissa << (52 - msb);

        mantissa = mantissa & GetMantissaMask();

        UINT64 sign = negative ? GetSignMask() : UINT64(0, 0);
        UINT64 exp = UINT64((UINT32)exponent << 20, 0);
        bits = sign | exp | mantissa;
    }

    constexpr UINT64 Bits() const noexcept { return bits; }

    NOINLINE DISABLE_OPTIMIZATION operator INT32() const noexcept
    {
        INT64 val64 = (INT64)(*this);
        return (INT32)(val64.Low());
    }

    NOINLINE DISABLE_OPTIMIZATION operator UINT32() const noexcept
    {
        UINT64 val64 = (UINT64)(*this);
        return val64.Low();
    }

    NOINLINE DISABLE_OPTIMIZATION operator INT64() const noexcept
    {
        UINT64 sign_bit = bits & GetSignMask();
        UINT64 exp_bits = bits & GetExpMask();
        UINT64 mantissa_bits = bits & GetMantissaMask();

        int exponent = (int)((exp_bits >> EXP_SHIFT).Low()) - 1023;

        if (exponent < 0)
            return INT64(0, 0);

        if (exponent >= 63)
        {
            if (sign_bit.High() != 0)
                return INT64((INT32)0x80000000, 0);
            else
                return INT64((INT32)0x7FFFFFFF, 0xFFFFFFFF);
        }

        UINT64 mantissa_with_implicit_one = mantissa_bits | UINT64(0x00100000U, 0x00000000U);

        UINT64 int_value;
        if (exponent <= 52)
            int_value = mantissa_with_implicit_one >> (52 - exponent);
        else
            int_value = mantissa_with_implicit_one << (exponent - 52);

        INT64 result = INT64((INT32)int_value.High(), int_value.Low());
        if (sign_bit.High() != 0)
            result = -result;

        return result;
    }

    NOINLINE DISABLE_OPTIMIZATION operator UINT64() const noexcept
    {
        UINT64 sign_bit = bits & GetSignMask();
        UINT64 exp_bits = bits & GetExpMask();
        UINT64 mantissa_bits = bits & GetMantissaMask();

        int exponent = (int)((exp_bits >> EXP_SHIFT).Low()) - 1023;

        if (sign_bit.High() != 0)
            return UINT64(0, 0);

        if (exponent < 0)
            return UINT64(0, 0);

        if (exponent >= 64)
            return UINT64(0xFFFFFFFF, 0xFFFFFFFF);

        UINT64 mantissa_with_implicit_one = mantissa_bits | UINT64(0x00100000U, 0x00000000U);

        UINT64 int_value;
        if (exponent <= 52)
            int_value = mantissa_with_implicit_one >> (52 - exponent);
        else
            int_value = mantissa_with_implicit_one << (exponent - 52);

        return int_value;
    }

    NOINLINE DISABLE_OPTIMIZATION operator unsigned long long() const noexcept
    {
        return (unsigned long long)((UINT64)(*this));
    }

    NOINLINE DISABLE_OPTIMIZATION operator signed long long() const noexcept
    {
        return (signed long long)((INT64)(*this));
    }

    NOINLINE DISABLE_OPTIMIZATION operator unsigned long() const noexcept
    {
        return (unsigned long)((UINT64)(*this));
    }

    NOINLINE DISABLE_OPTIMIZATION operator signed long() const noexcept
    {
        return (signed long)((INT64)(*this));
    }

    NOINLINE DISABLE_OPTIMIZATION operator double() const noexcept
    {
        unsigned long long ull = (unsigned long long)bits;
        return __builtin_bit_cast(double, ull);
    }

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

    NOINLINE DISABLE_OPTIMIZATION bool operator<(INT32 val) const noexcept
    {
        return *this < DOUBLE(val);
    }

    NOINLINE DISABLE_OPTIMIZATION DOUBLE operator-(UINT64 val) const noexcept
    {
        unsigned long long ull_val = (unsigned long long)val;
        DOUBLE d_val = DOUBLE((INT32)ull_val);
        return *this - d_val;
    }

    NOINLINE DISABLE_OPTIMIZATION DOUBLE operator-(unsigned long long val) const noexcept
    {
        DOUBLE d_val = DOUBLE((INT32)val);
        return *this - d_val;
    }

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

// Compile-time literal operators (consteval prevents .rdata)
consteval DOUBLE operator""_embed(long double v)
{
    return DOUBLE(static_cast<double>(v), DOUBLE::CompileTimeLiteral{});
}

consteval DOUBLE operator""_embed(unsigned long long value)
{
    return DOUBLE(static_cast<double>(value), DOUBLE::CompileTimeLiteral{});
}

typedef DOUBLE *PDOUBLE;
