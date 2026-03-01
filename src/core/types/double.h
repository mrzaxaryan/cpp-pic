/**
 * @file double.h
 * @brief Position-Independent Double Precision Floating-Point Type
 *
 * @details Provides a position-independent IEEE-754 double precision floating-point
 * type that stores values as UINT64 bit patterns. Uses the `_embed` suffix for
 * compile-time literal embedding to avoid .rdata section references.
 *
 * Key Features:
 * - IEEE-754 compliant double precision (64-bit)
 * - Stored as raw bit pattern (UINT64) for position independence
 * - Compile-time literal embedding via `_embed` suffix
 * - Full arithmetic operator support (+, -, *, /)
 * - Comparison operators (==, !=, <, <=, >, >=)
 * - Conversion to/from integer types
 * - String parsing
 *
 * @par Why This Class Exists:
 * Standard floating-point literals (e.g., `3.14`) are placed in .rdata sections
 * by the compiler, creating relocations that break position-independent code.
 * This class embeds the IEEE-754 bit pattern as a 64-bit immediate value directly
 * in the instruction stream.
 *
 * @see IEEE 754-2019 — IEEE Standard for Floating-Point Arithmetic
 *      https://standards.ieee.org/ieee/754/6210/
 * @see IEEE 754 Double-precision format — Wikipedia reference
 *      https://en.wikipedia.org/wiki/Double-precision_floating-point_format
 *
 * @par Example Usage:
 * @code
 * // Use _embed suffix for compile-time literals (no .rdata)
 * DOUBLE pi = 3.14159_embed;
 * DOUBLE half = 0.5_embed;
 *
 * // Arithmetic operations
 * DOUBLE result = pi * half;
 *
 * // Convert from integer (no .rdata needed)
 * DOUBLE intVal = DOUBLE(INT32(42));
 *
 * // Parse from string
 * DOUBLE parsed = DOUBLE::Parse("3.14159");
 * @endcode
 *
 * @ingroup core
 *
 * @defgroup double_type Double Type
 * @ingroup core
 * @{
 */

#pragma once

#include "primitives.h"

/**
 * @class DOUBLE
 * @brief Position-independent IEEE-754 double precision floating-point
 *
 * @details Stores the IEEE-754 bit pattern as UINT64 for position independence.
 * Use `_embed` suffix for compile-time literals to prevent .rdata generation.
 *
 * @par IEEE-754 Double-Precision Format (64 bits):
 * - Sign: 1 bit (bit 63)
 * - Exponent: 11 bits (bits 52-62), biased by 1023
 * - Mantissa: 52 bits (bits 0-51), with implicit leading 1
 *
 * @see IEEE 754-2019 Section 3.4 — Binary interchange format encodings
 *      https://standards.ieee.org/ieee/754/6210/
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
	/// @name Constructors
	/// @{

	/** @brief Default constructor — initializes to positive zero (+0.0) */
	constexpr DOUBLE() noexcept : bits(0ULL) {}
	/** @brief Copy constructor */
	constexpr DOUBLE(const DOUBLE &) noexcept = default;
	/**
	 * @brief Construct from raw IEEE-754 bit pattern
	 * @param bitPattern 64-bit IEEE-754 representation
	 */
	constexpr explicit DOUBLE(UINT64 bitPattern) noexcept : bits(bitPattern) {}

	/**
	 * @brief Construct from native double via __builtin_bit_cast
	 * @param val Native double value
	 *
	 * @details Uses __builtin_bit_cast to extract the IEEE-754 bit pattern
	 * at compile time without invoking undefined behavior.
	 *
	 * @see IEEE 754-2019 Section 3.4 — Binary interchange format encodings
	 *      https://standards.ieee.org/ieee/754/6210/
	 */
	constexpr DOUBLE(double val) noexcept
	{
		UINT64 ull = __builtin_bit_cast(UINT64, val);
		bits = ull;
	}

	/// @}

