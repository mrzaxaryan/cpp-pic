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
#include "double.h"
#include "embedded_string.h"

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
 * BOOL isDigit = String::IsDigit('5');           // TRUE
 * BOOL isSpace = String::IsSpace('\t');          // TRUE
 *
 * // String operations
 * USIZE len = String::Length("Hello");           // 5
 * BOOL eq = String::Equals("foo", "foo");        // TRUE
 *
 * // Number conversion
 * CHAR buf[32];
 * String::IntToStr(-42, buf, sizeof(buf));       // "-42"
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
     * @return TRUE if whitespace (space, tab, newline, etc.), FALSE otherwise
     */
    template <TCHAR TChar>
    static FORCE_INLINE BOOL IsSpace(TChar c) noexcept;

    /**
     * @brief Check if character is a decimal digit (0-9)
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param c Character to check
     * @return TRUE if digit, FALSE otherwise
     */
    template <TCHAR TChar>
    static FORCE_INLINE BOOL IsDigit(TChar c) noexcept;

    /**
     * @brief Check if character is alphabetic (a-z, A-Z)
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param c Character to check
     * @return TRUE if alphabetic, FALSE otherwise
     */
    template <TCHAR TChar>
    static FORCE_INLINE BOOL IsAlpha(TChar c) noexcept;

    /**
     * @brief Check if character is alphanumeric (a-z, A-Z, 0-9)
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param c Character to check
     * @return TRUE if alphanumeric, FALSE otherwise
     */
    template <TCHAR TChar>
    static FORCE_INLINE BOOL IsAlphaNum(TChar c) noexcept;

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
    static constexpr FORCE_INLINE USIZE Length(const TChar *pChar) noexcept;

    /**
     * @brief Compare two null-terminated strings
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param s1 First string
     * @param s2 Second string
     * @param ignoreCase If TRUE, comparison is case-insensitive
     * @return TRUE if strings are equal, FALSE otherwise
     */
    template <TCHAR TChar>
    static inline BOOL Compare(const TChar *s1, const TChar *s2, BOOL ignoreCase = FALSE) noexcept;

    /**
     * @brief Compare two strings with explicit lengths
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param a First string
     * @param aLen Length of first string
     * @param b Second string
     * @param bLen Length of second string
     * @return TRUE if strings are equal, FALSE otherwise
     */
    template <TCHAR TChar>
    static inline BOOL Equals(const TChar *a, USIZE aLen, const TChar *b, USIZE bLen) noexcept;

    /**
     * @brief Compare two null-terminated strings for equality
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param a First string
     * @param b Second string
     * @return TRUE if strings are equal, FALSE otherwise
     */
    template <TCHAR TChar>
    static FORCE_INLINE BOOL Equals(const TChar *a, const TChar *b) noexcept;

    /**
     * @brief Check if string starts with a prefix (null-terminated)
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param pChar String to check
     * @param pSubString Prefix to look for
     * @return TRUE if string starts with prefix, FALSE otherwise
     */
    template <TCHAR TChar>
    static inline BOOL StartsWith(const TChar *pChar, const TChar *pSubString) noexcept;

    /**
     * @brief Check if string starts with prefix (with explicit lengths)
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param str String to check
     * @param strLen Length of string
     * @param prefix Prefix to look for
     * @param prefixLen Length of prefix
     * @return TRUE if string starts with prefix, FALSE otherwise
     */
    template <TCHAR TChar>
    static FORCE_INLINE BOOL StartsWith(const TChar *str, USIZE strLen, const TChar *prefix, USIZE prefixLen) noexcept;

    /**
     * @brief Check if string ends with suffix
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param str String to check
     * @param strLen Length of string
     * @param suffix Suffix to look for
     * @param suffixLen Length of suffix
     * @return TRUE if string ends with suffix, FALSE otherwise
     */
    template <TCHAR TChar>
    static FORCE_INLINE BOOL EndsWith(const TChar *str, USIZE strLen, const TChar *suffix, USIZE suffixLen) noexcept;

    /// @}
    /// @name String Search
    /// @{

    /**
     * @brief Find address of character in string
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param c Character to find
     * @param pChar String to search
     * @return Pointer to first occurrence, or NULL if not found
     */
    template <TCHAR TChar>
    static inline const TChar *AddressOf(TChar c, const TChar *pChar) noexcept;

    /**
     * @brief Find index of character in string
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param str String to search
     * @param strLen Length of string
     * @param ch Character to find
     * @return Index of first occurrence, or -1 if not found
     */
    template <TCHAR TChar>
    static FORCE_INLINE SSIZE IndexOfChar(const TChar *str, USIZE strLen, TChar ch) noexcept;

    /**
     * @brief Find index of substring in string
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param str String to search
     * @param strLen Length of string
     * @param sub Substring to find
     * @param subLen Length of substring
     * @return Index of first occurrence, or -1 if not found
     */
    template <TCHAR TChar>
    static inline SSIZE IndexOf(const TChar *str, USIZE strLen, const TChar *sub, USIZE subLen) noexcept;

    /// @}
    /// @name String Copy Operations
    /// @{

    /**
     * @brief Copy string (no bounds checking)
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param dest Destination buffer
     * @param src Source string
     * @return Pointer to destination buffer
     * @warning No bounds checking - use safe version with buffer size
     */
    template <TCHAR TChar>
    static FORCE_INLINE TChar *Copy(TChar *dest, const TChar *src) noexcept;

    /**
     * @brief Safe string copy with explicit buffer size
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param dest Destination buffer
     * @param destSize Size of destination buffer
     * @param src Source string
     * @param srcLen Length of source string
     * @return Number of characters copied (excluding null terminator)
     */
    template <TCHAR TChar>
    static inline USIZE Copy(TChar *dest, USIZE destSize, const TChar *src, USIZE srcLen) noexcept;

    /**
     * @brief Safe string copy with compile-time buffer size
     * @tparam MaxLen Buffer size (deduced from array)
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param dest Destination array
     * @param src Source string
     * @param srcLen Length of source string
     * @return Number of characters copied (excluding null terminator)
     */
    template <USIZE MaxLen, TCHAR TChar>
    static FORCE_INLINE USIZE Copy(TChar (&dest)[MaxLen], const TChar *src, USIZE srcLen) noexcept;

    /**
     * @brief Copy embedded string to buffer
     * @tparam T Embedded string type
     * @param src Source embedded string
     * @param buffer Destination buffer
     * @param bufSize Size of destination buffer
     * @return Number of characters copied (excluding null terminator)
     */
    template <typename T>
    static inline USIZE CopyEmbed(const T &src, CHAR *buffer, USIZE bufSize) noexcept;

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
    static inline USIZE TrimEnd(TChar *str) noexcept;

    /**
     * @brief Trim whitespace from end (with explicit length)
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param str String to trim
     * @param len Length of string (updated on return)
     */
    template <TCHAR TChar>
    static FORCE_INLINE void TrimEnd(const TChar *str, USIZE &len) noexcept;

    /**
     * @brief Trim whitespace from start
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param str String to trim
     * @param len Length of string (updated on return)
     * @return Pointer to first non-whitespace character
     */
    template <TCHAR TChar>
    static FORCE_INLINE const TChar *TrimStart(const TChar *str, USIZE &len) noexcept;

    /**
     * @brief Trim whitespace from both ends
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param str String to trim
     * @param len Length of string (updated on return)
     * @return Pointer to first non-whitespace character
     */
    template <TCHAR TChar>
    static FORCE_INLINE const TChar *Trim(const TChar *str, USIZE &len) noexcept;

    /**
     * @brief Concatenate two strings into a buffer
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param buffer Destination buffer
     * @param bufSize Size of destination buffer
     * @param s1 First string
     * @param len1 Length of first string
     * @param s2 Second string
     * @param len2 Length of second string
     * @return Total number of characters written (excluding null terminator)
     */
    template <TCHAR TChar>
    static inline USIZE Concat(TChar *buffer, USIZE bufSize,
                               const TChar *s1, USIZE len1,
                               const TChar *s2, USIZE len2) noexcept;

    /// @}
    /// @name Number Conversion
    /// @{

    /**
     * @brief Convert signed integer to string
     * @param value Integer value to convert
     * @param buffer Destination buffer
     * @param bufSize Size of destination buffer
     * @return Number of characters written (excluding null terminator)
     */
    static inline USIZE IntToStr(INT64 value, CHAR *buffer, USIZE bufSize) noexcept;

    /**
     * @brief Convert unsigned integer to string
     * @param value Unsigned integer value to convert
     * @param buffer Destination buffer
     * @param bufSize Size of destination buffer
     * @return Number of characters written (excluding null terminator)
     */
    static inline USIZE UIntToStr(UINT64 value, CHAR *buffer, USIZE bufSize) noexcept;

    /**
     * @brief Parse hexadecimal string to UINT32
     * @param str Hexadecimal string (without 0x prefix)
     * @return Parsed value (stops at first non-hex character)
     */
    static inline UINT32 ParseHex(PCCHAR str) noexcept;

    /**
     * @brief Write decimal number to buffer
     * @param buffer Destination buffer
     * @param num Number to write
     * @return Pointer to null terminator
     */
    static FORCE_INLINE PCHAR WriteDecimal(PCHAR buffer, UINT32 num) noexcept;

    /**
     * @brief Write hexadecimal number to buffer
     * @param buffer Destination buffer
     * @param num Number to write
     * @param uppercase TRUE for A-F, FALSE for a-f
     * @return Pointer to null terminator
     */
    static inline PCHAR WriteHex(PCHAR buffer, UINT32 num, BOOL uppercase = FALSE) noexcept;

    /**
     * @brief Convert DOUBLE to string
     * @param value Floating-point value to convert
     * @param buffer Destination buffer
     * @param bufSize Size of destination buffer
     * @param precision Number of decimal places (default 6)
     * @return Number of characters written (excluding null terminator)
     */
    static inline USIZE FloatToStr(DOUBLE value, CHAR *buffer, USIZE bufSize, UINT8 precision = 6) noexcept;

    /**
     * @brief Parse string to INT64 (with explicit length)
     * @param str String to parse
     * @param len Length of string
     * @param result Output parameter for parsed value
     * @return TRUE on success, FALSE on failure
     */
    static inline BOOL ParseInt64(const CHAR *str, USIZE len, INT64 &result) noexcept;

    /**
     * @brief Parse null-terminated string to INT64
     * @param str Null-terminated string to parse
     * @return Parsed value (0 on failure)
     */
    static FORCE_INLINE INT64 ParseInt64(PCCHAR str) noexcept;

    /**
     * @brief Convert string to DOUBLE
     * @param str String to parse
     * @param len Length of string
     * @param result Output parameter for parsed value
     * @return TRUE on success, FALSE on failure
     */
    static inline BOOL StrToFloat(const CHAR *str, USIZE len, DOUBLE &result) noexcept;

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
     * @param utf8 Source UTF-8 string
     * @param wide Destination wide string buffer
     * @param wideBufferSize Size of destination buffer in characters
     * @return Number of wide characters written
     */
    static USIZE Utf8ToWide(PCCHAR utf8, PWCHAR wide, USIZE wideBufferSize);

    /// @}
};

