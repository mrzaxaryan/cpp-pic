/**
 * @file string.h
 * @brief String Manipulation Utilities
 *
 * @details Provides comprehensive string manipulation functions without CRT dependencies.
 * All operations are position-independent and work with both narrow (CHAR) and wide (WCHAR)
 * character types through template specialization.
 *
 * Features:
 * - Character classification (IsSpace, IsDigit, IsAlpha, IsAlphaNum)
 * - Case conversion (ToLowerCase, ToUpperCase)
 * - String comparison and searching
 * - String copying and manipulation
 * - Number to string conversion
 * - String to number parsing
 * - UTF-8 to UTF-16 conversion
 *
 * @note All functions are designed for position-independent code with no .rdata dependencies.
 *
 * @ingroup core
 *
 * @defgroup string String Utilities
 * @ingroup core
 * @{
 */

#pragma once

#include "primitives.h"
#include "span.h"
#include "double.h"
#include "embedded_string.h"
#include "error.h"
#include "result.h"

/**
 * @class String
 * @brief Static class providing string manipulation utilities
 *
 * @details Provides a comprehensive set of string operations without CRT dependencies.
 * All methods are template-based where appropriate, supporting both CHAR and WCHAR types.
 * Methods are force-inlined for performance.
 *
 * @par Example Usage:
 * @code
 * // Character classification
 * BOOL isDigit = String::IsDigit('5');           // true
 * BOOL isSpace = String::IsSpace('\t');          // true
 *
 * // String operations
 * USIZE len = String::Length("Hello");           // 5
 * BOOL eq = String::Equals("foo", "foo");        // true
 *
 * // Number conversion
 * CHAR buf[32];
 * String::IntToStr(-42, Span<CHAR>(buf));        // "-42"
 * INT64 num = String::ParseInt64("12345");       // 12345
 * @endcode
 */
class String
{
public:
	/// @name Character Classification
	/// @{

	/**
	 * @brief Check if character is whitespace
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param c Character to check
	 * @return true if whitespace (space, tab, newline, etc.), false otherwise
	 */
	template <TCHAR TChar>
	static constexpr FORCE_INLINE BOOL IsSpace(TChar c) noexcept;

	/**
	 * @brief Check if character is a decimal digit (0-9)
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param c Character to check
	 * @return true if digit, false otherwise
	 */
	template <TCHAR TChar>
	static constexpr FORCE_INLINE BOOL IsDigit(TChar c) noexcept;

	/**
	 * @brief Check if character is alphabetic (a-z, A-Z)
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param c Character to check
	 * @return true if alphabetic, false otherwise
	 */
	template <TCHAR TChar>
	static constexpr FORCE_INLINE BOOL IsAlpha(TChar c) noexcept;

	/**
	 * @brief Check if character is alphanumeric (a-z, A-Z, 0-9)
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param c Character to check
	 * @return true if alphanumeric, false otherwise
	 */
	template <TCHAR TChar>
	static constexpr FORCE_INLINE BOOL IsAlphaNum(TChar c) noexcept;

	/// @}
	/// @name Character Conversion
	/// @{

	/**
	 * @brief Convert character to lowercase
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param c Character to convert
	 * @return Lowercase character, or original if not uppercase
	 */
	template <TCHAR TChar>
	static constexpr FORCE_INLINE TChar ToLowerCase(TChar c) noexcept;

	/**
	 * @brief Convert character to uppercase
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param c Character to convert
	 * @return Uppercase character, or original if not lowercase
	 */
	template <TCHAR TChar>
	static constexpr FORCE_INLINE TChar ToUpperCase(TChar c) noexcept;

	/// @}
	/// @name String Length and Comparison
	/// @{

	/**
	 * @brief Get length of null-terminated string
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param pChar Pointer to null-terminated string
	 * @return Number of characters (excluding null terminator)
	 */
	template <TCHAR TChar>
	static constexpr USIZE Length(const TChar *pChar) noexcept;

	/**
	 * @brief Compare two null-terminated strings
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param s1 First string
	 * @param s2 Second string
	 * @param ignoreCase If true, comparison is case-insensitive
	 * @return true if strings are equal, false otherwise
	 */
	template <TCHAR TChar>
	static constexpr BOOL Compare(const TChar *s1, const TChar *s2, BOOL ignoreCase = false) noexcept;

	/**
	 * @brief Compare two strings with explicit lengths
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param a First string span
	 * @param b Second string span
	 * @return true if strings are equal, false otherwise
	 */
	template <TCHAR TChar>
	static constexpr BOOL Equals(Span<const TChar> a, Span<const TChar> b) noexcept;

