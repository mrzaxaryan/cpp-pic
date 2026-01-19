#pragma once

#include "uint64.h"
#include "int64_common.h"

/**
 * INT64 - Position-independent 64-bit signed integer class
 *
 * Prevents compiler from using native 64-bit types that could generate
 * .rdata section references. All operations are implemented manually
 * using the UINT64 class for unsigned operations and handling sign properly.
 */
class INT64
{
private:
    UINT32 low;   // Lower 32 bits
    INT32 high;   // Upper 32 bits (signed for proper sign extension)

public:
    // Maximum value for INT64: 0x7FFFFFFFFFFFFFFF (9,223,372,036,854,775,807)
    static constexpr INT64 MAX() noexcept
    {
        return INT64((INT32)0x7FFFFFFF, (UINT32)0xFFFFFFFF);
    }
    static constexpr INT64 MIN() noexcept
    {
        return INT64((INT32)0x80000000, (UINT32)0x00000000);
    }
    // Default constructor
    constexpr INT64() noexcept : low(0), high(0) {}

    // Copy constructor (default is fine, but explicit to avoid warnings)
    constexpr INT64(const INT64 &) noexcept = default;

    // Constructor from two 32-bit words (high and low)
    constexpr INT64(INT32 h, UINT32 l) noexcept : low(l), high(h) {}

    // Constructors from 8-bit integer types
    constexpr INT64(UINT8 val) noexcept : low((UINT32)val), high(0) {}
    constexpr INT64(INT8 val) noexcept : low((UINT32)val), high(val < 0 ? -1 : 0) {}

    // Constructors from 16-bit integer types
    constexpr INT64(UINT16 val) noexcept : low((UINT32)val), high(0) {}
    constexpr INT64(INT16 val) noexcept : low((UINT32)val), high(val < 0 ? -1 : 0) {}

    // Constructors from 32-bit integer types
    constexpr INT64(INT32 val) noexcept : low((UINT32)val), high(val < 0 ? -1 : 0) {}
    constexpr explicit INT64(UINT32 val) noexcept : low(val), high(0) {}

    // Constructor from UINT64 (explicit to avoid ambiguity)
    constexpr explicit INT64(const UINT64 &val) noexcept
        : low(val.Low()), high((INT32)val.High()) {}

    // Constructor from native 64-bit signed type (for compatibility with literals and native code)
    constexpr INT64(signed long long val) noexcept
        : low((UINT32)(val & 0xFFFFFFFFLL)),
          high((INT32)((val >> 32) & 0xFFFFFFFFLL))
    {
    }

    // Get low 32 bits
    constexpr UINT32 Low() const noexcept { return low; }

    // Get high 32 bits
    constexpr INT32 High() const noexcept { return high; }

    // Conversion to signed long long
    constexpr operator signed long long() const noexcept
    {
        return ((signed long long)high << 32) | (signed long long)low;
    }

    // Conversion to UINT64
    constexpr operator UINT64() const noexcept
    {
        return UINT64((UINT32)high, low);
    }

    // Assignment operators
    constexpr INT64 &operator=(const INT64 &other) noexcept
    {
        low = other.low;
        high = other.high;
        return *this;
    }

    constexpr INT64 &operator=(INT32 val) noexcept
    {
        low = (UINT32)val;
        high = val < 0 ? -1 : 0;
        return *this;
    }

    // Comparison operators (generated via int64_common.h macros)
    DEFINE_INT64_COMPARISON_OPERATORS(INT64)

    // Comparison operators with scalar types
    constexpr bool operator<(INT32 val) const noexcept
    {
        return *this < INT64(val);
    }

    constexpr bool operator<=(INT32 val) const noexcept
    {
        INT32 valHigh = val < 0 ? -1 : 0;
        if (high != valHigh)
            return high < valHigh;
        return low <= (UINT32)val;
    }

    constexpr bool operator>(INT32 val) const noexcept
    {
        INT32 valHigh = val < 0 ? -1 : 0;
        if (high != valHigh)
            return high > valHigh;
        return low > (UINT32)val;
    }

    constexpr bool operator>=(INT32 val) const noexcept
    {
        INT32 valHigh = val < 0 ? -1 : 0;
        if (high != valHigh)
            return high > valHigh;
        return low >= (UINT32)val;
    }

    constexpr bool operator==(INT32 val) const noexcept
    {
        INT32 valHigh = val < 0 ? -1 : 0;
        return (high == valHigh) && (low == (UINT32)val);
    }

    constexpr bool operator!=(INT32 val) const noexcept
    {
        INT32 valHigh = val < 0 ? -1 : 0;
        return (high != valHigh) || (low != (UINT32)val);
    }

    // Basic arithmetic operators (generated via int64_common.h macros)
    DEFINE_INT64_BASIC_ARITHMETIC(INT64, INT32)

    constexpr INT64 operator-() const noexcept // Unary minus
    {
        return INT64(0, 0) - *this;
    }