// ============================================================================
// CHARACTER CLASSIFICATION IMPLEMENTATIONS
// ============================================================================

template <TCHAR TChar>
FORCE_INLINE BOOL String::IsSpace(TChar c) noexcept
{
    return (c == (TChar)' ' ||  // space
            c == (TChar)'\t' || // horizontal tab
            c == (TChar)'\n' || // newline
            c == (TChar)'\v' || // vertical tab
            c == (TChar)'\f' || // form feed
            c == (TChar)'\r');  // carriage return
}

template <TCHAR TChar>
FORCE_INLINE BOOL String::IsDigit(TChar c) noexcept
{
    return (c >= (TChar)'0' && c <= (TChar)'9');
}

template <TCHAR TChar>
FORCE_INLINE BOOL String::IsAlpha(TChar c) noexcept
{
    return (c >= (TChar)'a' && c <= (TChar)'z') ||
           (c >= (TChar)'A' && c <= (TChar)'Z');
}

template <TCHAR TChar>
FORCE_INLINE BOOL String::IsAlphaNum(TChar c) noexcept
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
constexpr FORCE_INLINE USIZE String::Length(const TChar *p) noexcept
{
    if (!p) return 0;
    USIZE i = 0;
    while (p[i] != (TChar)'\0')
    {
        i++;
    }
    return i;
}