	/**
	 * @brief Compare two null-terminated strings for equality
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param a First string
	 * @param b Second string
	 * @return true if strings are equal, false otherwise
	 */
	template <TCHAR TChar>
	static constexpr BOOL Equals(const TChar *a, const TChar *b) noexcept;

	/**
	 * @brief Check if string starts with a prefix (null-terminated)
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param pChar String to check
	 * @param pSubString Prefix to look for
	 * @return true if string starts with prefix, false otherwise
	 */
	template <TCHAR TChar>
	static constexpr BOOL StartsWith(const TChar *pChar, const TChar *pSubString) noexcept;

	/**
	 * @brief Check if string starts with prefix (with explicit lengths)
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param str String span to check
	 * @param prefix Prefix span to look for
	 * @return true if string starts with prefix, false otherwise
	 */
	template <TCHAR TChar>
	static constexpr BOOL StartsWith(Span<const TChar> str, Span<const TChar> prefix) noexcept;

	/**
	 * @brief Check if string ends with suffix
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param str String span to check
	 * @param suffix Suffix span to look for
	 * @return true if string ends with suffix, false otherwise
	 */
	template <TCHAR TChar>
	static constexpr BOOL EndsWith(Span<const TChar> str, Span<const TChar> suffix) noexcept;

	/// @}
	/// @name String Search
	/// @{

	/**
	 * @brief Find address of character in string
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param c Character to find
	 * @param pChar String to search
	 * @return Pointer to first occurrence, or nullptr if not found
	 */
	template <TCHAR TChar>
	static constexpr const TChar *AddressOf(TChar c, const TChar *pChar) noexcept;

	/**
	 * @brief Find index of character in string
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param str String span to search
	 * @param ch Character to find
	 * @return Index of first occurrence, or -1 if not found
	 */
	template <TCHAR TChar>
	static constexpr SSIZE IndexOfChar(Span<const TChar> str, TChar ch) noexcept;

	/**
	 * @brief Find index of substring in string
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param str String span to search
	 * @param sub Substring span to find
	 * @return Index of first occurrence, or -1 if not found
	 */
	template <TCHAR TChar>
	static constexpr SSIZE IndexOf(Span<const TChar> str, Span<const TChar> sub) noexcept;

	/// @}
	/// @name String Copy Operations
	/// @{

	/**
	 * @brief Safe string copy with explicit buffer size
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param dest Destination buffer span
	 * @param src Source string span
	 * @return Number of characters copied (excluding null terminator)
	 */
	template <TCHAR TChar>
	static constexpr USIZE Copy(Span<TChar> dest, Span<const TChar> src) noexcept;

	/**
	 * @brief Safe string copy with compile-time buffer size
	 * @tparam MaxLen Buffer size (deduced from array)
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param dest Destination array
	 * @param src Source string span
	 * @return Number of characters copied (excluding null terminator)
	 */
	template <USIZE MaxLen, TCHAR TChar>
	static constexpr FORCE_INLINE USIZE Copy(TChar (&dest)[MaxLen], Span<const TChar> src) noexcept;

	/**
	 * @brief Copy embedded string to buffer
	 * @tparam T Embedded string type
	 * @param src Source embedded string
	 * @param buffer Destination buffer
	 * @param bufSize Size of destination buffer
	 * @return Number of characters copied (excluding null terminator)
	 */
	template <typename T>
	static constexpr USIZE CopyEmbed(const T &src, Span<CHAR> buffer) noexcept;

	/// @}
	/// @name String Manipulation
	/// @{

	/**
	 * @brief Trim whitespace from end of string (in-place)
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param str String to trim (modified in place)
	 * @return New length including null terminator
	 */
	template <TCHAR TChar>
	static constexpr USIZE TrimEnd(TChar *str) noexcept;

	/**
	 * @brief Trim whitespace from end (with explicit length)
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param str String to trim
	 * @param len Length of string (updated on return)
	 */
	template <TCHAR TChar>
	static constexpr void TrimEnd(const TChar *str, USIZE &len) noexcept;

	/**
	 * @brief Trim whitespace from start
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param str String to trim
	 * @param len Length of string (updated on return)
	 * @return Pointer to first non-whitespace character
	 */
	template <TCHAR TChar>
	static constexpr const TChar *TrimStart(const TChar *str, USIZE &len) noexcept;

	/**
	 * @brief Trim whitespace from both ends
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param str String to trim
	 * @param len Length of string (updated on return)
	 * @return Pointer to first non-whitespace character
	 */
	template <TCHAR TChar>
	static constexpr const TChar *Trim(const TChar *str, USIZE &len) noexcept;