    constexpr INT64 operator*(const INT64 &other) const noexcept
    {
        // Convert to unsigned for multiplication, then cast back
        UINT64 a = UINT64((UINT32)high, low);
        UINT64 b = UINT64((UINT32)other.high, other.low);
        UINT64 result = a * b;

        return INT64((INT32)result.High(), result.Low());
    }

    constexpr INT64 operator/(const INT64 &other) const noexcept
    {
        if (other.high == 0 && other.low == 0)
            return INT64(0, 0); // Division by zero

        // Handle signs
        bool negResult = (high < 0) != (other.high < 0);

        // Get absolute values
        INT64 absThis = (high < 0) ? -(*this) : *this;
        INT64 absOther = (other.high < 0) ? -other : other;

        // Convert to unsigned and divide
        UINT64 dividend = UINT64((UINT32)absThis.high, absThis.low);
        UINT64 divisor = UINT64((UINT32)absOther.high, absOther.low);
        UINT64 quotient = dividend / divisor;

        INT64 result = INT64((INT32)quotient.High(), quotient.Low());
        return negResult ? -result : result;
    }

    constexpr INT64 operator%(const INT64 &other) const noexcept
    {
        if (other.high == 0 && other.low == 0)
            return INT64(0, 0);

        INT64 quotient = *this / other;
        return *this - (quotient * other);
    }

    constexpr INT64 operator%(INT32 val) const noexcept
    {
        return *this % INT64(val);
    }

    constexpr INT64 operator+(INT32 val) const noexcept
    {
        return *this + INT64(val);
    }

    constexpr INT64 operator-(INT32 val) const noexcept
    {
        return *this - INT64(val);
    }

    constexpr INT64 operator*(INT32 val) const noexcept
    {
        return *this * INT64(val);
    }

    constexpr INT64 operator/(INT32 val) const noexcept
    {
        return *this / INT64(val);
    }

    // Bitwise operators (generated via int64_common.h macros)
    DEFINE_INT64_BITWISE_OPERATORS(INT64)

    // Left shift operators (generated via int64_common.h macros)
    DEFINE_INT64_LEFT_SHIFT(INT64, INT32)

    // Right shift operator (signed - arithmetic shift with sign extension)
    constexpr INT64 operator>>(int shift) const noexcept
    {
        if (shift < 0)
            return *this;
        if (shift >= 64)
            return INT64(high < 0 ? -1 : 0, high < 0 ? 0xFFFFFFFF : 0);
        if (shift == 0)
            return *this;
        if (shift >= 32)
            return INT64(high < 0 ? -1 : 0, (UINT32)(high >> (shift - 32)));

        return INT64(high >> shift, (low >> shift) | ((UINT32)high << (32 - shift)));
    }

    constexpr INT64 operator>>(UINT32 shift) const noexcept
    {
        return *this >> (int)shift;
    }

    // Arithmetic compound assignment operators (generated via int64_common.h macros)
    DEFINE_INT64_ARITHMETIC_ASSIGNMENTS(INT64, INT32)

    // Compound bitwise assignment operators (generated via int64_common.h macros)
    DEFINE_INT64_BITWISE_ASSIGNMENTS(INT64)

    // Left shift assignment operator (generated via int64_common.h macros)
    DEFINE_INT64_LEFT_SHIFT_ASSIGN(INT64, INT32)

    constexpr INT64 &operator>>=(int shift) noexcept
    {
        if (shift < 0)
        {
            // Nothing to do
        }
        else if (shift >= 64)
        {
            high = high < 0 ? -1 : 0;
            low = high < 0 ? 0xFFFFFFFF : 0;
        }
        else if (shift == 0)
        {
            // Nothing to do
        }
        else if (shift >= 32)
        {
            low = (UINT32)(high >> (shift - 32));
            high = high < 0 ? -1 : 0;
        }
        else
        {
            low = (low >> shift) | ((UINT32)high << (32 - shift));
            high = high >> shift;
        }
        return *this;
    }

    // Compound assignment operators for scalar types
    constexpr INT64 &operator+=(INT32 val) noexcept
    {
        return *this += INT64(val);
    }

    constexpr INT64 &operator-=(INT32 val) noexcept
    {
        return *this -= INT64(val);
    }

    constexpr INT64 &operator*=(INT32 val) noexcept
    {
        return *this *= INT64(val);
    }

    constexpr INT64 &operator/=(INT32 val) noexcept
    {
        return *this /= INT64(val);
    }

    constexpr INT64 &operator%=(INT32 val) noexcept
    {
        return *this %= INT64(val);
    }

    constexpr INT64 &operator+=(UINT64 val) noexcept
    {
        return *this += INT64((signed long long)val);
    }

    constexpr INT64 &operator-=(UINT64 val) noexcept
    {
        return *this -= INT64((signed long long)val);
    }

    // Increment/Decrement operators (generated via int64_common.h macros)
    DEFINE_INT64_INCREMENT_DECREMENT(INT64)
};

// Pointer types
typedef INT64 *PINT64;
typedef INT64 **PPINT64;