template <TCHAR TChar>
inline BOOL String::Compare(const TChar *s1, const TChar *s2, BOOL ignoreCase) noexcept
{
    const TChar *str1 = s1;
    const TChar *str2 = s2;
    while (*str1 && *str2)
    {
        TChar c1 = ignoreCase ? ToLowerCase(*str1) : *str1;
        TChar c2 = ignoreCase ? ToLowerCase(*str2) : *str2;
        if (c1 != c2)
        {
            return FALSE;
        }
        str1++;
        str2++;
    }
    return (*str1 == *str2);
}

template <TCHAR TChar>
inline BOOL String::Equals(const TChar *a, USIZE aLen, const TChar *b, USIZE bLen) noexcept
{
    if (aLen != bLen) return FALSE;
    for (USIZE i = 0; i < aLen; i++)
    {
        if (a[i] != b[i]) return FALSE;
    }
    return TRUE;
}

template <TCHAR TChar>
FORCE_INLINE BOOL String::Equals(const TChar *a, const TChar *b) noexcept
{
    if (!a || !b) return a == b;
    while (*a != (TChar)'\0' && *b != (TChar)'\0')
    {
        if (*a != *b) return FALSE;
        a++;
        b++;
    }
    return *a == *b;
}

template <TCHAR TChar>
inline BOOL String::StartsWith(const TChar *pChar, const TChar *pSubString) noexcept
{
    USIZE i = 0;
    while (pChar[i] != '\0' && pSubString[i] != '\0')
    {
        if (pChar[i] != pSubString[i])
        {
            return FALSE;
        }
        i++;
    }
    if (pSubString[i] != '\0')
    {
        return FALSE;
    }
    return TRUE;
}