	/**
	 * @brief Concatenate two strings into a buffer
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param buffer Destination buffer span
	 * @param s1 First string span
	 * @param s2 Second string span
	 * @return Total number of characters written (excluding null terminator)
	 */
	template <TCHAR TChar>
	static constexpr USIZE Concat(Span<TChar> buffer,
						Span<const TChar> s1,
						Span<const TChar> s2) noexcept;

	/// @}
	/// @name Number Conversion
	/// @{

	/**
	 * @brief Convert signed integer to string
	 * @param value Integer value to convert
	 * @param buffer Destination buffer span
	 * @return Number of characters written (excluding null terminator)
	 */
	static USIZE IntToStr(INT64 value, Span<CHAR> buffer) noexcept;

	/**
	 * @brief Convert unsigned integer to string
	 * @param value Unsigned integer value to convert
	 * @param buffer Destination buffer span
	 * @return Number of characters written (excluding null terminator)
	 */
	static USIZE UIntToStr(UINT64 value, Span<CHAR> buffer) noexcept;

	/**
	 * @brief Parse hexadecimal string to UINT32
	 * @param str Hexadecimal string (without 0x prefix)
	 * @return Parsed value (stops at first non-hex character)
	 */
	static UINT32 ParseHex(PCCHAR str) noexcept;

	/**
	 * @brief Write decimal number to buffer
	 * @param buffer Destination buffer
	 * @param num Number to write
	 * @return Pointer to null terminator
	 */
	static PCHAR WriteDecimal(PCHAR buffer, UINT32 num) noexcept;

	/**
	 * @brief Write hexadecimal number to buffer
	 * @param buffer Destination buffer
	 * @param num Number to write
	 * @param uppercase true for A-F, false for a-f
	 * @return Pointer to null terminator
	 */
	static PCHAR WriteHex(PCHAR buffer, UINT32 num, BOOL uppercase = false) noexcept;

	/**
	 * @brief Convert DOUBLE to string
	 * @param value Floating-point value to convert
	 * @param buffer Destination buffer span
	 * @param precision Number of decimal places (default 6)
	 * @return Number of characters written (excluding null terminator)
	 */
	static USIZE FloatToStr(DOUBLE value, Span<CHAR> buffer, UINT8 precision = 6) noexcept;

	/**
	 * @brief Parse string to INT64 (with explicit length)
	 * @param str String span to parse
	 * @return Result containing parsed INT64 value, or Error::String_ParseIntFailed
	 */
	[[nodiscard]] static Result<INT64, Error> ParseInt64(Span<const CHAR> str) noexcept;

	/**
	 * @brief Parse null-terminated string to INT64
	 * @param str Null-terminated string to parse
	 * @return Parsed value (0 on failure)
	 */
	static INT64 ParseInt64(PCCHAR str) noexcept;

	/**
	 * @brief Convert string to DOUBLE
	 * @param str String span to parse
	 * @return Result containing parsed DOUBLE value, or Error::String_ParseFloatFailed
	 */
	[[nodiscard]] static Result<DOUBLE, Error> StrToFloat(Span<const CHAR> str) noexcept;

	/**
	 * @brief Parse string to specified type
	 * @tparam T Target type
	 * @param str String to parse
	 * @return Parsed value
	 */
	template <typename T>
	static T ParseString(const CHAR *str);

	/// @}
	/// @name UTF Conversion
	/// @{

	/**
	 * @brief Convert UTF-8 string to UTF-16 (wide string)
	 * @param utf8 Source UTF-8 string (null-terminated)
	 * @param wide Destination wide string buffer span
	 * @return Number of wide characters written (excluding null terminator)
	 *
	 * @details Decodes UTF-8 multibyte sequences (1-4 bytes per codepoint) and
	 * encodes them as UTF-16 code units. Codepoints above U+FFFF are encoded
	 * as surrogate pairs per RFC 2781.
	 *
	 * @see RFC 3629 — UTF-8, a transformation format of ISO 10646
	 *      https://datatracker.ietf.org/doc/html/rfc3629
	 * @see RFC 2781 — UTF-16, an encoding of ISO 10646
	 *      https://datatracker.ietf.org/doc/html/rfc2781
	 */
	static USIZE Utf8ToWide(PCCHAR utf8, Span<WCHAR> wide);

	/// @}
};

// ============================================================================
// CHARACTER CLASSIFICATION IMPLEMENTATIONS
// ============================================================================

