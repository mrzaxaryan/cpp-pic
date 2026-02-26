/**
 * @file string_formatter.h
 * @brief Printf-style String Formatting
 *
 * @details Provides printf-style string formatting without CRT dependencies.
 * Uses a callback-based writer interface for flexible output destinations
 * (console, buffer, file, network, etc.).
 *
 * Supported format specifiers:
 * - %d, %D - Signed integer (INT32)
 * - %u, %U - Unsigned integer (UINT32)
 * - %x, %X - Hexadecimal (lowercase/uppercase)
 * - %f, %F - Floating-point (DOUBLE)
 * - %s, %S - String (CHAR*)
 * - %ws, %ls - Wide string (WCHAR*)
 * - %p, %P - Pointer
 * - %c, %C - Character
 * - %ld, %lu - Long integers
 * - %lld, %llu - Long long integers
 * - %lx, %lX, %llx, %llX - Long/long long hex
 * - %zu, %zd - Size_t variants
 * - %e, %E - Error value (Error struct from Result::Error())
 * - %% - Literal percent sign
 *
 * Format flags:
 * - Field width (e.g., %10d)
 * - Zero padding (e.g., %08x)
 * - Left alignment (e.g., %-10s)
 * - Precision for floats (e.g., %.3f)
 * - Alternate form prefix (e.g., %#x for 0x prefix)
 *
 * @note All formatting is position-independent with no .rdata dependencies.
 *
 * @ingroup core
 *
 * @defgroup formatter String Formatting
 * @ingroup core
 * @{
 */

#pragma once

#include "primitives.h"
#include "string.h"
#include "error.h"

/**
 * @class StringFormatter
 * @brief Printf-style string formatting with callback-based output
 *
 * @details Provides type-safe, variadic template-based string formatting.
 * Output is written through a callback function, enabling flexible output
 * destinations without buffer management concerns.
 *
 * @par Writer Callback:
 * The writer callback has the signature: BOOL (*writer)(PVOID context, TChar ch)
 * - Returns true to continue formatting
 * - Returns false to stop formatting (e.g., buffer full)
 *
 * @par Example Usage:
 * @code
 * // Buffer writer callback
 * struct BufferContext {
 *     CHAR* buffer;
 *     USIZE size;
 *     USIZE pos;
 * };
 *
 * BOOL BufferWriter(PVOID ctx, CHAR ch) {
 *     BufferContext* c = (BufferContext*)ctx;
 *     if (c->pos >= c->size - 1) return false;
 *     c->buffer[c->pos++] = ch;
 *     return true;
 * }
 *
 * // Format string
 * BufferContext ctx = { buffer, sizeof(buffer), 0 };
 * StringFormatter::Format(BufferWriter, &ctx, "Value: %d, Hex: %08X", 42, 0xDEADBEEF);
 * ctx.buffer[ctx.pos] = '\0';
 * @endcode
 */
class StringFormatter
{
public:
    /**
     * @brief Type-erased argument holder for variadic formatting
     * @details Uses PIC-safe types from primitives.h. Supports automatic
     * type conversion from common C++ types. Public so Logger can
     * type-erase arguments before calling FormatWithArgs directly,
     * eliminating per-argument-type template instantiations.
     */
    struct Argument
    {
        /** @brief Argument type enumeration */
        enum class Type
        {
            INT32,
            UINT32,
            INT64,
            UINT64,
            DOUBLE,
            CSTR,
            WSTR,
            PTR,
            ERROR_VALUE
        };

        Type type; ///< Type of stored value
        union
        {
            INT32 i32;         ///< Signed 32-bit integer
            UINT32 u32;        ///< Unsigned 32-bit integer
            INT64 i64;         ///< Signed 64-bit integer
            UINT64 u64;        ///< Unsigned 64-bit integer
            DOUBLE dbl;        ///< Floating-point value
            const CHAR *cstr;  ///< Narrow string pointer
            const WCHAR *wstr; ///< Wide string pointer
            PVOID ptr;         ///< Generic pointer
            Error errValue;    ///< Single error value
        };