template <TCHAR TChar>
FORCE_INLINE BOOL String::StartsWith(const TChar *str, USIZE strLen, const TChar *prefix, USIZE prefixLen) noexcept
{
    if (prefixLen > strLen) return FALSE;
    for (USIZE i = 0; i < prefixLen; i++)
    {
        if (str[i] != prefix[i]) return FALSE;
    }
    return TRUE;
}

template <TCHAR TChar>
FORCE_INLINE BOOL String::EndsWith(const TChar *str, USIZE strLen, const TChar *suffix, USIZE suffixLen) noexcept
{
    if (suffixLen > strLen) return FALSE;
    USIZE offset = strLen - suffixLen;
    for (USIZE i = 0; i < suffixLen; i++)
    {
        if (str[offset + i] != suffix[i]) return FALSE;
    }
    return TRUE;
}

// ============================================================================
// STRING SEARCH IMPLEMENTATIONS
// ============================================================================

template <TCHAR TChar>
inline const TChar *String::AddressOf(TChar c, const TChar *pChar) noexcept
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
    return NULL;
}

template <TCHAR TChar>
FORCE_INLINE SSIZE String::IndexOfChar(const TChar *str, USIZE strLen, TChar ch) noexcept
{
    for (USIZE i = 0; i < strLen; i++)
    {
        if (str[i] == ch) return (SSIZE)i;
    }
    return -1;
}

template <TCHAR TChar>
inline SSIZE String::IndexOf(const TChar *str, USIZE strLen, const TChar *sub, USIZE subLen) noexcept
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

// ============================================================================
// STRING COPY IMPLEMENTATIONS
// ============================================================================

template <TCHAR TChar>
FORCE_INLINE TChar *String::Copy(TChar *dest, const TChar *src) noexcept
{
    SSIZE i = 0;
    while (src[i] != (TChar)'\0')
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = (TChar)'\0';
    return dest;
}

template <TCHAR TChar>
inline USIZE String::Copy(TChar *dest, USIZE destSize, const TChar *src, USIZE srcLen) noexcept
{
    if (!dest || destSize == 0) return 0;
    if (!src || srcLen == 0)
    {
        dest[0] = (TChar)'\0';
        return 0;
    }

    USIZE copyLen = srcLen < destSize - 1 ? srcLen : destSize - 1;
    for (USIZE i = 0; i < copyLen; i++)
    {
        dest[i] = src[i];
    }
    dest[copyLen] = (TChar)'\0';
    return copyLen;
}

template <USIZE MaxLen, TCHAR TChar>
FORCE_INLINE USIZE String::Copy(TChar (&dest)[MaxLen], const TChar *src, USIZE srcLen) noexcept
{
    return Copy(dest, MaxLen, src, srcLen);
}

