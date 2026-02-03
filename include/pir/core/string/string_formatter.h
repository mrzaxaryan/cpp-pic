#pragma once

#include "primitives.h"
#include "string.h"

class StringFormatter
{
private:
    // Type-erased argument holder using PIC-safe types from primitives.h
    struct Argument {
        enum class Type { INT32, UINT32, INT64, UINT64, DOUBLE, CSTR, WSTR, PTR };

        Type type;
        union {
            INT32 i32;
            UINT32 u32;
            INT64 i64;
            UINT64 u64;
            DOUBLE dbl;
            const CHAR* cstr;
            const WCHAR* wstr;
            PVOID ptr;
        };

        // Default constructor
        Argument() : type(Type::INT32), i32(0) {}

        // PIC-safe primitive type constructors
        Argument(INT32 v) : type(Type::INT32), i32(v) {}
        Argument(UINT32 v) : type(Type::UINT32), u32(v) {}
        Argument(DOUBLE v) : type(Type::DOUBLE), dbl(v) {}

        // String and pointer constructors
        Argument(const CHAR* v) : type(Type::CSTR), cstr(v) {}
        Argument(CHAR* v) : type(Type::CSTR), cstr(v) {}
        Argument(const WCHAR* v) : type(Type::WSTR), wstr(v) {}
        Argument(WCHAR* v) : type(Type::WSTR), wstr(v) {}
        Argument(PVOID v) : type(Type::PTR), ptr(v) {}
        Argument(const void* v) : type(Type::PTR), ptr(const_cast<PVOID>(v)) {}

        // Native C++ type compatibility (INT64/UINT64 are typedefs)
        Argument(INT64 v) : type(Type::INT64), i64(v) {}
        Argument(UINT64 v) : type(Type::UINT64), u64(v) {}
#if defined(__LP64__) || defined(_LP64)
        Argument(signed long v) : type(Type::INT64), i64(INT64(v)) {}
        Argument(unsigned long v) : type(Type::UINT64), u64(UINT64(v)) {}
#endif
    };

    template <TCHAR TChar>
    static INT32 FormatWithArgs(BOOL (*writer)(PVOID, TChar), PVOID context, const TChar *format, const Argument* args, INT32 argCount);

private:
    template <TCHAR TChar>
    static INT32 FormatInt64(BOOL (*writer)(PVOID, TChar), PVOID context, INT64 num, INT32 width = 0, INT32 zeroPad = 0, INT32 leftAlign = 0);
    template <TCHAR TChar>
    static INT32 FormatUInt64(BOOL (*writer)(PVOID, TChar), PVOID context, UINT64 num, INT32 width = 0, INT32 zeroPad = 0, INT32 leftAlign = 0);
    template <TCHAR TChar>
    static INT32 FormatUInt64AsHex(BOOL (*writer)(PVOID, TChar), PVOID context, UINT64 num);
    template <TCHAR TChar>
    static INT32 FormatDouble(BOOL (*writer)(PVOID, TChar), PVOID context, DOUBLE num, INT32 precision = 6, INT32 width = 0, INT32 zeroPad = 0);
    template <TCHAR TChar>
    static INT32 FormatPointerAsHex(BOOL (*writer)(PVOID, TChar), PVOID context, PVOID ptr);
    template <TCHAR TChar>
    static INT32 FormatUInt32AsHex(BOOL (*writer)(PVOID, TChar), PVOID context, UINT32 num, INT32 fieldWidth = 0, INT32 uppercase = 0, INT32 zeroPad = 0, BOOL addPrefix = FALSE);

public:
    // C++11 variadic template version - supports custom types like DOUBLE
    template <TCHAR TChar, typename... Args>
    static INT32 Format(BOOL (*writer)(PVOID, TChar), PVOID context, const TChar *format, Args&&... args);
};