        /// @name Constructors
        /// @{
        Argument() : type(Type::INT32), i32(0) {}
        Argument(INT32 v) : type(Type::INT32), i32(v) {}
        Argument(UINT32 v) : type(Type::UINT32), u32(v) {}
        Argument(DOUBLE v) : type(Type::DOUBLE), dbl(v) {}
        Argument(const CHAR *v) : type(Type::CSTR), cstr(v) {}
        Argument(CHAR *v) : type(Type::CSTR), cstr(v) {}
        Argument(const WCHAR *v) : type(Type::WSTR), wstr(v) {}
        Argument(WCHAR *v) : type(Type::WSTR), wstr(v) {}
        Argument(PVOID v) : type(Type::PTR), ptr(v) {}
        Argument(const void *v) : type(Type::PTR), ptr(const_cast<PVOID>(v)) {}
        Argument(const Error &v) : type(Type::ERROR_VALUE), errValue(v) {}
        Argument(INT64 v) : type(Type::INT64), i64(v) {}
        Argument(UINT64 v) : type(Type::UINT64), u64(v) {}
#if defined(__LP64__) || defined(_LP64)
        Argument(signed long v) : type(Type::INT64), i64(INT64(v)) {}
        Argument(unsigned long v) : type(Type::UINT64), u64(UINT64(v)) {}
#endif
        /// @}
    };

    /** @brief Format with pre-erased argument array (public for Logger type-erasure) */
    template <TCHAR TChar>
    static INT32 FormatWithArgs(BOOL (*writer)(PVOID, TChar), PVOID context, const TChar *format, const Argument *args, INT32 argCount);

private:
    /// @name Internal Formatting Functions
    /// @{
    template <TCHAR TChar>
    static INT32 FormatInt64(BOOL (*writer)(PVOID, TChar), PVOID context, INT64 num, INT32 width = 0, INT32 zeroPad = 0, INT32 leftAlign = 0);
    template <TCHAR TChar>
    static INT32 FormatUInt64(BOOL (*writer)(PVOID, TChar), PVOID context, UINT64 num, INT32 width = 0, INT32 zeroPad = 0, INT32 leftAlign = 0, TChar signChar = 0);
    template <TCHAR TChar>
    static INT32 FormatUInt64AsHex(BOOL (*writer)(PVOID, TChar), PVOID context, UINT64 num, INT32 fieldWidth = 0, INT32 uppercase = 0, INT32 zeroPad = 0, BOOL addPrefix = false);
    template <TCHAR TChar>
    static INT32 FormatDouble(BOOL (*writer)(PVOID, TChar), PVOID context, DOUBLE num, INT32 precision = 6, INT32 width = 0, INT32 zeroPad = 0);
    template <TCHAR TChar>
    static INT32 FormatPointerAsHex(BOOL (*writer)(PVOID, TChar), PVOID context, PVOID ptr);
    template <TCHAR TChar>
    static INT32 FormatWideString(BOOL (*writer)(PVOID, TChar), PVOID context, const WCHAR *wstr, INT32 fieldWidth = 0, INT32 leftAlign = 0);
    template <TCHAR TChar>
    static INT32 FormatError(BOOL (*writer)(PVOID, TChar), PVOID context, const Error &error);
    /// @}

public:
    /**
     * @brief Format string with variadic arguments
     * @tparam TChar Character type (CHAR or WCHAR)
     * @tparam Args Variadic argument types
     * @param writer Callback function to write each character
     * @param context User context passed to writer callback
     * @param format Printf-style format string
     * @param args Arguments to format
     * @return Number of characters written
     *
     * @details Type-safe variadic template implementation. Arguments are
     * automatically converted to the appropriate internal type.
     */
    template <TCHAR TChar, typename... Args>
    static INT32 Format(BOOL (*writer)(PVOID, TChar), PVOID context, const TChar *format, Args &&...args);
};

template <TCHAR TChar>
INT32 StringFormatter::FormatInt64(BOOL (*writer)(PVOID, TChar), PVOID context, INT64 num, INT32 width, INT32 zeroPad, INT32 leftAlign)
{
    if (num < 0)
        return FormatUInt64<TChar>(writer, context, UINT64(0) - UINT64(num), width, zeroPad, leftAlign, (TChar)'-');
    return FormatUInt64<TChar>(writer, context, (UINT64)num, width, zeroPad, leftAlign, (TChar)0);
}