template <typename T>
inline USIZE String::CopyEmbed(const T &src, CHAR *buffer, USIZE bufSize) noexcept
{
    if (!buffer || bufSize == 0) return 0;

    USIZE len = 0;
    const CHAR *s = src;
    while (s[len] != '\0' && len < bufSize - 1)
    {
        buffer[len] = s[len];
        len++;
    }
    buffer[len] = '\0';
    return len;
}

// ============================================================================
// STRING MANIPULATION IMPLEMENTATIONS
// ============================================================================

template <TCHAR TChar>
inline USIZE String::TrimEnd(TChar *str) noexcept
{
    if (!str)
        return 0;
    INT32 len = (INT32)String::Length(str);
    if (len == 0)
        return 1;

    USIZE lenghtAfterTrim = len + 1;

    TChar *p = str + len - 1;
    while (p >= str && String::IsSpace((TChar)*p))
    {
        *p = (TChar)'\0';
        p--;
        lenghtAfterTrim--;
    }
    return lenghtAfterTrim;
}

template <TCHAR TChar>
FORCE_INLINE void String::TrimEnd(const TChar *str, USIZE &len) noexcept
{
    if (!str) return;
    while (len > 0 && IsSpace(str[len - 1]))
    {
        len--;
    }
}

template <TCHAR TChar>
FORCE_INLINE const TChar *String::TrimStart(const TChar *str, USIZE &len) noexcept
{
    if (!str) return str;
    while (len > 0 && IsSpace(*str))
    {
        str++;
        len--;
    }
    return str;
}

template <TCHAR TChar>
FORCE_INLINE const TChar *String::Trim(const TChar *str, USIZE &len) noexcept
{
    str = TrimStart(str, len);
    TrimEnd(str, len);
    return str;
}

template <TCHAR TChar>
inline USIZE String::Concat(TChar *buffer, USIZE bufSize,
                            const TChar *s1, USIZE len1,
                            const TChar *s2, USIZE len2) noexcept
{
    if (!buffer || bufSize == 0) return 0;

    USIZE pos = 0;

    for (USIZE i = 0; i < len1 && pos < bufSize - 1; i++)
    {
        buffer[pos++] = s1[i];
    }

    for (USIZE i = 0; i < len2 && pos < bufSize - 1; i++)
    {
        buffer[pos++] = s2[i];
    }

    buffer[pos] = (TChar)'\0';
    return pos;
}

// ============================================================================
// NUMBER CONVERSION IMPLEMENTATIONS
// ============================================================================