template <TCHAR TChar>
INT32 StringFormatter::FormatInt64(BOOL (*writer)(PVOID, TChar), PVOID context, INT64 num, INT32 width, INT32 zeroPad, INT32 leftAlign)
{
    BOOL isNegative = FALSE; // Flag to check if the number is negative
    INT32 index = 0;
    INT32 startIndex = index; // Store the starting index for the string

    // Handle negative numbers
    if (num < 0)
    {
        isNegative = TRUE; // Set the negative flag if the number is negative
        num = -num;        // Make the number positive for further processing
    }

    TChar rev[20]; // Temporary storage for reversed digits
    INT32 len = 0; // Length of the number in digits

    // Convert the number to a reversed string
    do
    {
        rev[len++] = (num % 10) + (TChar)'0'; // Get the last digit and convert it to character
        num /= 10;                            // Remove the last digit from the number
    } while (num);

    INT32 totalDigits = len;                               // Total number of digits in the number
    INT32 signWidth = isNegative ? 1 : 0;                  // Width for the sign character (1 for negative sign, 0 otherwise)
    INT32 paddingSpaces = width - totalDigits - signWidth; // Calculate padding spaces needed based on width, total digits, and sign width
    INT32 paddingZeros = 0;                                // Count of leading zeros to add

    // Calculate padding based on flags
    if (zeroPad && !leftAlign)
    {
        paddingZeros = paddingSpaces > 0 ? paddingSpaces : 0; // If zero padding is enabled and not left-aligned, use padding spaces for leading zeros
        paddingSpaces = 0;                                    // Set padding spaces to 0 since leading zeros are used
    }
    else
    {
        paddingSpaces = paddingSpaces > 0 ? paddingSpaces : 0; // If not zero padding or left-aligned, ensure padding spaces are non-negative
    }

    // If not left-aligned, pad spaces first
    if (!leftAlign)
    {
        for (INT32 i = 0; i < paddingSpaces; ++i)
        {
            // Add spaces before the number
            writer(context, (TChar)' ');
            index++;
        }
    }

    // Add negative sign if needed
    if (isNegative)
    {
        writer(context, (TChar)'-');
        index++;
    }

    // Add leading zeros
    for (INT32 i = 0; i < paddingZeros; ++i)
    {
        writer(context, (TChar)'0');
        index++;
    }

    // Copy digits in correct order
    while (len)
    {
        writer(context, rev[--len]);
        index++;
    }

    // If left-aligned, pad spaces after the number
    if (leftAlign)
    {
        INT32 printed = index - startIndex; // Number of characters printed so far
        for (INT32 i = printed; i < width; ++i)
        {
            // Add spaces after the number to fill the width
            writer(context, (TChar)' ');
            index++;
        }
    }
    return index - startIndex; // Return the total number of characters added to the string
}

template <TCHAR TChar>
INT32 StringFormatter::FormatUInt64(BOOL (*writer)(PVOID, TChar), PVOID context, UINT64 num, INT32 width, INT32 zeroPad, INT32 leftAlign)
{
    TChar rev[20]; // Temporary storage for reversed digits
    INT32 len = 0; // Length of the number in digits
    INT32 index = 0;
    INT32 startIndex = index; // Store the starting index for the string

    // Convert the unsigned number to a reversed string
    do
    {
        rev[len++] = (TChar)((num % 10) + (UINT64)(UINT32)'0'); // Convert last digit to character
        num /= 10;                            // Remove the last digit from the number
    } while (num);

    INT32 totalDigits = len;                   // Total number of digits in the number
    INT32 paddingSpaces = width - totalDigits; // Calculate padding spaces needed based on width and total digits
    INT32 paddingZeros = 0;                    // Count of leading zeros to add

    // Calculate padding
    if (zeroPad && !leftAlign)
    {
        paddingZeros = paddingSpaces > 0 ? paddingSpaces : 0;
        paddingSpaces = 0;
    }
    else
    {
        paddingSpaces = paddingSpaces > 0 ? paddingSpaces : 0;
    }

    // If not left-aligned, pad spaces first
    if (!leftAlign)
    {
        for (INT32 i = 0; i < paddingSpaces; ++i)
        {
            writer(context, (TChar)' ');
            index++;
        }
    }

    // Add leading zeros
    for (INT32 i = 0; i < paddingZeros; ++i)
    {
        writer(context, (TChar)'0');
        index++;
    }

    // Copy digits in correct order
    while (len)
    {
        writer(context, rev[--len]);
        index++;
    }

    // If left-aligned, pad trailing spaces
    if (leftAlign)
    {
        INT32 printed = index - startIndex;
        for (INT32 i = printed; i < width; ++i)
        {
            writer(context, (TChar)' ');
            index++;
        }
    }
    return index - startIndex; // total number of characters added to the string
}