template <TCHAR TChar>
INT32 StringFormatter::FormatUInt64(BOOL (*writer)(PVOID, TChar), PVOID context, UINT64 num, INT32 width, INT32 zeroPad, INT32 leftAlign, TChar signChar)
{
    TChar rev[20];
    INT32 len = 0;
    INT32 index = 0;
    INT32 startIndex = index;

    // Convert number to reversed digit string
    do
    {
        rev[len++] = (TChar)((num % 10) + (UINT64)(UINT32)'0');
        num /= 10;
    } while (num);

    INT32 signWidth = signChar ? 1 : 0;
    INT32 paddingSpaces = width - len - signWidth;
    INT32 paddingZeros = 0;

    if (zeroPad && !leftAlign)
    {
        paddingZeros = paddingSpaces > 0 ? paddingSpaces : 0;
        paddingSpaces = 0;
    }
    else
    {
        paddingSpaces = paddingSpaces > 0 ? paddingSpaces : 0;
    }

    // Right-align: pad spaces first
    if (!leftAlign)
    {
        for (INT32 i = 0; i < paddingSpaces; ++i)
        {
            if (!writer(context, (TChar)' '))
                return index - startIndex;
            index++;
        }
    }

    // Output sign if present
    if (signChar)
    {
        if (!writer(context, signChar))
            return index - startIndex;
        index++;
    }

    // Leading zeros
    for (INT32 i = 0; i < paddingZeros; ++i)
    {
        if (!writer(context, (TChar)'0'))
            return index - startIndex;
        index++;
    }

    // Digits in correct order
    while (len)
    {
        if (!writer(context, rev[--len]))
            return index - startIndex;
        index++;
    }

    // Left-align: pad trailing spaces
    if (leftAlign)
    {
        INT32 printed = index - startIndex;
        for (INT32 i = printed; i < width; ++i)
        {
            if (!writer(context, (TChar)' '))
                return index - startIndex;
            index++;
        }
    }
    return index - startIndex;
}

template <TCHAR TChar>
INT32 StringFormatter::FormatUInt64AsHex(BOOL (*writer)(PVOID, TChar), PVOID context, UINT64 num, INT32 fieldWidth, INT32 uppercase, INT32 zeroPad, BOOL addPrefix)
{
    TChar buffer[16]; // max 16 hex digits for UINT64
    INT32 buffIndex = 0;
    INT32 index = 0;
    INT32 startIdx = index;

    // Convert number to hex (reversed)
    if (num == 0)
    {
        buffer[buffIndex++] = (TChar)'0';
    }
    else
    {
        while (num)
        {
            UINT32 digit = (UINT32)(num & 0xF);
            TChar c;

            if (digit < 10)
                c = (TChar)('0' + digit);
            else
                c = (TChar)((uppercase ? 'A' : 'a') + (digit - 10));

            buffer[buffIndex++] = c;
            num >>= 4;
        }
    }

    INT32 prefixLen = addPrefix ? 2 : 0;
    INT32 totalDigits = buffIndex + prefixLen;
    INT32 pad = fieldWidth - totalDigits;
    if (pad < 0)
        pad = 0;

    // Space padding (right aligned) must come BEFORE prefix
    if (!zeroPad)
    {
        while (pad > 0)
        {
            if (!writer(context, (TChar)' '))
                return index - startIdx;
            index++;
            pad--;
        }
    }

    // Prefix
    if (addPrefix)
    {
        if (!writer(context, (TChar)'0'))
            return index - startIdx;
        index++;
        if (!writer(context, uppercase ? (TChar)'X' : (TChar)'x'))
            return index - startIdx;
        index++;
    }

    // Zero padding (after prefix, before digits)
    if (zeroPad)
    {
        while (pad > 0)
        {
            if (!writer(context, (TChar)'0'))
                return index - startIdx;
            index++;
            pad--;
        }
    }

    // Copy digits (reverse order)
    while (buffIndex)
    {
        if (!writer(context, buffer[--buffIndex]))
            return index - startIdx;
        index++;
    }

    return index - startIdx;
}