inline USIZE String::IntToStr(INT64 value, CHAR *buffer, USIZE bufSize) noexcept
{
    if (!buffer || bufSize < 2) return 0;

    CHAR temp[24];
    USIZE pos = 0;
    BOOL negative = FALSE;

    if (value < 0)
    {
        negative = TRUE;
        value = -value;
    }

    if (value == 0)
    {
        temp[pos++] = '0';
    }
    else
    {
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

    USIZE copyLen = pos < bufSize - 1 ? pos : bufSize - 1;
    for (USIZE i = 0; i < copyLen; i++)
    {
        buffer[i] = temp[pos - 1 - i];
    }
    buffer[copyLen] = '\0';
    return copyLen;
}

inline USIZE String::UIntToStr(UINT64 value, CHAR *buffer, USIZE bufSize) noexcept
{
    if (!buffer || bufSize < 2) return 0;

    CHAR temp[24];
    USIZE pos = 0;

    if (value == 0)
    {
        temp[pos++] = '0';
    }
    else
    {
        while (value > 0 && pos < 22)
        {
            temp[pos++] = '0' + (CHAR)(value % 10);
            value = value / 10;
        }
    }

    USIZE copyLen = pos < bufSize - 1 ? pos : bufSize - 1;
    for (USIZE i = 0; i < copyLen; i++)
    {
        buffer[i] = temp[pos - 1 - i];
    }
    buffer[copyLen] = '\0';
    return copyLen;
}

inline USIZE String::FloatToStr(DOUBLE value, CHAR *buffer, USIZE bufSize, UINT8 precision) noexcept
{
    if (!buffer || bufSize < 2) return 0;
    if (precision > 15) precision = 15;

    USIZE pos = 0;
    DOUBLE zero = DOUBLE(INT32(0));

    // Handle negative
    if (value < zero)
    {
        if (pos < bufSize - 1) buffer[pos++] = '-';
        value = -value;
    }

    // Rounding: add 0.5 / 10^precision
    if (precision > 0)
    {
        DOUBLE scale = DOUBLE(INT32(1));
        for (UINT8 p = 0; p < precision; p++)
            scale = scale * DOUBLE(INT32(10));
        value = value + DOUBLE(INT32(5)) / (scale * DOUBLE(INT32(10)));
    }
    else
    {
        value = value + DOUBLE(INT32(5)) / DOUBLE(INT32(10));
    }

    // Integer part
    UINT64 intPart = (UINT64)(INT64)value;
    DOUBLE fracPart = value - DOUBLE((INT64)intPart);

    CHAR intBuf[24];
    USIZE intLen = UIntToStr(intPart, intBuf, sizeof(intBuf));
    for (USIZE i = 0; i < intLen && pos < bufSize - 1; i++)
        buffer[pos++] = intBuf[i];

    // Fractional part
    if (precision > 0 && pos < bufSize - 1)
    {
        buffer[pos++] = '.';

        for (UINT8 p = 0; p < precision && pos < bufSize - 1; p++)
        {
            fracPart = fracPart * DOUBLE(INT32(10));
            INT32 digit = (INT32)fracPart;
            if (digit < 0) digit = 0;
            if (digit > 9) digit = 9;
            buffer[pos++] = '0' + digit;
            fracPart = fracPart - DOUBLE(digit);
        }

        // Trim trailing zeros
        while (pos > 2 && buffer[pos - 1] == '0' && buffer[pos - 2] != '.')
            pos--;
    }

    buffer[pos] = '\0';
    return pos;
}

inline BOOL String::ParseInt64(const CHAR *str, USIZE len, INT64 &result) noexcept
{
    if (!str || len == 0)
    {
        result = 0;
        return FALSE;
    }

    USIZE i = 0;
    BOOL negative = FALSE;

    while (i < len && (str[i] == ' ' || str[i] == '\t'))
    {
        i++;
    }

    if (i < len && str[i] == '-')
    {
        negative = TRUE;
        i++;
    }
    else if (i < len && str[i] == '+')
    {
        i++;
    }

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

FORCE_INLINE INT64 String::ParseInt64(PCCHAR str) noexcept
{
    INT64 result = 0;
    if (!str)
        return 0;
    ParseInt64(str, Length(str), result);
    return result;
}

inline BOOL String::StrToFloat(const CHAR *str, USIZE len, DOUBLE &result) noexcept
{
    if (!str || len == 0)
    {
        result = DOUBLE(INT32(0));
        return FALSE;
    }

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

inline UINT32 String::ParseHex(PCCHAR str) noexcept
{
    UINT32 result = 0;
    while (*str != '\0')
    {
        CHAR c = *str;
        UINT32 digit = 0;

        if (c >= '0' && c <= '9')
        {
            digit = c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            digit = 10 + (c - 'a');
        }
        else if (c >= 'A' && c <= 'F')
        {
            digit = 10 + (c - 'A');
        }
        else
        {
            break;
        }

        result = (result << 4) | digit;
        str++;
    }
    return result;
}

FORCE_INLINE PCHAR String::WriteDecimal(PCHAR buffer, UINT32 num) noexcept
{
    USIZE len = UIntToStr((UINT64)num, buffer, 12);
    return buffer + len;
}

inline PCHAR String::WriteHex(PCHAR buffer, UINT32 num, BOOL uppercase) noexcept
{
    if (num == 0)
    {
        buffer[0] = '0';
        buffer[1] = '\0';
        return buffer + 1;
    }

    CHAR temp[9];
    INT32 i = 0;
    CHAR baseChar = uppercase ? 'A' : 'a';

    while (num > 0)
    {
        UINT32 digit = num & 0xF;
        temp[i++] = (digit < 10) ? ('0' + digit) : (baseChar + digit - 10);
        num >>= 4;
    }

    INT32 j = 0;
    while (i > 0)
    {
        buffer[j++] = temp[--i];
    }
    buffer[j] = '\0';
    return buffer + j;
}

/** @} */ // end of string group
