/**
 * strutil.h - String Utilities for PIL (Position Independent Language)
 *
 * Provides common string operations to eliminate code duplication.
 * Position-independent, no .rdata dependencies.
 *
 * Part of RAL (Runtime Abstraction Layer).
 */

#pragma once

#include "bal/types/primitives.h"
#include "bal/types/numeric/double.h"
#include "bal/types/embedded/embedded_string.h"

namespace script
{
namespace StrUtil
{

// ============================================================================
// STRING COPY OPERATIONS
// ============================================================================

/**
 * Safe string copy with explicit buffer size.
 * Copies at most (destSize - 1) characters and always null-terminates.
 * @param dest      Destination buffer
 * @param destSize  Size of destination buffer (including null terminator)
 * @param src       Source string
 * @param srcLen    Length of source string (not including null terminator)
 * @return          Number of characters copied (not including null terminator)
 */
FORCE_INLINE USIZE Copy(CHAR* dest, USIZE destSize, const CHAR* src, USIZE srcLen) noexcept
{
    if (!dest || destSize == 0) return 0;
    if (!src || srcLen == 0)
    {
        dest[0] = '\0';
        return 0;
    }

    USIZE copyLen = srcLen < destSize - 1 ? srcLen : destSize - 1;
    for (USIZE i = 0; i < copyLen; i++)
    {
        dest[i] = src[i];
    }
    dest[copyLen] = '\0';
    return copyLen;
}

/**
 * Safe string copy with compile-time buffer size.
 * @param dest      Destination buffer (array reference for compile-time size)
 * @param src       Source string
 * @param srcLen    Length of source string
 * @return          Number of characters copied
 */
template<USIZE MaxLen>
FORCE_INLINE USIZE Copy(CHAR (&dest)[MaxLen], const CHAR* src, USIZE srcLen) noexcept
{
    return Copy(dest, MaxLen, src, srcLen);
}

/**
 * Copy embedded string to buffer.
 * For use with _embed strings.
 * @param src       Embedded string source
 * @param buffer    Destination buffer
 * @param bufSize   Size of destination buffer
 * @return          Number of characters copied
 */
template<typename T>
FORCE_INLINE USIZE CopyEmbed(const T& src, CHAR* buffer, USIZE bufSize) noexcept
{
    if (!buffer || bufSize == 0) return 0;

    USIZE len = 0;
    const CHAR* s = src;
    while (s[len] != '\0' && len < bufSize - 1)
    {
        buffer[len] = s[len];
        len++;
    }
    buffer[len] = '\0';
    return len;
}

// ============================================================================
// STRING LENGTH AND COMPARISON
// ============================================================================

/**
 * Calculate length of null-terminated string.
 * @param str       Null-terminated string
 * @return          Length (not including null terminator)
 */
FORCE_INLINE USIZE Length(const CHAR* str) noexcept
{
    if (!str) return 0;
    USIZE len = 0;
    while (str[len] != '\0') len++;
    return len;
}

/**
 * Compare two strings for equality.
 * @param a         First string
 * @param aLen      Length of first string
 * @param b         Second string
 * @param bLen      Length of second string
 * @return          TRUE if equal, FALSE otherwise
 */
FORCE_INLINE BOOL Equals(const CHAR* a, USIZE aLen, const CHAR* b, USIZE bLen) noexcept
{
    if (aLen != bLen) return FALSE;
    for (USIZE i = 0; i < aLen; i++)
    {
        if (a[i] != b[i]) return FALSE;
    }
    return TRUE;
}

/**
 * Compare two null-terminated strings for equality.
 * @param a         First string (null-terminated)
 * @param b         Second string (null-terminated)
 * @return          TRUE if equal, FALSE otherwise
 */
FORCE_INLINE BOOL Equals(const CHAR* a, const CHAR* b) noexcept
{
    if (!a || !b) return a == b;
    while (*a != '\0' && *b != '\0')
    {
        if (*a != *b) return FALSE;
        a++;
        b++;
    }
    return *a == *b;
}

/**
 * Check if string starts with prefix.
 * @param str       String to check
 * @param strLen    Length of string
 * @param prefix    Prefix to look for
 * @param prefixLen Length of prefix
 * @return          TRUE if str starts with prefix
 */
FORCE_INLINE BOOL StartsWith(const CHAR* str, USIZE strLen, const CHAR* prefix, USIZE prefixLen) noexcept
{
    if (prefixLen > strLen) return FALSE;
    for (USIZE i = 0; i < prefixLen; i++)
    {
        if (str[i] != prefix[i]) return FALSE;
    }
    return TRUE;
}

/**
 * Check if string ends with suffix.
 * @param str       String to check
 * @param strLen    Length of string
 * @param suffix    Suffix to look for
 * @param suffixLen Length of suffix
 * @return          TRUE if str ends with suffix
 */
FORCE_INLINE BOOL EndsWith(const CHAR* str, USIZE strLen, const CHAR* suffix, USIZE suffixLen) noexcept
{
    if (suffixLen > strLen) return FALSE;
    USIZE offset = strLen - suffixLen;
    for (USIZE i = 0; i < suffixLen; i++)
    {
        if (str[offset + i] != suffix[i]) return FALSE;
    }
    return TRUE;
}

/**
 * Find first occurrence of substring.
 * @param str       String to search in
 * @param strLen    Length of string
 * @param sub       Substring to find
 * @param subLen    Length of substring
 * @return          Index of first occurrence, or -1 if not found
 */
FORCE_INLINE SSIZE IndexOf(const CHAR* str, USIZE strLen, const CHAR* sub, USIZE subLen) noexcept
{
    if (subLen == 0) return 0;
    if (subLen > strLen) return -1;

    USIZE limit = strLen - subLen + 1;
    for (USIZE i = 0; i < limit; i++)
    {
        BOOL match = TRUE;
        for (USIZE j = 0; j < subLen && match; j++)
        {
            if (str[i + j] != sub[j]) match = FALSE;
        }
        if (match) return (SSIZE)i;
    }
    return -1;
}

/**
 * Find first occurrence of character.
 * @param str       String to search in
 * @param strLen    Length of string
 * @param ch        Character to find
 * @return          Index of first occurrence, or -1 if not found
 */
FORCE_INLINE SSIZE IndexOfChar(const CHAR* str, USIZE strLen, CHAR ch) noexcept
{
    for (USIZE i = 0; i < strLen; i++)
    {
        if (str[i] == ch) return (SSIZE)i;
    }
    return -1;
}

// ============================================================================
// STRING CONVERSION
// ============================================================================

/**
 * Convert integer to string.
 * @param value     Integer value to convert
 * @param buffer    Destination buffer
 * @param bufSize   Size of destination buffer
 * @return          Number of characters written
 */
FORCE_INLINE USIZE IntToStr(INT64 value, CHAR* buffer, USIZE bufSize) noexcept
{
    if (!buffer || bufSize < 2) return 0;

    CHAR temp[24];  // Max INT64 is 20 digits + sign + null
    USIZE pos = 0;
    BOOL negative = FALSE;

    if (value < 0)
    {
        negative = TRUE;
        value = -value;
    }

    // Special case for zero
    if (value == 0)
    {
        temp[pos++] = '0';
    }
    else
    {
        // Build string in reverse
        while (value > 0 && pos < 22)
        {
            temp[pos++] = '0' + (CHAR)(value % 10);
            value = value / 10;
        }
    }

    if (negative && pos < 22)
    {
        temp[pos++] = '-';
    }

    // Reverse and copy to buffer
    USIZE copyLen = pos < bufSize - 1 ? pos : bufSize - 1;
    for (USIZE i = 0; i < copyLen; i++)
    {
        buffer[i] = temp[pos - 1 - i];
    }
    buffer[copyLen] = '\0';
    return copyLen;
}

/**
 * Convert unsigned integer to string.
 * @param value     Unsigned integer value to convert
 * @param buffer    Destination buffer
 * @param bufSize   Size of destination buffer
 * @return          Number of characters written
 */
FORCE_INLINE USIZE UIntToStr(UINT64 value, CHAR* buffer, USIZE bufSize) noexcept
{
    if (!buffer || bufSize < 2) return 0;

    CHAR temp[24];  // Max UINT64 is 20 digits + null
    USIZE pos = 0;

    // Special case for zero
    if (value == 0)
    {
        temp[pos++] = '0';
    }
    else
    {
        // Build string in reverse
        while (value > 0 && pos < 22)
        {
            temp[pos++] = '0' + (CHAR)(value % 10);
            value = value / 10;
        }
    }

    // Reverse and copy to buffer
    USIZE copyLen = pos < bufSize - 1 ? pos : bufSize - 1;
    for (USIZE i = 0; i < copyLen; i++)
    {
        buffer[i] = temp[pos - 1 - i];
    }
    buffer[copyLen] = '\0';
    return copyLen;
}

/**
 * Convert DOUBLE to string with configurable precision.
 * @param value     DOUBLE value to convert
 * @param buffer    Destination buffer
 * @param bufSize   Size of destination buffer
 * @param precision Number of decimal places (0-15, default 6)
 * @return          Number of characters written
 */
FORCE_INLINE USIZE FloatToStr(DOUBLE value, CHAR* buffer, USIZE bufSize, UINT8 precision = 6) noexcept
{
    if (!buffer || bufSize < 2) return 0;
    if (precision > 15) precision = 15;

    USIZE pos = 0;
    DOUBLE zero = DOUBLE(INT32(0));

    // Handle negative
    if (value < zero)
    {
        if (pos < bufSize - 1) buffer[pos++] = '-';
        value = -value;  // Use unary minus operator
    }

    // Get integer part
    INT64 intPart = (INT64)value;
    DOUBLE fracPart = value - DOUBLE(intPart);

    // Convert integer part
    CHAR intBuf[24];
    USIZE intLen = IntToStr(intPart < 0 ? -intPart : intPart, intBuf, sizeof(intBuf));
    for (USIZE i = 0; i < intLen && pos < bufSize - 1; i++)
    {
        buffer[pos++] = intBuf[i];
    }

    // Add decimal point and fractional part if precision > 0
    if (precision > 0 && pos < bufSize - 1)
    {
        buffer[pos++] = '.';

        DOUBLE ten = DOUBLE(INT32(10));

        // Multiply fractional part by 10^precision
        for (UINT8 p = 0; p < precision; p++)
        {
            fracPart = fracPart * ten;
        }

        // Round by adding 0.5
        DOUBLE half = DOUBLE(INT32(5)) / ten;  // 0.5
        fracPart = fracPart + half;
        UINT64 fracInt = (UINT64)(INT64)fracPart;

        // Pad with leading zeros and write digits
        CHAR fracBuf[24];
        USIZE fracLen = UIntToStr(fracInt, fracBuf, sizeof(fracBuf));

        // Add leading zeros if needed
        USIZE leadingZeros = fracLen < precision ? precision - fracLen : 0;
        for (USIZE i = 0; i < leadingZeros && pos < bufSize - 1; i++)
        {
            buffer[pos++] = '0';
        }

        // Write fractional digits
        for (USIZE i = 0; i < fracLen && pos < bufSize - 1; i++)
        {
            buffer[pos++] = fracBuf[i];
        }

        // Trim trailing zeros (but keep at least one decimal digit)
        while (pos > 2 && buffer[pos - 1] == '0' && buffer[pos - 2] != '.')
        {
            pos--;
        }
    }

    buffer[pos] = '\0';
    return pos;
}

/**
 * Convert string to integer.
 * @param str       String to convert
 * @param len       Length of string
 * @param result    Output for result
 * @return          TRUE if conversion successful, FALSE otherwise
 */
FORCE_INLINE BOOL StrToInt(const CHAR* str, USIZE len, INT64& result) noexcept
{
    if (!str || len == 0)
    {
        result = 0;
        return FALSE;
    }

    USIZE i = 0;
    BOOL negative = FALSE;

    // Skip whitespace
    while (i < len && (str[i] == ' ' || str[i] == '\t'))
    {
        i++;
    }

    // Handle sign
    if (i < len && str[i] == '-')
    {
        negative = TRUE;
        i++;
    }
    else if (i < len && str[i] == '+')
    {
        i++;
    }

    // Parse digits
    INT64 value = 0;
    BOOL hasDigits = FALSE;
    while (i < len && str[i] >= '0' && str[i] <= '9')
    {
        value = value * 10 + (str[i] - '0');
        hasDigits = TRUE;
        i++;
    }

    if (!hasDigits)
    {
        result = 0;
        return FALSE;
    }

    result = negative ? -value : value;
    return TRUE;
}

/**
 * Convert string to DOUBLE.
 * @param str       String to convert
 * @param len       Length of string
 * @param result    Output for result
 * @return          TRUE if conversion successful, FALSE otherwise
 */
FORCE_INLINE BOOL StrToFloat(const CHAR* str, USIZE len, DOUBLE& result) noexcept
{
    if (!str || len == 0)
    {
        result = DOUBLE(INT32(0));
        return FALSE;
    }

    // Copy to null-terminated buffer for DOUBLE::Parse
    CHAR buffer[64];
    USIZE copyLen = len < 63 ? len : 63;
    for (USIZE i = 0; i < copyLen; i++)
    {
        buffer[i] = str[i];
    }
    buffer[copyLen] = '\0';

    result = DOUBLE::Parse(buffer);
    return TRUE;
}

// ============================================================================
// STRING MANIPULATION
// ============================================================================

/**
 * Convert character to uppercase.
 * @param ch        Character to convert
 * @return          Uppercase character
 */
FORCE_INLINE CHAR ToUpper(CHAR ch) noexcept
{
    if (ch >= 'a' && ch <= 'z') return ch - 32;
    return ch;
}

/**
 * Convert character to lowercase.
 * @param ch        Character to convert
 * @return          Lowercase character
 */
FORCE_INLINE CHAR ToLower(CHAR ch) noexcept
{
    if (ch >= 'A' && ch <= 'Z') return ch + 32;
    return ch;
}

/**
 * Check if character is whitespace.
 * @param ch        Character to check
 * @return          TRUE if whitespace
 */
FORCE_INLINE BOOL IsWhitespace(CHAR ch) noexcept
{
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

/**
 * Check if character is a digit.
 * @param ch        Character to check
 * @return          TRUE if digit
 */
FORCE_INLINE BOOL IsDigit(CHAR ch) noexcept
{
    return ch >= '0' && ch <= '9';
}

/**
 * Check if character is alphabetic.
 * @param ch        Character to check
 * @return          TRUE if alphabetic
 */
FORCE_INLINE BOOL IsAlpha(CHAR ch) noexcept
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

/**
 * Check if character is alphanumeric.
 * @param ch        Character to check
 * @return          TRUE if alphanumeric
 */
FORCE_INLINE BOOL IsAlphaNum(CHAR ch) noexcept
{
    return IsAlpha(ch) || IsDigit(ch);
}

/**
 * Trim whitespace from start of string (returns new start pointer).
 * @param str       String to trim
 * @param len       Length of string (will be updated)
 * @return          Pointer to first non-whitespace character
 */
FORCE_INLINE const CHAR* TrimStart(const CHAR* str, USIZE& len) noexcept
{
    if (!str) return str;
    while (len > 0 && IsWhitespace(*str))
    {
        str++;
        len--;
    }
    return str;
}

/**
 * Trim whitespace from end of string (modifies length).
 * @param str       String to trim
 * @param len       Length of string (will be updated)
 */
FORCE_INLINE void TrimEnd(const CHAR* str, USIZE& len) noexcept
{
    if (!str) return;
    while (len > 0 && IsWhitespace(str[len - 1]))
    {
        len--;
    }
}

/**
 * Trim whitespace from both ends.
 * @param str       String to trim
 * @param len       Length of string (will be updated)
 * @return          Pointer to first non-whitespace character
 */
FORCE_INLINE const CHAR* Trim(const CHAR* str, USIZE& len) noexcept
{
    str = TrimStart(str, len);
    TrimEnd(str, len);
    return str;
}

/**
 * Concatenate two strings into a buffer.
 * @param buffer    Destination buffer
 * @param bufSize   Size of destination buffer
 * @param s1        First string
 * @param len1      Length of first string
 * @param s2        Second string
 * @param len2      Length of second string
 * @return          Total length of concatenated string
 */
FORCE_INLINE USIZE Concat(CHAR* buffer, USIZE bufSize,
                      const CHAR* s1, USIZE len1,
                      const CHAR* s2, USIZE len2) noexcept
{
    if (!buffer || bufSize == 0) return 0;

    USIZE pos = 0;

    // Copy first string
    for (USIZE i = 0; i < len1 && pos < bufSize - 1; i++)
    {
        buffer[pos++] = s1[i];
    }

    // Copy second string
    for (USIZE i = 0; i < len2 && pos < bufSize - 1; i++)
    {
        buffer[pos++] = s2[i];
    }

    buffer[pos] = '\0';
    return pos;
}

} // namespace StrUtil
} // namespace script