template <TCHAR TChar>
INT32 StringFormatter::FormatPointerAsHex(BOOL (*writer)(PVOID, TChar), PVOID context, PVOID ptr)
{
    return FormatUInt64AsHex<TChar>(writer, context, (UINT64)(USIZE)ptr, (INT32)(sizeof(USIZE) * 2), 0, 1, true);
}

template <TCHAR TChar>
INT32 StringFormatter::FormatWideString(BOOL (*writer)(PVOID, TChar), PVOID context, const WCHAR *wstr, INT32 fieldWidth, INT32 leftAlign)
{
    INT32 j = 0;
    if (wstr == nullptr)
    {
        if (!writer(context, (TChar)'?'))
            return 0;
        return 1;
    }

    // Measure string length for padding
    INT32 len = 0;
    for (INT32 k = 0; wstr[k] != (WCHAR)'\0'; k++)
        len++;

    INT32 padding = fieldWidth - len;
    if (padding < 0)
        padding = 0;

    // Right-align: leading spaces
    if (!leftAlign)
    {
        for (INT32 k = 0; k < padding; k++)
        {
            if (!writer(context, (TChar)' '))
                return j;
            j++;
        }
    }

    // Output string
    for (INT32 k = 0; wstr[k] != (WCHAR)'\0'; k++)
    {
        if (!writer(context, (TChar)wstr[k]))
            return j;
        j++;
    }

    // Left-align: trailing spaces
    if (leftAlign)
    {
        for (INT32 k = 0; k < padding; k++)
        {
            if (!writer(context, (TChar)' '))
                return j;
            j++;
        }
    }

    return j;
}

template <TCHAR TChar>
INT32 StringFormatter::FormatError(BOOL (*writer)(PVOID, TChar), PVOID context, const Error &error)
{
    INT32 j = 0;

    UINT32 code = (UINT32)error.Code;
    Error::PlatformKind platform = error.Platform;

    // Windows/UEFI: hex with 0x prefix (uppercase digits). Runtime/Posix: decimal.
    if (platform == Error::PlatformKind::Windows || platform == Error::PlatformKind::Uefi)
    {
        if (!writer(context, (TChar)'0')) return j; j++;
        if (!writer(context, (TChar)'x')) return j; j++;
        j += FormatUInt64AsHex<TChar>(writer, context, (UINT64)code, 0, 1, 0, false);
    }
    else
        j += FormatUInt64<TChar>(writer, context, (UINT64)code, 0, 0, 0);

    // Platform tag for non-Runtime: [W], [P], or [U]
    TChar tag = 0;
    if (platform == Error::PlatformKind::Windows)      tag = (TChar)'W';
    else if (platform == Error::PlatformKind::Posix)   tag = (TChar)'P';
    else if (platform == Error::PlatformKind::Uefi)    tag = (TChar)'U';

    if (tag)
    {
        if (!writer(context, (TChar)'[')) return j; j++;
        if (!writer(context, tag))        return j; j++;
        if (!writer(context, (TChar)']')) return j; j++;
    }

    return j;
}