private:
	// Private consteval constructor for _embed literals (prevents .rdata)
	struct CompileTimeLiteral
	{
	};
	consteval DOUBLE(double val, CompileTimeLiteral) noexcept
	{
		UINT64 ull = __builtin_bit_cast(UINT64, val);
		bits = ull;
	}

	friend consteval DOUBLE operator""_embed(long double v);
	friend consteval DOUBLE operator""_embed(UINT64 value);

	// Comparison operation enum for shared helper
	enum CmpOp
	{
		CMP_EQ,
		CMP_LT,
		CMP_LE,
		CMP_GT,
		CMP_GE
	};

	// Shared comparison helper to reduce code duplication
	// Uses if-else chain instead of switch to prevent jump table generation
	// at -Os/-Oz with LTO (jump tables create constant pool entries that can
	// break position independence in PIC shellcode)
	NOINLINE DISABLE_OPTIMIZATION BOOL Compare(const DOUBLE &other, CmpOp op) const noexcept
	{
		UINT64 ull_a = bits;
		UINT64 ull_b = other.bits;
		double a = __builtin_bit_cast(double, ull_a);
		double b = __builtin_bit_cast(double, ull_b);
		if (op == CMP_EQ)
			return a == b;
		if (op == CMP_LT)
			return a < b;
		if (op == CMP_LE)
			return a <= b;
		if (op == CMP_GT)
			return a > b;
		if (op == CMP_GE)
			return a >= b;
		return false;
	}

	// Arithmetic operation enum for shared helper
	enum ArithOp
	{
		OP_ADD,
		OP_SUB,
		OP_MUL,
		OP_DIV
	};

	// Shared arithmetic helper to reduce code duplication
	// Uses if-else chain instead of switch to prevent jump table generation
	// at -Os/-Oz with LTO (jump tables create constant pool entries that can
	// break position independence in PIC shellcode)
	NOINLINE DISABLE_OPTIMIZATION DOUBLE Arithmetic(const DOUBLE &other, ArithOp op) const noexcept
	{
		UINT64 ull_a = bits;
		UINT64 ull_b = other.bits;
		double a = __builtin_bit_cast(double, ull_a);
		double b = __builtin_bit_cast(double, ull_b);
		double result;
		if (op == OP_ADD)
			result = a + b;
		else if (op == OP_SUB)
			result = a - b;
		else if (op == OP_MUL)
			result = a * b;
		else
			result = a / b;
		UINT64 result_ull = __builtin_bit_cast(UINT64, result);
		return DOUBLE(result_ull);
	}

