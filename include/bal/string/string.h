#pragma once

#include "primitives.h"
#include "double.h"
#include "embedded_string.h"

// Class to handle string operations
// Position-independent, no .rdata dependencies.
class String
{
public:
    // ============================================================================
    // CHARACTER CLASSIFICATION
    // ============================================================================

    // Check if character is a whitespace
    template <TCHAR TChar>
    static FORCE_INLINE BOOL IsSpace(TChar c) noexcept;

    // Check if character is a digit
    template <TCHAR TChar>
    static FORCE_INLINE BOOL IsDigit(TChar c) noexcept;

    // Check if character is alphabetic
    template <TCHAR TChar>
    static FORCE_INLINE BOOL IsAlpha(TChar c) noexcept;

    // Check if character is alphanumeric
    template <TCHAR TChar>
    static FORCE_INLINE BOOL IsAlphaNum(TChar c) noexcept;

    // ============================================================================
    // CHARACTER CONVERSION
    // ============================================================================

    // Convert character to lowercase
    template <TCHAR TChar>
    static FORCE_INLINE TChar ToLowerCase(TChar c) noexcept;

    // Convert character to uppercase
    template <TCHAR TChar>
    static FORCE_INLINE TChar ToUpperCase(TChar c) noexcept;

    // ============================================================================
    // STRING LENGTH AND COMPARISON
    // ============================================================================

    // Getting the length of a string
    template <TCHAR TChar>
    static FORCE_INLINE USIZE Length(const TChar *pChar) noexcept;

    // Compare two null-terminated strings
    template <TCHAR TChar>
    static FORCE_INLINE BOOL Compare(const TChar *s1, const TChar *s2, BOOL ignoreCase = FALSE) noexcept;

    // Compare two strings with explicit lengths
    template <TCHAR TChar>
    static FORCE_INLINE BOOL Equals(const TChar *a, USIZE aLen, const TChar *b, USIZE bLen) noexcept;

    // Compare two null-terminated strings for equality
    template <TCHAR TChar>
    static FORCE_INLINE BOOL Equals(const TChar *a, const TChar *b) noexcept;

    // Check if a string starts with a given substring (null-terminated)
    template <TCHAR TChar>
    static FORCE_INLINE BOOL StartsWith(const TChar *pChar, const TChar *pSubString) noexcept;

    // Check if string starts with prefix (with explicit lengths)
    template <TCHAR TChar>
    static FORCE_INLINE BOOL StartsWith(const TChar *str, USIZE strLen, const TChar *prefix, USIZE prefixLen) noexcept;

    // Check if string ends with suffix
    template <TCHAR TChar>
    static FORCE_INLINE BOOL EndsWith(const TChar *str, USIZE strLen, const TChar *suffix, USIZE suffixLen) noexcept;

    // ============================================================================
    // STRING SEARCH
    // ============================================================================

    // Find the address of a character in a string
    template <TCHAR TChar>
    static FORCE_INLINE const TChar *AddressOf(TChar c, const TChar *pChar) noexcept;

    // Find first occurrence of character, return index or -1
    template <TCHAR TChar>
    static FORCE_INLINE SSIZE IndexOfChar(const TChar *str, USIZE strLen, TChar ch) noexcept;

    // Find first occurrence of substring, return index or -1
    template <TCHAR TChar>
    static FORCE_INLINE SSIZE IndexOf(const TChar *str, USIZE strLen, const TChar *sub, USIZE subLen) noexcept;

    // ============================================================================
    // STRING COPY OPERATIONS
    // ============================================================================

    // Copy a string from src to dest (no bounds checking)
    template <TCHAR TChar>
    static FORCE_INLINE TChar *Copy(TChar *dest, const TChar *src) noexcept;

    // Safe string copy with explicit buffer size
    template <TCHAR TChar>
    static FORCE_INLINE USIZE Copy(TChar *dest, USIZE destSize, const TChar *src, USIZE srcLen) noexcept;

    // Safe string copy with compile-time buffer size
    template <USIZE MaxLen, TCHAR TChar>
    static FORCE_INLINE USIZE Copy(TChar (&dest)[MaxLen], const TChar *src, USIZE srcLen) noexcept;

    // Copy embedded string to buffer
    template <typename T>
    static FORCE_INLINE USIZE CopyEmbed(const T &src, CHAR *buffer, USIZE bufSize) noexcept;

    // ============================================================================
    // STRING MANIPULATION
    // ============================================================================

    // Trim whitespace from the end of a string (modifies in place)
    template <TCHAR TChar>
    static FORCE_INLINE USIZE TrimEnd(TChar *str) noexcept;

    // Trim whitespace from end (with explicit length)
    template <TCHAR TChar>
    static FORCE_INLINE void TrimEnd(const TChar *str, USIZE &len) noexcept;

    // Trim whitespace from start (returns new pointer, updates length)
    template <TCHAR TChar>
    static FORCE_INLINE const TChar *TrimStart(const TChar *str, USIZE &len) noexcept;

    // Trim whitespace from both ends
    template <TCHAR TChar>
    static FORCE_INLINE const TChar *Trim(const TChar *str, USIZE &len) noexcept;

    // Concatenate two strings into a buffer
    template <TCHAR TChar>
    static FORCE_INLINE USIZE Concat(TChar *buffer, USIZE bufSize,
                                     const TChar *s1, USIZE len1,
                                     const TChar *s2, USIZE len2) noexcept;