template <TCHAR TChar>
INT32 StringFormatter::FormatDouble(
    BOOL (*writer)(PVOID, TChar),
    PVOID context,
    DOUBLE num,
    INT32 precision,
    INT32 width,
    INT32 zeroPad)
{
    DOUBLE d0_5 = 0.5_embed;
    // Clamp precision to something safe for a small stack buffer
    if (precision < 0)
        precision = 0;
    if (precision > 32)
        precision = 32;

    // Handle NaN (portable check)
    if (num != num)
    {
        INT32 pad = (width > 3) ? (width - 3) : 0;
        // Right-align: pad before "nan"
        for (INT32 i = 0; i < pad; ++i)
            if (!writer(context, (TChar)' '))
                return i;
        writer(context, (TChar)'n');
        writer(context, (TChar)'a');
        writer(context, (TChar)'n');
        return 3 + pad;
    }

    BOOL isNegative = false;
    if (num < 0)
    {
        isNegative = true;
        num = -num;
    }

    // Rounding: num += 0.5 / 10^precision
    if (precision > 0)
    {
        DOUBLE scale = 1.0_embed;
        for (INT32 i = 0; i < precision; ++i)
            scale *= (DOUBLE)10.0_embed;
        num += (d0_5 / scale);
    }
    else
    {
        // precision == 0 => round to integer
        num += d0_5;
    }

    // Build into a small local buffer, then emit through writer()
    // Max: sign(1) + 20 digits + '.' + 32 frac + a bit extra
    TChar tmp[80];
    INT32 len = 0;

    if (isNegative)
        tmp[len++] = (TChar)'-';

    // Integer part
    UINT64 int_part = (UINT64)num;
    DOUBLE frac_part = num - int_part;

    // Convert integer to reversed digits
    TChar intRev[32];
    INT32 intN = 0;

    if (int_part == 0)
    {
        intRev[intN++] = (TChar)'0';
    }
    else
    {
        while (int_part != 0 && intN < (INT32)(sizeof(intRev) / sizeof(intRev[0])))
        {
            UINT32 digit = (UINT32)(int_part % 10);
            intRev[intN++] = (TChar)((TChar)'0' + (TChar)digit);
            int_part /= 10;
        }
    }

    // Reverse into tmp
    for (INT32 i = intN - 1; i >= 0; --i)
        tmp[len++] = intRev[i];

    // Fractional part
    if (precision > 0)
    {
        tmp[len++] = (TChar)'.';
        for (INT32 i = 0; i < precision; ++i)
        {
            frac_part *= (DOUBLE)10.0_embed;
            INT32 d = (INT32)frac_part;
            if (d < 0)
                d = 0;
            if (d > 9)
                d = 9;
            tmp[len++] = (TChar)((TChar)'0' + (TChar)d);
            frac_part -= d;
        }
    }

    // Right-align: pad BEFORE the number
    INT32 written = 0;
    if (width > len)
    {
        INT32 pad = width - len;
        if (zeroPad)
        {
            // Zero-pad: emit sign first, then zeros, then digits
            INT32 signLen = isNegative ? 1 : 0;
            if (signLen)
            {
                if (!writer(context, tmp[0]))
                    return written;
                written++;
            }
            for (INT32 i = 0; i < pad; ++i)
            {
                if (!writer(context, (TChar)'0'))
                    return written;
                written++;
            }
            for (INT32 i = signLen; i < len; ++i)
            {
                if (!writer(context, tmp[i]))
                    return written;
                written++;
            }
        }
        else
        {
            // Space-pad before the number
            for (INT32 i = 0; i < pad; ++i)
            {
                if (!writer(context, (TChar)' '))
                    return written;
                written++;
            }
            for (INT32 i = 0; i < len; ++i)
            {
                if (!writer(context, tmp[i]))
                    return written;
                written++;
            }
        }
    }
    else
    {
        for (INT32 i = 0; i < len; ++i)
        {
            if (!writer(context, tmp[i]))
                return written;
            written++;
        }
    }

    return written;
}

// Variadic template implementation
template <TCHAR TChar, typename... Args>
INT32 StringFormatter::Format(BOOL (*writer)(PVOID, TChar), PVOID context, const TChar *format, Args &&...args)
{
    if constexpr (sizeof...(Args) == 0)
    {
        // No arguments, just copy the format string
        return FormatWithArgs<TChar>(writer, context, format, nullptr, 0);
    }
    else
    {
        // Pack arguments into array
        Argument argArray[] = {Argument(args)...};
        return FormatWithArgs<TChar>(writer, context, format, argArray, sizeof...(Args));
    }
}