template <TCHAR TChar>
constexpr FORCE_INLINE BOOL String::IsSpace(TChar c) noexcept
{
	return (c == (TChar)' ' ||  // space
			c == (TChar)'\t' || // horizontal tab
			c == (TChar)'\n' || // newline
			c == (TChar)'\v' || // vertical tab
			c == (TChar)'\f' || // form feed
			c == (TChar)'\r');  // carriage return
}

template <TCHAR TChar>
constexpr FORCE_INLINE BOOL String::IsDigit(TChar c) noexcept
{
	return (c >= (TChar)'0' && c <= (TChar)'9');
}

template <TCHAR TChar>
constexpr FORCE_INLINE BOOL String::IsAlpha(TChar c) noexcept
{
	return (c >= (TChar)'a' && c <= (TChar)'z') ||
		   (c >= (TChar)'A' && c <= (TChar)'Z');
}

template <TCHAR TChar>
constexpr FORCE_INLINE BOOL String::IsAlphaNum(TChar c) noexcept
{
	return IsAlpha(c) || IsDigit(c);
}

// ============================================================================
// CHARACTER CONVERSION IMPLEMENTATIONS
// ============================================================================

template <TCHAR TChar>
constexpr FORCE_INLINE TChar String::ToLowerCase(TChar c) noexcept
{
	if (c >= (TChar)'A' && c <= (TChar)'Z')
	{
		return c + ((TChar)'a' - (TChar)'A');
	}
	return c;
}

template <TCHAR TChar>
constexpr FORCE_INLINE TChar String::ToUpperCase(TChar c) noexcept
{
	if (c >= (TChar)'a' && c <= (TChar)'z')
	{
		return c - ((TChar)'a' - (TChar)'A');
	}
	return c;
}

// ============================================================================
// STRING LENGTH AND COMPARISON IMPLEMENTATIONS
// ============================================================================

template <TCHAR TChar>
constexpr USIZE String::Length(const TChar *p) noexcept
{
	if (!p)
		return 0;
	USIZE i = 0;
	while (p[i] != (TChar)'\0')
	{
		i++;
	}
	return i;
}

template <TCHAR TChar>
constexpr BOOL String::Compare(const TChar *s1, const TChar *s2, BOOL ignoreCase) noexcept
{
	const TChar *str1 = s1;
	const TChar *str2 = s2;
	while (*str1 && *str2)
	{
		TChar c1 = ignoreCase ? ToLowerCase(*str1) : *str1;
		TChar c2 = ignoreCase ? ToLowerCase(*str2) : *str2;
		if (c1 != c2)
		{
			return false;
		}
		str1++;
		str2++;
	}
	return (*str1 == *str2);
}

template <TCHAR TChar>
constexpr BOOL String::Equals(Span<const TChar> a, Span<const TChar> b) noexcept
{
	if (a.Size() != b.Size())
		return false;
	for (USIZE i = 0; i < a.Size(); i++)
	{
		if (a[i] != b[i])
			return false;
	}
	return true;
}

template <TCHAR TChar>
constexpr BOOL String::Equals(const TChar *a, const TChar *b) noexcept
{
	if (!a || !b)
		return a == b;
	while (*a != (TChar)'\0' && *b != (TChar)'\0')
	{
		if (*a != *b)
			return false;
		a++;
		b++;
	}
	return *a == *b;
}

template <TCHAR TChar>
constexpr BOOL String::StartsWith(const TChar *pChar, const TChar *pSubString) noexcept
{
	USIZE i = 0;
	while (pChar[i] != '\0' && pSubString[i] != '\0')
	{
		if (pChar[i] != pSubString[i])
		{
			return false;
		}
		i++;
	}
	if (pSubString[i] != '\0')
	{
		return false;
	}
	return true;
}

template <TCHAR TChar>
constexpr BOOL String::StartsWith(Span<const TChar> str, Span<const TChar> prefix) noexcept
{
	if (prefix.Size() > str.Size())
		return false;
	for (USIZE i = 0; i < prefix.Size(); i++)
	{
		if (str[i] != prefix[i])
			return false;
	}
	return true;
}

template <TCHAR TChar>
constexpr BOOL String::EndsWith(Span<const TChar> str, Span<const TChar> suffix) noexcept
{
	if (suffix.Size() > str.Size())
		return false;
	USIZE offset = str.Size() - suffix.Size();
	for (USIZE i = 0; i < suffix.Size(); i++)
	{
		if (str[offset + i] != suffix[i])
			return false;
	}
	return true;
}

// ============================================================================
// STRING SEARCH IMPLEMENTATIONS
// ============================================================================