template <TCHAR TChar>
INT32 StringFormatter::FormatUInt32AsHex(BOOL (*writer)(PVOID, TChar), PVOID context, UINT32 num, INT32 fieldWidth, INT32 uppercase, INT32 zeroPad, BOOL addPrefix)
{
    TChar buffer[16];
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
            UINT32 digit = num & 0xF; // num % 16
            TChar c;

            if (digit < 10)
                c = (TChar)('0' + digit);
            else
                c = (TChar)((uppercase ? 'A' : 'a') + (digit - 10));

            buffer[buffIndex++] = c;
            num >>= 4; // num /= 16
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
            writer(context, (TChar)' ');
            index++;
            pad--;
        }
    }

    // Prefix
    if (addPrefix)
    {
        writer(context, (TChar)'0');
        index++;
        writer(context, uppercase ? (TChar)'X' : (TChar)'x');
        index++;
    }

    // Zero padding (after prefix, before digits)
    if (zeroPad)
    {
        while (pad > 0)
        {
            writer(context, (TChar)'0');
            index++;
            pad--;
        }
    }

    // Copy digits (reverse order)
    while (buffIndex)
    {
        writer(context, buffer[--buffIndex]);
        index++;
    }

    return index - startIdx;
}

template <TCHAR TChar>
INT32 StringFormatter::FormatUInt64AsHex(BOOL (*writer)(PVOID, TChar), PVOID context, UINT64 num)
{
    TChar rev[16]; // max 16 hex digits for UINT64
    INT32 len = 0;
    INT32 index = 0;
    INT32 startIndex = index;

    do
    {
        UINT32 v = (UINT32)(num & 0xF); // num % 16
        rev[len++] = (TChar)(v < 10 ? (TChar)('0' + v)
                                    : (TChar)('a' + (v - 10)));
        num >>= 4; // num /= 16
    } while (num);

    while (len)
    {
        writer(context, rev[--len]);
        index++;
    }
    return index - startIndex;
}

template <TCHAR TChar>
INT32 StringFormatter::FormatPointerAsHex(BOOL (*writer)(PVOID, TChar), PVOID context, PVOID ptr)
{
    INT32 index = 0;
    INT32 startIndex = index;
    USIZE addr = (USIZE)ptr;

    writer(context, (TChar)'0');
    index++;
    writer(context, (TChar)'x');
    index++;
    for (INT32 i = (INT32)(sizeof(USIZE) * 2) - 1; i >= 0; --i)
    {
        UINT32 v = (addr >> (i * 4)) & 0xF;
        writer(context, (TChar)(v < 10 ? ('0' + v) : ('a' + (v - 10))));
        index++;
    }
    return index - startIndex;
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
        writer(context, (TChar)'n');
        writer(context, (TChar)'a');
        writer(context, (TChar)'n');
        // pad after
        INT32 pad = (width > 3) ? (width - 3) : 0;
        TChar pch = zeroPad ? (TChar)'0' : (TChar)' ';
        for (INT32 i = 0; i < pad; ++i)
            if (!writer(context, pch))
                return 3 + i;
        return 3 + pad;
    }

    BOOL isNegative = FALSE;
    if (num < 0)
    {
        isNegative = TRUE;
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

    // Emit number
    INT32 written = 0;
    for (INT32 i = 0; i < len; ++i)
    {
        if (!writer(context, tmp[i]))
            return written;
        written++;
    }

    // Pad AFTER (matches your original intent)
    if (width > written)
    {
        INT32 pad = width - written;
        TChar pch = zeroPad ? (TChar)'0' : (TChar)' ';
        for (INT32 i = 0; i < pad; ++i)
        {
            if (!writer(context, pch))
                return written;
            written++;
        }
    }

    return written;
}

// Variadic template implementation
template <TCHAR TChar, typename... Args>
INT32 StringFormatter::Format(BOOL (*writer)(PVOID, TChar), PVOID context, const TChar *format, Args&&... args)
{
    if constexpr (sizeof...(Args) == 0)
    {
        // No arguments, just copy the format string
        return FormatWithArgs<TChar>(writer, context, format, nullptr, 0);
    }
    else
    {
        // Pack arguments into array
        Argument argArray[] = { Argument(args)... };
        return FormatWithArgs<TChar>(writer, context, format, argArray, sizeof...(Args));
    }
}