template <TCHAR TChar>
INT32 StringFormatter::FormatWithArgs(BOOL (*writer)(PVOID, TChar), PVOID context, const TChar *format, const Argument *args, INT32 argCount)
{
    INT32 i = 0, j = 0;   // Index for the format string and output string
    INT32 precision = 6;  // Default precision for floating-point numbers
    INT32 currentArg = 0; // Current argument index

    // Validate the output string
    if (format == nullptr)
    {
        return 0;
    }

    // Loop through the format string to process each character
    while (format[i] != (TChar)'\0')
    {
        if (format[i] == (TChar)'%')
        {
            i++; // Skip '%'

            // Guard against trailing '%' at end of format string
            if (format[i] == (TChar)'\0')
                break;

            precision = 6; // Reset default precision

            // Parse flags: '-', '0', '#'
            INT32 addPrefix = 0;
            INT32 leftAlign = 0;
            INT32 zeroPad = 0;
            INT32 fieldWidth = 0;

            BOOL parsingFlags = true;
            while (parsingFlags)
            {
                if (format[i] == (TChar)'-')
                {
                    leftAlign = 1;
                    zeroPad = 0; // '-' overrides '0'
                    i++;
                }
                else if (format[i] == (TChar)'0' && !leftAlign)
                {
                    zeroPad = 1;
                    i++;
                }
                else if (format[i] == (TChar)'#')
                {
                    addPrefix = 1;
                    i++;
                }
                else
                {
                    parsingFlags = false;
                }
            }

            // Parse field width
            while (format[i] >= (TChar)'0' && format[i] <= (TChar)'9')
            {
                fieldWidth = fieldWidth * 10 + (format[i] - (TChar)'0');
                i++;
            }

            // Parse precision (e.g. "%.3f")
            if (format[i] == (TChar)'.')
            {
                i++; // Skip '.'
                precision = 0;
                while (format[i] >= (TChar)'0' && format[i] <= (TChar)'9')
                {
                    precision = precision * 10 + (format[i] - (TChar)'0');
                    i++;
                }
            }

            // Now switch based on the conversion specifier
            if (format[i] == (TChar)'X')
            {
                i++; // Skip 'X'
                if (currentArg >= argCount)
                    continue;
                UINT32 num = args[currentArg++].u32;
                // Format the number as uppercase hexadecimal.
                j += StringFormatter::FormatUInt64AsHex(writer, context, (UINT64)num, fieldWidth, 1, zeroPad, addPrefix);

                // If a '-' follows, add it (for MAC address separators)
                if (format[i] == (TChar)'-')
                {
                    if (!writer(context, (TChar)'-'))
                        return j;
                    j++;
                    i++; // Skip the hyphen
                }
                continue;
            }
            // NOTE: making specifiers lowercase to handle both cases (e.g., %d and %D), that's why we use ToLowerCase function
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'f')
            {
                if (currentArg >= argCount)
                {
                    i++;
                    continue;
                }
                DOUBLE num = args[currentArg++].dbl;
                j += StringFormatter::FormatDouble(writer, context, num, precision, fieldWidth, zeroPad);
                i++; // Skip 'f'
                continue;
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'d')
            { // Handle %d (signed integer)
                if (currentArg >= argCount)
                {
                    i++;
                    continue;
                }
                INT32 num = args[currentArg++].i32;
                j += StringFormatter::FormatInt64(writer, context, num, fieldWidth, zeroPad, leftAlign);
                i++; // Skip 'd'
                continue;
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'u')
            { // Handle %u (unsigned integer)
                if (currentArg >= argCount)
                {
                    i++;
                    continue;
                }
                UINT32 num = args[currentArg++].u32;
                j += StringFormatter::FormatUInt64(writer, context, UINT64(num), fieldWidth, zeroPad, leftAlign);
                i++; // Skip 'u'
                continue;
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'x')
            { // Handle %x (hexadecimal, lowercase)
                if (currentArg >= argCount)
                {
                    i++;
                    continue;
                }
                UINT32 num = args[currentArg++].u32;
                j += StringFormatter::FormatUInt64AsHex(writer, context, (UINT64)num, fieldWidth, 0, zeroPad, addPrefix);
                i++; // Skip 'x'
                continue;
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'p')
            {        // Handle %p (pointer)
                i++; // Skip 'p'
                if (currentArg >= argCount)
                    continue;
                j += StringFormatter::FormatPointerAsHex(writer, context, args[currentArg++].ptr);
                continue;
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'c')
            { // Handle %c (character)
                if (currentArg >= argCount)
                {
                    i++;
                    continue;
                }
                TChar ch = (TChar)args[currentArg++].i32;
                INT32 padding = fieldWidth > 1 ? fieldWidth - 1 : 0;

                if (!leftAlign)
                {
                    for (INT32 k = 0; k < padding; k++)
                    {
                        if (!writer(context, (TChar)' '))
                            return j;
                        j++;
                    }
                }
                if (!writer(context, ch))
                    return j;
                j++;
                if (leftAlign)
                {
                    for (INT32 k = 0; k < padding; k++)
                    {
                        if (!writer(context, (TChar)' '))
                            return j;
                        j++;
                    }
                }
                i++; // Skip 'c'
                continue;
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'s')
            {        // Handle %s (narrow string)
                i++; // Skip 's'
                if (currentArg >= argCount)
                    continue;
                const CHAR *str = args[currentArg++].cstr;
                if (str == nullptr)
                {
                    if (!writer(context, (TChar)'?'))
                        return j;
                    j++;
                    continue;
                }
                INT32 len = 0;
                const CHAR *temp = str;
                while (*temp)
                {
                    len++;
                    temp++;
                }
                INT32 padding = fieldWidth - len;
                if (padding < 0)
                    padding = 0;

                if (!leftAlign)
                {
                    for (INT32 k = 0; k < padding; k++)
                    {
                        if (!writer(context, (TChar)' '))
                            return j;
                        j++;
                    }
                }
                while (*str)
                {
                    if (!writer(context, (TChar)*str++))
                        return j;
                    j++;
                }
                if (leftAlign)
                {
                    for (INT32 k = 0; k < padding; k++)
                    {
                        if (!writer(context, (TChar)' '))
                            return j;
                        j++;
                    }
                }
                continue;
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'w')
            { // Handle %ws (wide string)
                if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'s')
                {
                    i += 2; // Skip over "ws"
                    if (currentArg >= argCount)
                        continue;
                    j += FormatWideString<TChar>(writer, context, args[currentArg++].wstr, fieldWidth, leftAlign);
                    continue;
                }
                else
                {
                    if (!writer(context, (TChar)format[i++]))
                        return j;
                    j++;
                    continue;
                }
            }
            // Support %ls (wide string) in the same way as %ws
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'l')
            {
                if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'s')
                {
                    i += 2; // Skip over "ls"
                    if (currentArg >= argCount)
                        continue;
                    j += FormatWideString<TChar>(writer, context, args[currentArg++].wstr, fieldWidth, leftAlign);
                    continue;
                }
                // Handle other long variants (ld, lu, lld)
                else if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'d')
                {           // long int (%ld)
                    i += 2; // Skip over "ld"
                    if (currentArg >= argCount)
                        continue;
                    // Type-aware read: on LLP64 (Windows) long is 32-bit, stored as INT32
                    const Argument &arg = args[currentArg++];
                    INT64 num = (arg.type == Argument::Type::INT64) ? arg.i64 : (INT64)arg.i32;
                    j += StringFormatter::FormatInt64(writer, context, num, fieldWidth, zeroPad, leftAlign);
                    continue;
                }
                else if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'u')
                {           // unsigned long int (%lu)
                    i += 2; // Skip over "lu"
                    if (currentArg >= argCount)
                        continue;
                    // Type-aware read: on LLP64 (Windows) unsigned long is 32-bit, stored as UINT32
                    const Argument &arg = args[currentArg++];
                    UINT64 num = (arg.type == Argument::Type::UINT64) ? arg.u64 : (UINT64)arg.u32;
                    j += StringFormatter::FormatUInt64(writer, context, num, fieldWidth, zeroPad, leftAlign);
                    continue;
                }
                else if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'l' && String::ToLowerCase<TChar>(format[i + 2]) == (TChar)'d')
                {           // long long int (%lld)
                    i += 3; // Skip over "lld"
                    if (currentArg >= argCount)
                        continue;
                    INT64 num = args[currentArg++].i64;
                    j += StringFormatter::FormatInt64(writer, context, num, fieldWidth, zeroPad, leftAlign);
                    continue;
                }
                else if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'l' && String::ToLowerCase<TChar>(format[i + 2]) == (TChar)'u')
                {
                    i += 3; // Skip over "llu"
                    if (currentArg >= argCount)
                        continue;
                    UINT64 num = args[currentArg++].u64;
                    j += StringFormatter::FormatUInt64(writer, context, num, fieldWidth, zeroPad, leftAlign);
                    continue;
                }
                else if (format[i + 1] == (TChar)'X')
                {           // long uppercase hex (%lX)
                    i += 2; // Skip over "lX"
                    if (currentArg >= argCount)
                        continue;
                    const Argument &arg = args[currentArg++];
                    UINT64 num = (arg.type == Argument::Type::UINT64) ? arg.u64 : (UINT64)arg.u32;
                    j += StringFormatter::FormatUInt64AsHex(writer, context, num, fieldWidth, 1, zeroPad, addPrefix);
                    continue;
                }
                else if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'x')
                {           // long hex (%lx)
                    i += 2; // Skip over "lx"
                    if (currentArg >= argCount)
                        continue;
                    const Argument &arg = args[currentArg++];
                    UINT64 num = (arg.type == Argument::Type::UINT64) ? arg.u64 : (UINT64)arg.u32;
                    j += StringFormatter::FormatUInt64AsHex(writer, context, num, fieldWidth, 0, zeroPad, addPrefix);
                    continue;
                }
                else if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'l' && format[i + 2] == (TChar)'X')
                {           // long long uppercase hex (%llX)
                    i += 3; // Skip over "llX"
                    if (currentArg >= argCount)
                        continue;
                    UINT64 num = args[currentArg++].u64;
                    j += StringFormatter::FormatUInt64AsHex(writer, context, num, fieldWidth, 1, zeroPad, addPrefix);
                    continue;
                }
                else if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'l' && String::ToLowerCase<TChar>(format[i + 2]) == (TChar)'x')
                {           // long long hex (%llx)
                    i += 3; // Skip over "llx"
                    if (currentArg >= argCount)
                        continue;
                    UINT64 num = args[currentArg++].u64;
                    j += StringFormatter::FormatUInt64AsHex(writer, context, num, fieldWidth, 0, zeroPad, addPrefix);
                    continue;
                }
                else
                {
                    if (!writer(context, format[i++]))
                        return j;
                    j++;
                    continue;
                }
            }
            // Handle size_t variants (%zu, %zd) - USIZE/SSIZE are stored as UINT64/INT64
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'z')
            {
                if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'u')
                {           // unsigned size_t (%zu)
                    i += 2; // Skip over "zu"
                    if (currentArg >= argCount)
                        continue;
                    // Type-aware read: on 32-bit platforms USIZE is stored as UINT32
                    const Argument &arg = args[currentArg++];
                    UINT64 num = (arg.type == Argument::Type::UINT64) ? arg.u64 : (UINT64)arg.u32;
                    j += StringFormatter::FormatUInt64(writer, context, num, fieldWidth, zeroPad, leftAlign);
                    continue;
                }
                else if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'d')
                {           // signed size_t (%zd)
                    i += 2; // Skip over "zd"
                    if (currentArg >= argCount)
                        continue;
                    // Type-aware read: on 32-bit platforms SSIZE is stored as INT32
                    const Argument &arg = args[currentArg++];
                    INT64 num = (arg.type == Argument::Type::INT64) ? arg.i64 : (INT64)arg.i32;
                    j += StringFormatter::FormatInt64(writer, context, num, fieldWidth, zeroPad, leftAlign);
                    continue;
                }
                else
                {
                    if (!writer(context, format[i++]))
                        return j;
                    j++;
                    continue;
                }
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'e')
            {        // Handle %e (error value)
                i++; // Skip 'e'
                if (currentArg >= argCount)
                    continue;
                j += FormatError<TChar>(writer, context, args[currentArg++].errValue);
                continue;
            }
            else if (format[i] == (TChar)'%')
            { // Handle literal "%%"
                if (!writer(context, (TChar)'%'))
                    return j;
                j++;
                i++; // Skip the '%'
                continue;
            }
            else
            { // Unknown specifier: output it as-is.
                if (!writer(context, format[i++]))
                    return j;
                j++;
                continue;
            }
        }
        else
        { // Ordinary character: copy it.
            if (!writer(context, format[i++]))
                return j;
            j++;
        }
    }
    return j; // Return the length of the formatted string
}

/** @} */ // end of formatter group