template <TCHAR TChar>
constexpr const TChar *String::AddressOf(TChar c, const TChar *pChar) noexcept
{
	USIZE i = 0;
	while (pChar[i] != '\0')
	{
		if (pChar[i] == c)
		{
			return &pChar[i];
		}
		i++;
	}
	return nullptr;
}

template <TCHAR TChar>
constexpr SSIZE String::IndexOfChar(Span<const TChar> str, TChar ch) noexcept
{
	for (USIZE i = 0; i < str.Size(); i++)
	{
		if (str[i] == ch)
			return (SSIZE)i;
	}
	return -1;
}

template <TCHAR TChar>
constexpr SSIZE String::IndexOf(Span<const TChar> str, Span<const TChar> sub) noexcept
{
	if (sub.Size() == 0)
		return 0;
	if (sub.Size() > str.Size())
		return -1;

	USIZE limit = str.Size() - sub.Size() + 1;
	for (USIZE i = 0; i < limit; i++)
	{
		BOOL match = true;
		for (USIZE j = 0; j < sub.Size() && match; j++)
		{
			if (str[i + j] != sub[j])
				match = false;
		}
		if (match)
			return (SSIZE)i;
	}
	return -1;
}

// ============================================================================
// STRING COPY IMPLEMENTATIONS
// ============================================================================

template <TCHAR TChar>
constexpr USIZE String::Copy(Span<TChar> dest, Span<const TChar> src) noexcept
{
	if (!dest.Data() || dest.Size() == 0)
		return 0;
	if (!src.Data() || src.Size() == 0)
	{
		dest[0] = (TChar)'\0';
		return 0;
	}

	USIZE copyLen = src.Size() < dest.Size() - 1 ? src.Size() : dest.Size() - 1;
	for (USIZE i = 0; i < copyLen; i++)
	{
		dest[i] = src[i];
	}
	dest[copyLen] = (TChar)'\0';
	return copyLen;
}

template <USIZE MaxLen, TCHAR TChar>
constexpr FORCE_INLINE USIZE String::Copy(TChar (&dest)[MaxLen], Span<const TChar> src) noexcept
{
	return Copy(Span<TChar>(dest), src);
}

template <typename T>
constexpr USIZE String::CopyEmbed(const T &src, Span<CHAR> buffer) noexcept
{
	if (buffer.Size() == 0)
		return 0;

	USIZE len = 0;
	const CHAR *s = src;
	while (s[len] != '\0' && len < buffer.Size() - 1)
	{
		buffer.Data()[len] = s[len];
		len++;
	}
	buffer.Data()[len] = '\0';
	return len;
}

// ============================================================================
// STRING MANIPULATION IMPLEMENTATIONS
// ============================================================================

template <TCHAR TChar>
constexpr USIZE String::TrimEnd(TChar *str) noexcept
{
	if (!str)
		return 0;
	INT32 len = (INT32)String::Length(str);
	if (len == 0)
		return 1;

	USIZE lengthAfterTrim = len + 1;

	TChar *p = str + len - 1;
	while (p >= str && String::IsSpace((TChar)*p))
	{
		*p = (TChar)'\0';
		p--;
		lengthAfterTrim--;
	}
	return lengthAfterTrim;
}

template <TCHAR TChar>
constexpr void String::TrimEnd(const TChar *str, USIZE &len) noexcept
{
	if (!str)
		return;
	while (len > 0 && IsSpace(str[len - 1]))
	{
		len--;
	}
}

template <TCHAR TChar>
constexpr const TChar *String::TrimStart(const TChar *str, USIZE &len) noexcept
{
	if (!str)
		return str;
	while (len > 0 && IsSpace(*str))
	{
		str++;
		len--;
	}
	return str;
}

template <TCHAR TChar>
constexpr const TChar *String::Trim(const TChar *str, USIZE &len) noexcept
{
	str = TrimStart(str, len);
	TrimEnd(str, len);
	return str;
}

template <TCHAR TChar>
constexpr USIZE String::Concat(Span<TChar> buffer,
					 Span<const TChar> s1,
					 Span<const TChar> s2) noexcept
{
	if (!buffer.Data() || buffer.Size() == 0)
		return 0;

	USIZE pos = 0;

	for (USIZE i = 0; i < s1.Size() && pos < buffer.Size() - 1; i++)
	{
		buffer[pos++] = s1[i];
	}

	for (USIZE i = 0; i < s2.Size() && pos < buffer.Size() - 1; i++)
	{
		buffer[pos++] = s2[i];
	}

	buffer[pos] = (TChar)'\0';
	return pos;
}

/** @} */ // end of string group