    // ============================================================================
    // NUMBER CONVERSION
    // ============================================================================

    // Convert integer to string
    static FORCE_INLINE USIZE IntToStr(INT64 value, CHAR *buffer, USIZE bufSize) noexcept;

    // Convert unsigned integer to string
    static FORCE_INLINE USIZE UIntToStr(UINT64 value, CHAR *buffer, USIZE bufSize) noexcept;

    // Convert DOUBLE to string with configurable precision
    static FORCE_INLINE USIZE FloatToStr(DOUBLE value, CHAR *buffer, USIZE bufSize, UINT8 precision = 6) noexcept;

    // Convert string to integer
    static FORCE_INLINE BOOL StrToInt(const CHAR *str, USIZE len, INT64 &result) noexcept;

    // Convert string to DOUBLE
    static FORCE_INLINE BOOL StrToFloat(const CHAR *str, USIZE len, DOUBLE &result) noexcept;

    // Note: naming may be improved later
    template <typename T>
    static T ParseString(const CHAR *str);

    // ============================================================================
    // UTF CONVERSION
    // ============================================================================

    // UTF-16 (WCHAR) <-> UTF-8 (CHAR) conversion
    static USIZE WideToUtf8(PCWCHAR wide, PCHAR utf8, USIZE utf8BufferSize);
    static USIZE Utf8ToWide(PCCHAR utf8, PWCHAR wide, USIZE wideBufferSize);
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
FORCE_INLINE TChar String::ToLowerCase(TChar c) noexcept
{
    if (c >= (TChar)'A' && c <= (TChar)'Z')
    {
        return c + ((TChar)'a' - (TChar)'A');
    }
    return c;
}

template <TCHAR TChar>
FORCE_INLINE TChar String::ToUpperCase(TChar c) noexcept
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
FORCE_INLINE USIZE String::Length(const TChar *p) noexcept
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
FORCE_INLINE BOOL String::Compare(const TChar *s1, const TChar *s2, BOOL ignoreCase) noexcept
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
FORCE_INLINE BOOL String::Equals(const TChar *a, USIZE aLen, const TChar *b, USIZE bLen) noexcept
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
FORCE_INLINE BOOL String::StartsWith(const TChar *pChar, const TChar *pSubString) noexcept
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
FORCE_INLINE const TChar *String::AddressOf(TChar c, const TChar *pChar) noexcept
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
FORCE_INLINE SSIZE String::IndexOf(const TChar *str, USIZE strLen, const TChar *sub, USIZE subLen) noexcept
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
FORCE_INLINE USIZE String::Copy(TChar *dest, USIZE destSize, const TChar *src, USIZE srcLen) noexcept
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
FORCE_INLINE USIZE String::CopyEmbed(const T &src, CHAR *buffer, USIZE bufSize) noexcept
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
FORCE_INLINE USIZE String::TrimEnd(TChar *str) noexcept
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
FORCE_INLINE USIZE String::Concat(TChar *buffer, USIZE bufSize,
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

FORCE_INLINE USIZE String::IntToStr(INT64 value, CHAR *buffer, USIZE bufSize) noexcept
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

FORCE_INLINE USIZE String::UIntToStr(UINT64 value, CHAR *buffer, USIZE bufSize) noexcept
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

FORCE_INLINE USIZE String::FloatToStr(DOUBLE value, CHAR *buffer, USIZE bufSize, UINT8 precision) noexcept
{
    if (!buffer || bufSize < 2) return 0;
    if (precision > 15) precision = 15;

    USIZE pos = 0;
    DOUBLE zero = DOUBLE(INT32(0));

    if (value < zero)
    {
        if (pos < bufSize - 1) buffer[pos++] = '-';
        value = -value;
    }

    INT64 intPart = (INT64)value;
    DOUBLE fracPart = value - DOUBLE(intPart);

    CHAR intBuf[24];
    USIZE intLen = IntToStr(intPart < 0 ? -intPart : intPart, intBuf, sizeof(intBuf));
    for (USIZE i = 0; i < intLen && pos < bufSize - 1; i++)
    {
        buffer[pos++] = intBuf[i];
    }

    if (precision > 0 && pos < bufSize - 1)
    {
        buffer[pos++] = '.';

        DOUBLE ten = DOUBLE(INT32(10));

        for (UINT8 p = 0; p < precision; p++)
        {
            fracPart = fracPart * ten;
        }

        DOUBLE half = DOUBLE(INT32(5)) / ten;
        fracPart = fracPart + half;
        UINT64 fracInt = (UINT64)(INT64)fracPart;

        CHAR fracBuf[24];
        USIZE fracLen = UIntToStr(fracInt, fracBuf, sizeof(fracBuf));

        USIZE leadingZeros = fracLen < precision ? precision - fracLen : 0;
        for (USIZE i = 0; i < leadingZeros && pos < bufSize - 1; i++)
        {
            buffer[pos++] = '0';
        }

        for (USIZE i = 0; i < fracLen && pos < bufSize - 1; i++)
        {
            buffer[pos++] = fracBuf[i];
        }

        while (pos > 2 && buffer[pos - 1] == '0' && buffer[pos - 2] != '.')
        {
            pos--;
        }
    }

    buffer[pos] = '\0';
    return pos;
}

FORCE_INLINE BOOL String::StrToInt(const CHAR *str, USIZE len, INT64 &result) noexcept
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

FORCE_INLINE BOOL String::StrToFloat(const CHAR *str, USIZE len, DOUBLE &result) noexcept
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