template <TCHAR TChar>
INT32 StringFormatter::FormatWithArgs(BOOL (*writer)(PVOID, TChar), PVOID context, const TChar *format, const Argument* args, INT32 argCount)
    {
        INT32 i = 0, j = 0;  // Index for the format string and output string
        INT32 precision = 6; // Default precision for floating-point numbers
        INT32 currentArg = 0; // Current argument index

        // Validate the output string
        if (format == NULL)
        {
            return 0;
        }

    // Loop through the format string to process each character
    while (format[i] != (TChar)'\0')
    {
        if (format[i] == (TChar)'%')
        {
            i++;           // Skip '%'
            precision = 6; // Reset default precision

            // Handle precision for floating-point numbers (e.g. "%.3f")
            if (format[i] == (TChar)'.')
            {
                i++;           // Skip '.'
                precision = 0; // Reset precision to 0 before parsing
                while (format[i] >= (TChar)'0' && format[i] <= (TChar)'9')
                {                                                          // Parse precision value
                    precision = precision * 10 + (format[i] - (TChar)'0'); // Convert character to integer
                    i++;                                                   // Move to the next character
                }
            }

            INT32 addPrefix = 0; // Flag for adding '0x' prefix to hex numbers
            if (format[i] == (TChar)'#')
            {
                addPrefix = 1; // Set flag to add prefix
                i++;           // Skip '#'
            }
            // Check for an optional zero flag and field width
            INT32 leftAlign = 0;
            INT32 zeroPad = 0;
            INT32 fieldWidth = 0;

            // Check for optional flags: '-' for left align, '0' for zero padding
            while (format[i] == (TChar)'-' || format[i] == (TChar)'0')
            {
                if (format[i] == (TChar)'-')
                {
                    leftAlign = 1; // Set left alignment flag
                    zeroPad = 0;   // '-' overrides '0'
                }
                else if (format[i] == (TChar)'0' && !leftAlign)
                {
                    zeroPad = 1; // Set zero padding flag only if not left aligned
                }
                i++;
            }

            // Parse any numeric field width
            while (format[i] >= (TChar)'0' && format[i] <= (TChar)'9')
            {
                fieldWidth = fieldWidth * 10 + (format[i] - (TChar)'0');
                i++;
            }

            // Now switch based on the conversion specifier
            if (format[i] == (TChar)'X')
            {
                i++; // Skip 'X'
                if (currentArg >= argCount) continue;
                UINT32 num = args[currentArg++].u32;
                // Format the number as uppercase hexadecimal.
                j += StringFormatter::FormatUInt32AsHex(writer, context, num, fieldWidth, 1, zeroPad, addPrefix);

                // If a '-' follows, add it (for MAC address separators)
                if (format[i] == (TChar)'-')
                {
                    writer(context, (TChar)'-');
                    j++;
                    i++; // Skip the hyphen
                }
                continue;
            }
            // NOTE: making specifiers lowercase to handle both cases (e.g., %d and %D), that's why we use ToLowerCase function
            else if (String::ToLowerCase(format[i]) == (TChar)'f')
            {
                // Now we can use DOUBLE directly without casting!
                if (currentArg >= argCount) { i++; continue; }
                DOUBLE num = args[currentArg++].dbl;
                j += StringFormatter::FormatDouble(writer, context, num, precision, fieldWidth, zeroPad); // Convert the double to string with specified formatting
                i++;                                                                     // Skip 'f'
                continue;
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'d')
            {                                                                           // Handle %d (signed integer) :
                if (currentArg >= argCount) { i++; continue; }
                INT32 num = args[currentArg++].i32;                                     // Get the next argument as an INT32
                j += StringFormatter::FormatInt64(writer, context, num, fieldWidth, zeroPad, leftAlign); // Convert the integer to string with specified formatting
                i++;                                                                    // Skip 'd'
                continue;
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'u')
            {                                                                            // Handle %u (unsigned integer)
                if (currentArg >= argCount) { i++; continue; }
                UINT32 num = args[currentArg++].u32;                                     // Get the next argument as an UINT32
                j += StringFormatter::FormatUInt64(writer, context, UINT64(num), fieldWidth, zeroPad, leftAlign); // Convert the unsigned integer to string with specified formatting
                i++;                                                                     // Skip 'u'
                continue;
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'x')
            {                                                                                    // Handle %x (hexadecimal, lowercase)
                if (currentArg >= argCount) { i++; continue; }
                UINT32 num = args[currentArg++].u32;                                             // Get the next argument as an UINT32
                j += StringFormatter::FormatUInt32AsHex(writer, context, num, fieldWidth, 0, zeroPad, addPrefix); // Convert the number to lowercase hexadecimal with specified formatting
                i++;                                                                             // Skip 'x'
                continue;
            }
            else if (String::ToLowerCase(format[i]) == (TChar)'p')
            {                                                                  // Handle %p (pointer)
                i++;                                                           // Skip 'p'
                if (currentArg >= argCount) continue;
                j += StringFormatter::FormatPointerAsHex(writer, context, args[currentArg++].ptr); // Convert the pointer address to hexadecimal string
                continue;
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'c')
            { // Handle %c (character)
                // Loop through the field width to ensure proper spacing
                for (INT32 k = 0; k < fieldWidth - 1; k++)
                {
                    writer(context, (TChar)' '); // Add spaces for field width
                    j++;
                }
                if (currentArg < argCount) {
                    writer(context, (TChar)args[currentArg++].i32); // Get the next argument as an INT32 (character) and add it to the string
                }
                j++;
                i++; // Skip 'c'
                continue;
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'s')
            {                                     // Handle %s (narrow string)
                i++;                              // Skip 's'
                if (currentArg >= argCount) continue;
                CHAR *str = (CHAR*)args[currentArg++].cstr; // Get the next argument as a PWCHAR (narrow string)
                // C standard does not allow NULL strings, so if the string is NULL, handle it by printing '?'.
                if (str == NULL)
                {
                    writer(context, '?');
                    writer(context, '\0');
                    j += 2;
                    continue;
                }
                INT32 len = 0; // Length of the string to be printed

                // Checking the string is not NULL and calculating its length
                if (str)
                {
                    TChar *temp = (TChar *)str;
                    while (*temp)
                    {
                        len++;
                        temp++;
                    }
                    INT32 padding = fieldWidth - len; // Calculate padding based on field width and string length
                    if (padding < 0)
                        padding = 0; // Ensure padding is non-negative

                    // If left-aligned, copy the string directly
                    for (INT32 k = 0; k < padding; k++)
                    {
                        writer(context, (TChar)' '); // Add spaces for padding
                        j++;
                    }
                    // Copy the string to the output
                    while (*str)
                    {
                        writer(context, (TChar)*str++); // Copy each character from the string to the output
                        j++;
                    }
                }
                continue;
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'w')
            { // Handle %ws (wide string)
                if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'s')
                {
                    i += 2;                              // Skip over "ws"
                    if (currentArg >= argCount) continue;
                    WCHAR *wstr = (WCHAR*)args[currentArg++].wstr; // Get the next argument as a PWCHAR (wide string)
                    // C standard does not allow NULL strings, so if the string is NULL, handle it by printing '?'.
                    if (wstr == NULL)
                    {
                        writer(context, (TChar)'?');
                        writer(context, (TChar)'\0');
                        j += 2;
                        continue;
                    }

                    // copy wide string to output
                    for (INT32 k = 0; wstr[k] != (TChar)'\0'; k++)
                    {
                        writer(context, (TChar)wstr[k]); // Copy each character from the wide string to the output
                        j++;
                    }
                    continue;
                }
                else
                {
                    writer(context, (TChar)format[i++]); // If it's not %ws, just copy the character as is.
                    j++;
                    continue;
                }
            }
            // Support %ls (wide string) in the same way as %ws
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'l')
            {
                if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'s')
                {
                    i += 2;                              // Skip over "ls"
                    if (currentArg >= argCount) continue;
                    WCHAR *wstr = (WCHAR*)args[currentArg++].wstr; // Get the next argument as a PWCHAR (wide string)
                    // C standard does not allow NULL strings, so if the string is NULL, handle it by printing '?'.
                    if (wstr == NULL)
                    {
                        writer(context, (WCHAR)'?');
                        writer(context, (WCHAR)'\0');
                        j += 2;
                        continue;
                    }

                    // Copy wide string to output
                    for (INT32 k = 0; wstr[k] != (TChar)'\0'; k++)
                    {
                        writer(context, (TChar)wstr[k]); // Copy each character from the wide string to the output
                        j++;
                    }

                    continue;
                }
                // Handle other long variants ( ld, lu, lld)
                else if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'d')
                {                                                                           // long int (%ld)
                    i += 2;                                                                 // Skip over "ld"
                    if (currentArg >= argCount) continue;
                    INT32 num = args[currentArg++].i32;                                     // Get the next argument as an INT32 (long int)
                    j += StringFormatter::FormatInt64(writer, context, num, fieldWidth, zeroPad, leftAlign); // Convert the long int to string with specified formatting
                    continue;
                }
                else if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'u')
                {                                                                            // unsigned long int (%lu)
                    i += 2;                                                                  // Skip over "lu"
                    if (currentArg >= argCount) continue;
                    UINT32 num = args[currentArg++].u32;                                     // Get the next argument as an UINT32 (unsigned long int)
                    j += StringFormatter::FormatUInt64(writer, context, num, fieldWidth, zeroPad, leftAlign); // Convert the unsigned long int to string with specified formatting
                    continue;
                }
                else if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'l' && String::ToLowerCase<TChar>(format[i + 2]) == (TChar)'d')
                {                                                                           // long long int (%lld)
                    i += 3;                                                                 // Skip over "lld"
                    if (currentArg >= argCount) continue;
                    // Now we can use INT64 directly!
                    INT64 num = args[currentArg++].i64;
                    j += StringFormatter::FormatInt64(writer, context, num, fieldWidth, zeroPad, leftAlign); // Convert the long long int to string with specified formatting
                    continue;
                }
                else if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'l' && String::ToLowerCase<TChar>(format[i + 2]) == (TChar)'u')
                {
                    i += 3;                                                                  // Skip over "llu"
                    if (currentArg >= argCount) continue;
                    // Now we can use UINT64 directly!
                    UINT64 num = args[currentArg++].u64;
                    j += StringFormatter::FormatUInt64(writer, context, num, fieldWidth, zeroPad, leftAlign); // Convert the unsigned long long int to string with specified formatting
                    continue;
                }
                else if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'x')
                {                                                                            // long hex (%lx)
                    i += 2;                                                                  // Skip over "lx"
                    if (currentArg >= argCount) continue;
                    UINT64 num = args[currentArg++].u64;                                     // Get the next argument as UINT64
                    j += StringFormatter::FormatUInt64AsHex(writer, context, num);           // Convert to hex
                    continue;
                }
                else if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'l' && String::ToLowerCase<TChar>(format[i + 2]) == (TChar)'x')
                {                                                                            // long long hex (%llx)
                    i += 3;                                                                  // Skip over "llx"
                    if (currentArg >= argCount) continue;
                    UINT64 num = args[currentArg++].u64;                                     // Get the next argument as UINT64
                    j += StringFormatter::FormatUInt64AsHex(writer, context, num);           // Convert to hex
                    continue;
                }
                else
                {
                    writer(context, format[i++]); // If it's not recognized, just copy the character as is.
                    j++;
                    continue;
                }
            }
            // Handle size_t variants (%zu, %zd) - USIZE/SSIZE are stored as UINT64/INT64
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'z')
            {
                if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'u')
                {                                                                            // unsigned size_t (%zu)
                    i += 2;                                                                  // Skip over "zu"
                    if (currentArg >= argCount) continue;
                    // USIZE is converted to UINT64 through Argument constructor
                    UINT64 num = args[currentArg++].u64;                                     // Get as UINT64
                    j += StringFormatter::FormatUInt64(writer, context, num, fieldWidth, zeroPad, leftAlign); // Convert USIZE to string
                    continue;
                }
                else if (String::ToLowerCase<TChar>(format[i + 1]) == (TChar)'d')
                {                                                                            // signed size_t (%zd)
                    i += 2;                                                                  // Skip over "zd"
                    if (currentArg >= argCount) continue;
                    // SSIZE is converted to INT64 through Argument constructor
                    INT64 num = args[currentArg++].i64;                                      // Get as INT64
                    j += StringFormatter::FormatInt64(writer, context, num, fieldWidth, zeroPad, leftAlign); // Convert SSIZE to string
                    continue;
                }
                else
                {
                    writer(context, format[i++]); // If it's not recognized, just copy the character as is.
                    j++;
                    continue;
                }
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'f')
            {                                                                            // Handle %f (double)
                i++;                                                                     // Skip 'f'
                if (currentArg >= argCount) continue;
                // Now we can use DOUBLE directly without casting!
                DOUBLE num = args[currentArg++].dbl;
                j += StringFormatter::FormatDouble(writer, context, num, precision, fieldWidth, zeroPad); // Convert the double to string with specified formatting
                continue;
            }
            else if (String::ToLowerCase<TChar>(format[i]) == (TChar)'%')
            {                                // Handle literal "%%"
                writer(context, (TChar)'%'); // Output a literal '%'
                j++;
                i++; // Skip the '%'
                continue;
            }
            else
            {                                 // Unknown specifier: output it as-is.
                writer(context, format[i++]); // Copy the unknown specifier character to the output string
                j++;
                continue;
            }
        }
        else
        {                                 // Ordinary character: copy it.
            writer(context, format[i++]); // Copy the ordinary character to the output string
            j++;
        }
    }
    // writer(context, (TChar)'\0'); // Null-terminate the output string
    // j++;
    return j; // Return the length of the formatted string
}