public:
	/// @name Parsing
	/// @{

	/**
	 * @brief Parse a decimal string to DOUBLE
	 * @param s Null-terminated string to parse (supports sign, integer, and fractional parts)
	 * @return Parsed DOUBLE value, or +0.0 if s is null
	 *
	 * @details Performs manual decimal-to-binary floating-point conversion without
	 * CRT functions (strtod, atof). Handles optional sign, integer digits, and
	 * fractional digits separated by '.'.
	 */
	static DOUBLE Parse(PCCHAR s) noexcept
	{
		if (!s)
			return DOUBLE(INT32(0));

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

	/// @}

	/**
	 * @brief Construct from a 32-bit signed integer
	 * @param val Integer value to convert
	 *
	 * @details Converts val to IEEE-754 double-precision by computing the
	 * sign, exponent (biased by 1023), and mantissa fields from the integer's
	 * binary representation. Handles zero and negative values.
	 *
	 * @see IEEE 754-2019 Section 5.4.1 — Arithmetic operations (conversion)
	 *      https://standards.ieee.org/ieee/754/6210/
	 */
	constexpr DOUBLE(INT32 val) noexcept
	{
		if (val == 0)
		{
			bits = 0ULL;
			return;
		}

		BOOL negative = val < 0;
		UINT32 absVal = negative ? (UINT32)0 - (UINT32)val : (UINT32)val;

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

	/**
	 * @brief Construct from a 64-bit signed integer
	 * @param val Integer value to convert
	 *
	 * @details Converts val to IEEE-754 double-precision. May lose precision
	 * for values exceeding 2^53 (the mantissa capacity). Handles INT64_MIN
	 * specially to avoid undefined behavior in negation.
	 *
	 * @see IEEE 754-2019 Section 5.4.1 — Arithmetic operations (conversion)
	 *      https://standards.ieee.org/ieee/754/6210/
	 */
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

	/// @name Accessors
	/// @{

	/** @brief Get the raw IEEE-754 bit pattern */
	constexpr UINT64 Bits() const noexcept { return bits; }

	/// @}
	/// @name Conversion Operators
	/// @{

	/**
	 * @brief Convert to INT32 by truncating toward zero
	 * @return Truncated 32-bit signed integer value
	 *
	 * @details Extracts the integer value from the IEEE-754 representation
	 * using pure bitwise operations — no FPU conversion instructions that
	 * might reference .rdata constants.
	 *
	 * @see IEEE 754-2019 Section 5.8 — Details of conversions to integer formats
	 *      https://standards.ieee.org/ieee/754/6210/
	 */
	NOINLINE DISABLE_OPTIMIZATION operator INT32() const noexcept
	{
		INT64 val64 = (INT64)(*this);
		return (INT32)val64;
	}

	/**
	 * @brief Convert to UINT32 by truncating toward zero
	 * @return Truncated 32-bit unsigned integer value (0 for negative values)
	 */
	NOINLINE DISABLE_OPTIMIZATION operator UINT32() const noexcept
	{
		UINT64 val64 = (UINT64)(*this);
		return (UINT32)val64;
	}

	/**
	 * @brief Convert to INT64 by truncating toward zero
	 * @return Truncated 64-bit signed integer value
	 *
	 * @details Extracts sign, exponent, and mantissa from the IEEE-754 bit
	 * pattern using pure integer operations. Returns INT64_MIN / INT64_MAX
	 * for out-of-range values.
	 *
	 * @see IEEE 754-2019 Section 5.8 — Details of conversions to integer formats
	 *      https://standards.ieee.org/ieee/754/6210/
	 */
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

	/**
	 * @brief Convert to UINT64 by truncating toward zero
	 * @return Truncated 64-bit unsigned integer value (0 for negative values)
	 */
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

	/**
	 * @brief Convert to native double via __builtin_bit_cast
	 * @return Native double value
	 */
	NOINLINE DISABLE_OPTIMIZATION operator double() const noexcept
	{
		UINT64 ull = bits;
		return __builtin_bit_cast(double, ull);
	}

	/// @}
	/// @name Assignment Operators
	/// @{

	/** @brief Copy-assign from another DOUBLE */
	constexpr DOUBLE &operator=(const DOUBLE &other) noexcept
	{
		bits = other.bits;
		return *this;
	}

	/** @brief Assign from native double */
	constexpr DOUBLE &operator=(double val) noexcept
	{
		UINT64 ull = __builtin_bit_cast(UINT64, val);
		bits = ull;
		return *this;
	}

	/// @}
	/// @name Comparison Operators
	/// @{

	BOOL operator==(const DOUBLE &other) const noexcept { return Compare(other, CMP_EQ); }
	BOOL operator!=(const DOUBLE &other) const noexcept { return !Compare(other, CMP_EQ); }
	BOOL operator<(const DOUBLE &other) const noexcept { return Compare(other, CMP_LT); }
	BOOL operator<=(const DOUBLE &other) const noexcept { return Compare(other, CMP_LE); }
	BOOL operator>(const DOUBLE &other) const noexcept { return Compare(other, CMP_GT); }
	BOOL operator>=(const DOUBLE &other) const noexcept { return Compare(other, CMP_GE); }

	/// @}
	/// @name Arithmetic Operators
	/// @{

	DOUBLE operator+(const DOUBLE &other) const noexcept { return Arithmetic(other, OP_ADD); }
	DOUBLE operator-(const DOUBLE &other) const noexcept { return Arithmetic(other, OP_SUB); }
	DOUBLE operator*(const DOUBLE &other) const noexcept { return Arithmetic(other, OP_MUL); }
	DOUBLE operator/(const DOUBLE &other) const noexcept { return Arithmetic(other, OP_DIV); }

	/** @brief Unary negation — flips the sign bit (IEEE 754 Section 5.5.1) */
	NOINLINE DISABLE_OPTIMIZATION DOUBLE operator-() const noexcept
	{
		UINT64 newBits = bits ^ GetSignMask();
		return DOUBLE(newBits);
	}

	/// @}
	/// @name Compound Assignment Operators
	/// @{

	FORCE_INLINE DOUBLE &operator+=(const DOUBLE &other) noexcept
	{
		*this = *this + other;
		return *this;
	}

	FORCE_INLINE DOUBLE &operator-=(const DOUBLE &other) noexcept
	{
		*this = *this - other;
		return *this;
	}

	FORCE_INLINE DOUBLE &operator*=(const DOUBLE &other) noexcept
	{
		*this = *this * other;
		return *this;
	}

	FORCE_INLINE DOUBLE &operator/=(const DOUBLE &other) noexcept
	{
		*this = *this / other;
		return *this;
	}

	/// @}
	/// @name Mixed-Type Operators
	/// @{

	/** @brief Compare with INT32 (converts val to DOUBLE first) */
	NOINLINE DISABLE_OPTIMIZATION BOOL operator<(INT32 val) const noexcept
	{
		return *this < DOUBLE(val);
	}

	/** @brief Subtract UINT64 (converts val to DOUBLE first) */
	NOINLINE DISABLE_OPTIMIZATION DOUBLE operator-(UINT64 val) const noexcept
	{
		DOUBLE d_val = DOUBLE((INT64)val);
		return *this - d_val;
	}

	/** @brief Subtract UINT32 (converts val to DOUBLE first) */
	NOINLINE DISABLE_OPTIMIZATION DOUBLE operator-(UINT32 val) const noexcept
	{
		DOUBLE d_val = DOUBLE((INT64)val);
		return *this - d_val;
	}

	/// @}
};

// =============================================================================
// COMPILE-TIME LITERAL OPERATORS
// =============================================================================

/**
 * @brief User-defined literal for compile-time DOUBLE embedding (long double)
 * @param v Floating-point value
 * @return DOUBLE with IEEE-754 bit pattern embedded as immediate value
 *
 * @details The consteval keyword ensures this is evaluated at compile time,
 * preventing the compiler from placing the value in .rdata.
 *
 * @par Usage:
 * @code
 * DOUBLE pi = 3.14159_embed;
 * @endcode
 */
consteval DOUBLE operator""_embed(long double v)
{
	return DOUBLE(static_cast<double>(v), DOUBLE::CompileTimeLiteral{});
}

/**
 * @brief User-defined literal for compile-time DOUBLE embedding (integer)
 * @param value Integer value to convert to DOUBLE
 * @return DOUBLE with IEEE-754 bit pattern embedded as immediate value
 */
consteval DOUBLE operator""_embed(UINT64 value)
{
	return DOUBLE(static_cast<double>(value), DOUBLE::CompileTimeLiteral{});
}

/** @brief Pointer to DOUBLE */
typedef DOUBLE *PDOUBLE;

/** @} */ // end of double_type group
