/**
 * stdlib.h - Standard Library for PIL (Position Independent Language)
 *
 * Native functions using the CFunction API.
 * print() outputs via State.SetOutput() callback.
 *
 * Position-independent, no .rdata dependencies.
 *
 * Part of RAL (Runtime Abstraction Layer).
 *
 * USAGE:
 *   PIL::State L;
 *   L.SetOutput(MyOutputFunc);  // Set output callback
 *   PIL::OpenStdLib(L);      // Registers print, len, str, num, type
 *   L.DoString("print(\"Hello!\");");
 */

#pragma once

#include "value.h"  // includes State forward declaration

namespace PIL
{

// ============================================================================
// VALUE TO STRING HELPER
// ============================================================================

NOINLINE USIZE ValueToString(const Value& value, CHAR* buffer, USIZE bufferSize) noexcept
{
    if (!buffer || bufferSize < 2) return 0;

    switch (value.type)
    {
        case ValueType::NIL:
            return String::CopyEmbed("nil"_embed, buffer, bufferSize);

        case ValueType::BOOL:
            return value.boolValue
                ? String::CopyEmbed("true"_embed, buffer, bufferSize)
                : String::CopyEmbed("false"_embed, buffer, bufferSize);

        case ValueType::NUMBER:
            {
                DOUBLE d = value.numberValue;
                INT64 intPart = (INT64)d;
                DOUBLE reconstructed = DOUBLE(INT32(intPart));

                if (d == reconstructed)
                {
                    // Format as integer using StrUtil
                    return String::IntToStr(intPart, buffer, bufferSize);
                }
                else
                {
                    // Format as float using StrUtil
                    return String::FloatToStr(d, buffer, bufferSize, 6);
                }
            }

        case ValueType::STRING:
            return String::Copy(buffer, bufferSize, value.strValue, value.strLength);

        case ValueType::FUNCTION:
            {
                USIZE len = 0;
                if (len < bufferSize - 1) buffer[len++] = '<';
                if (len < bufferSize - 1) buffer[len++] = 'f';
                if (len < bufferSize - 1) buffer[len++] = 'n';
                if (len < bufferSize - 1) buffer[len++] = ' ';
                const FunctionStmt* fn = value.function.declaration;
                if (fn)
                {
                    for (USIZE i = 0; i < fn->nameLength && len < bufferSize - 2; i++)
                    {
                        buffer[len++] = fn->name[i];
                    }
                }
                if (len < bufferSize - 1) buffer[len++] = '>';
                buffer[len] = '\0';
                return len;
            }

        case ValueType::NATIVE_FUNCTION:
        case ValueType::CFUNCTION:
            return String::CopyEmbed("<native>"_embed, buffer, bufferSize);

        case ValueType::ARRAY:
            {
                USIZE len = 0;
                if (len < bufferSize - 1) buffer[len++] = '[';
                ArrayStorage* arr = value.array;
                if (arr)
                {
                    for (UINT8 i = 0; i < arr->count && len < bufferSize - 10; i++)
                    {
                        if (i > 0)
                        {
                            if (len < bufferSize - 1) buffer[len++] = ',';
                            if (len < bufferSize - 1) buffer[len++] = ' ';
                        }
                        CHAR elemBuf[64];
                        USIZE elemLen = ValueToString(arr->elements[i], elemBuf, sizeof(elemBuf));
                        for (USIZE j = 0; j < elemLen && len < bufferSize - 2; j++)
                        {
                            buffer[len++] = elemBuf[j];
                        }
                    }
                }
                if (len < bufferSize - 1) buffer[len++] = ']';
                buffer[len] = '\0';
                return len;
            }

        default:
            buffer[0] = '\0';
            return 0;
    }
}

// ============================================================================
// PRINT FUNCTION - First registered function
// ============================================================================

/**
 * print(value, ...) - Print values to output via Console
 *
 * Usage:
 *   print("Hello");
 *   print("x =", x);
 *   print(1, 2, 3);
 */
NOINLINE Value StdLib_Print(FunctionContext& ctx) noexcept
{
    CHAR buffer[512];

    for (UINT8 i = 0; i < ctx.argCount; i++)
    {
        if (i > 0)
        {
            ctx.state->Write(" "_embed, 1);
        }

        USIZE len = ValueToString(ctx.Arg(i), buffer, sizeof(buffer));
        ctx.state->Write(buffer, len);
    }

    ctx.state->Write("\n"_embed, 1);
    return Value::Nil();
}

// ============================================================================
// LEN FUNCTION
// ============================================================================

/**
 * len(value) - Get length of string or array
 *
 * Usage:
 *   var n = len("hello");  // 5
 *   var n = len([1,2,3]);  // 3
 */
NOINLINE Value StdLib_Len(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1))
    {
        return Value::Number(-1);
    }

    if (ctx.IsString(0))
    {
        return Value::Number((INT64)ctx.ToStringLength(0));
    }

    if (ctx.IsArray(0))
    {
        return Value::Number((INT64)ctx.ToArrayLength(0));
    }

    return Value::Number(-1);
}

// ============================================================================
// STR FUNCTION
// ============================================================================

/**
 * str(value) - Convert value to string
 *
 * Usage:
 *   var s = str(42);  // "42"
 */
NOINLINE Value StdLib_Str(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1))
    {
        return Value::String(""_embed, 0);
    }

    // Strings pass through
    if (ctx.IsString(0))
    {
        return ctx.Arg(0);
    }

    CHAR buffer[256];
    USIZE len = ValueToString(ctx.Arg(0), buffer, sizeof(buffer));
    return Value::String(buffer, len);
}

// ============================================================================
// NUM FUNCTION
// ============================================================================

/**
 * num(value) - Convert value to number
 *
 * Usage:
 *   var n = num("123");  // 123
 *   var n = num(true);   // 1
 */
NOINLINE Value StdLib_Num(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1))
    {
        return Value::Number(0);
    }

    const Value& arg = ctx.Arg(0);

    switch (arg.type)
    {
        case ValueType::NUMBER:
            return arg;

        case ValueType::BOOL:
            return Value::Number(arg.boolValue ? 1 : 0);

        case ValueType::STRING:
            {
                // Use DOUBLE::Parse which handles both integers and floats
                DOUBLE result = DOUBLE::Parse(arg.strValue);
                return Value::Float(result);
            }

        default:
            return Value::Number(0);
    }
}

// ============================================================================
// TYPE FUNCTION
// ============================================================================

/**
 * type(value) - Get type name as string
 *
 * Usage:
 *   var t = type(42);      // "number"
 *   var t = type("hello"); // "string"
 */
NOINLINE Value StdLib_Type(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1))
    {
        return Value::String("unknown"_embed, 7);
    }

    CHAR typeBuffer[16];
    USIZE len = GetValueTypeName(ctx.Arg(0).type, typeBuffer, sizeof(typeBuffer));
    return Value::String(typeBuffer, len);
}

// ============================================================================
// ABS FUNCTION
// ============================================================================

/**
 * abs(number) - Get absolute value
 *
 * Usage:
 *   var n = abs(-5);  // 5
 */
NOINLINE Value StdLib_Abs(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Number(0);
    }

    DOUBLE n = ctx.ToDouble(0);
    DOUBLE zero = DOUBLE(INT32(0));
    if (n < zero)
    {
        return Value::Float(-n);
    }
    return ctx.Arg(0);
}

// ============================================================================
// MIN FUNCTION
// ============================================================================

/**
 * min(a, b) - Get minimum of two numbers
 *
 * Usage:
 *   var n = min(3, 5);  // 3
 */
NOINLINE Value StdLib_Min(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(2) || !ctx.IsNumber(0) || !ctx.IsNumber(1))
    {
        return Value::Number(0);
    }

    DOUBLE a = ctx.ToDouble(0);
    DOUBLE b = ctx.ToDouble(1);
    return Value::Float(a < b ? a : b);
}

// ============================================================================
// MAX FUNCTION
// ============================================================================

/**
 * max(a, b) - Get maximum of two numbers
 *
 * Usage:
 *   var n = max(3, 5);  // 5
 */
NOINLINE Value StdLib_Max(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(2) || !ctx.IsNumber(0) || !ctx.IsNumber(1))
    {
        return Value::Number(0);
    }

    DOUBLE a = ctx.ToDouble(0);
    DOUBLE b = ctx.ToDouble(1);
    return Value::Float(a > b ? a : b);
}

// ============================================================================
// FLOOR FUNCTION
// ============================================================================

/**
 * floor(x) - Round down to nearest integer
 *
 * Usage:
 *   var n = floor(3.7);   // 3
 *   var n = floor(-3.2);  // -4
 */
NOINLINE Value StdLib_Floor(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Number(0);
    }

    DOUBLE d = ctx.ToDouble(0);
    INT64 n = (INT64)d;  // Truncates toward zero
    DOUBLE zero = DOUBLE(INT32(0));

    // floor rounds toward -infinity, truncation rounds toward zero
    // Adjust for negative numbers with fractional parts
    if (d < zero)
    {
        DOUBLE reconstructed = DOUBLE(INT32(n));
        if (d != reconstructed)
        {
            n = n - 1;
        }
    }
    return Value::Number(n);
}

// ============================================================================
// CEIL FUNCTION
// ============================================================================

/**
 * ceil(x) - Round up to nearest integer
 *
 * Usage:
 *   var n = ceil(3.2);   // 4
 *   var n = ceil(-3.7);  // -3
 */
NOINLINE Value StdLib_Ceil(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Number(0);
    }

    DOUBLE d = ctx.ToDouble(0);
    INT64 n = (INT64)d;  // Truncates toward zero
    DOUBLE zero = DOUBLE(INT32(0));

    // ceil rounds toward +infinity
    // Adjust for positive numbers with fractional parts
    if (d > zero)
    {
        DOUBLE reconstructed = DOUBLE(INT32(n));
        if (d != reconstructed)
        {
            n = n + 1;
        }
    }
    return Value::Number(n);
}

// ============================================================================
// INT FUNCTION
// ============================================================================

/**
 * int(x) - Truncate to integer (toward zero)
 *
 * Usage:
 *   var n = int(3.7);   // 3
 *   var n = int(-3.7);  // -3
 */
NOINLINE Value StdLib_Int(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Number(0);
    }

    return Value::Number(ctx.ToNumber(0));
}

// ============================================================================
// ARRAY FUNCTIONS
// ============================================================================

/**
 * push(array, value) - Add element to end of array
 *
 * Usage:
 *   var arr = [1, 2];
 *   push(arr, 3);  // arr is now [1, 2, 3]
 *
 * Returns: new array length, or -1 on error
 */
NOINLINE Value StdLib_Push(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(2) || !ctx.IsArray(0))
    {
        return Value::Number(-1);
    }

    ArrayStorage* arr = ctx.ToArray(0);
    if (!arr || arr->count >= MAX_ARRAY_SIZE)
    {
        return Value::Number(-1);  // Array full
    }

    arr->elements[arr->count++] = ctx.Arg(1);
    return Value::Number((INT64)arr->count);
}

/**
 * pop(array) - Remove and return last element
 *
 * Usage:
 *   var arr = [1, 2, 3];
 *   var last = pop(arr);  // last = 3, arr is now [1, 2]
 *
 * Returns: removed element, or nil on empty array
 */
NOINLINE Value StdLib_Pop(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsArray(0))
    {
        return Value::Nil();
    }

    ArrayStorage* arr = ctx.ToArray(0);
    if (!arr || arr->count == 0)
    {
        return Value::Nil();  // Empty array
    }

    Value last = arr->elements[--arr->count];
    return last;
}

// ============================================================================
// STRING MANIPULATION FUNCTIONS
// ============================================================================

/**
 * substr(str, start [, length]) - Extract substring
 *
 * Usage:
 *   var s = substr("hello", 1, 3);  // "ell"
 *   var s = substr("hello", 2);     // "llo"
 */
NOINLINE Value StdLib_Substr(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgsMin(2) || !ctx.IsString(0) || !ctx.IsNumber(1))
    {
        return Value::String(""_embed, 0);
    }

    const CHAR* str = ctx.ToString(0);
    USIZE strLen = ctx.ToStringLength(0);
    INT64 start = ctx.ToNumber(1);

    // Handle negative start (from end)
    if (start < 0) start = (INT64)strLen + start;
    if (start < 0) start = 0;
    if ((USIZE)start >= strLen) return Value::String(""_embed, 0);

    // Determine length
    USIZE length = strLen - (USIZE)start;
    if (ctx.GetArgCount() >= 3 && ctx.IsNumber(2))
    {
        INT64 reqLen = ctx.ToNumber(2);
        if (reqLen <= 0) return Value::String(""_embed, 0);
        if ((USIZE)reqLen < length) length = (USIZE)reqLen;
    }

    return Value::String(str + start, length);
}

/**
 * indexOf(str, search [, start]) - Find substring position
 *
 * Usage:
 *   var i = indexOf("hello", "ll");    // 2
 *   var i = indexOf("hello", "x");     // -1
 */
NOINLINE Value StdLib_IndexOf(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgsMin(2) || !ctx.IsString(0) || !ctx.IsString(1))
    {
        return Value::Number(-1);
    }

    const CHAR* str = ctx.ToString(0);
    USIZE strLen = ctx.ToStringLength(0);
    const CHAR* search = ctx.ToString(1);
    USIZE searchLen = ctx.ToStringLength(1);

    USIZE start = 0;
    if (ctx.GetArgCount() >= 3 && ctx.IsNumber(2))
    {
        INT64 s = ctx.ToNumber(2);
        if (s > 0) start = (USIZE)s;
    }

    if (start >= strLen) return Value::Number(-1);

    SSIZE result = String::IndexOf(str + start, strLen - start, search, searchLen);
    if (result < 0) return Value::Number(-1);
    return Value::Number((INT64)(start + (USIZE)result));
}

/**
 * trim(str) - Remove leading and trailing whitespace
 *
 * Usage:
 *   var s = trim("  hello  ");  // "hello"
 */
NOINLINE Value StdLib_Trim(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsString(0))
    {
        return Value::String(""_embed, 0);
    }

    const CHAR* str = ctx.ToString(0);
    USIZE len = ctx.ToStringLength(0);

    const CHAR* trimmed = String::Trim(str, len);
    return Value::String(trimmed, len);
}

/**
 * upper(str) - Convert string to uppercase
 *
 * Usage:
 *   var s = upper("hello");  // "HELLO"
 */
NOINLINE Value StdLib_Upper(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsString(0))
    {
        return Value::String(""_embed, 0);
    }

    const CHAR* str = ctx.ToString(0);
    USIZE len = ctx.ToStringLength(0);

    CHAR buffer[MAX_STRING_VALUE];
    USIZE copyLen = len < MAX_STRING_VALUE - 1 ? len : MAX_STRING_VALUE - 1;
    for (USIZE i = 0; i < copyLen; i++)
    {
        buffer[i] = String::ToUpperCase(str[i]);
    }
    buffer[copyLen] = '\0';
    return Value::String(buffer, copyLen);
}

/**
 * lower(str) - Convert string to lowercase
 *
 * Usage:
 *   var s = lower("HELLO");  // "hello"
 */
NOINLINE Value StdLib_Lower(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsString(0))
    {
        return Value::String(""_embed, 0);
    }

    const CHAR* str = ctx.ToString(0);
    USIZE len = ctx.ToStringLength(0);

    CHAR buffer[MAX_STRING_VALUE];
    USIZE copyLen = len < MAX_STRING_VALUE - 1 ? len : MAX_STRING_VALUE - 1;
    for (USIZE i = 0; i < copyLen; i++)
    {
        buffer[i] = String::ToLowerCase(str[i]);
    }
    buffer[copyLen] = '\0';
    return Value::String(buffer, copyLen);
}

/**
 * startsWith(str, prefix) - Check if string starts with prefix
 *
 * Usage:
 *   var b = startsWith("hello", "he");  // true
 */
NOINLINE Value StdLib_StartsWith(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(2) || !ctx.IsString(0) || !ctx.IsString(1))
    {
        return Value::Bool(FALSE);
    }

    const CHAR* str = ctx.ToString(0);
    USIZE strLen = ctx.ToStringLength(0);
    const CHAR* prefix = ctx.ToString(1);
    USIZE prefixLen = ctx.ToStringLength(1);

    return Value::Bool(String::StartsWith(str, strLen, prefix, prefixLen));
}

/**
 * endsWith(str, suffix) - Check if string ends with suffix
 *
 * Usage:
 *   var b = endsWith("hello", "lo");  // true
 */
NOINLINE Value StdLib_EndsWith(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(2) || !ctx.IsString(0) || !ctx.IsString(1))
    {
        return Value::Bool(FALSE);
    }

    const CHAR* str = ctx.ToString(0);
    USIZE strLen = ctx.ToStringLength(0);
    const CHAR* suffix = ctx.ToString(1);
    USIZE suffixLen = ctx.ToStringLength(1);

    return Value::Bool(String::EndsWith(str, strLen, suffix, suffixLen));
}

/**
 * replace(str, search, replacement) - Replace first occurrence
 *
 * Usage:
 *   var s = replace("hello", "l", "L");  // "heLlo"
 */
NOINLINE Value StdLib_Replace(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(3) || !ctx.IsString(0) || !ctx.IsString(1) || !ctx.IsString(2))
    {
        return Value::String(""_embed, 0);
    }

    const CHAR* str = ctx.ToString(0);
    USIZE strLen = ctx.ToStringLength(0);
    const CHAR* search = ctx.ToString(1);
    USIZE searchLen = ctx.ToStringLength(1);
    const CHAR* repl = ctx.ToString(2);
    USIZE replLen = ctx.ToStringLength(2);

    if (searchLen == 0) return Value::String(str, strLen);

    SSIZE pos = String::IndexOf(str, strLen, search, searchLen);
    if (pos < 0) return Value::String(str, strLen);

    CHAR buffer[MAX_STRING_VALUE];
    USIZE len = 0;

    // Copy before match
    for (SSIZE i = 0; i < pos && len < MAX_STRING_VALUE - 1; i++)
    {
        buffer[len++] = str[i];
    }

    // Copy replacement
    for (USIZE i = 0; i < replLen && len < MAX_STRING_VALUE - 1; i++)
    {
        buffer[len++] = repl[i];
    }

    // Copy after match
    for (USIZE i = (USIZE)pos + searchLen; i < strLen && len < MAX_STRING_VALUE - 1; i++)
    {
        buffer[len++] = str[i];
    }

    buffer[len] = '\0';
    return Value::String(buffer, len);
}

/**
 * char(code) - Get character from ASCII code
 *
 * Usage:
 *   var c = char(65);  // "A"
 */
NOINLINE Value StdLib_Char(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::String(""_embed, 0);
    }

    INT64 code = ctx.ToNumber(0);
    if (code < 0 || code > 255) return Value::String(""_embed, 0);

    CHAR buffer[2] = { (CHAR)code, '\0' };
    return Value::String(buffer, 1);
}

/**
 * ord(str) - Get ASCII code of first character
 *
 * Usage:
 *   var n = ord("A");  // 65
 */
NOINLINE Value StdLib_Ord(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsString(0))
    {
        return Value::Number(-1);
    }

    USIZE len = ctx.ToStringLength(0);
    if (len == 0) return Value::Number(-1);

    return Value::Number((INT64)(UINT8)ctx.ToString(0)[0]);
}

// ============================================================================
// ADDITIONAL MATH FUNCTIONS
// ============================================================================

/**
 * round(x) - Round to nearest integer
 *
 * Usage:
 *   var n = round(3.4);   // 3
 *   var n = round(3.5);   // 4
 *   var n = round(-3.5);  // -4
 */
NOINLINE Value StdLib_Round(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Number(0);
    }

    DOUBLE d = ctx.ToDouble(0);
    DOUBLE half = DOUBLE(INT32(5)) / DOUBLE(INT32(10));  // 0.5
    DOUBLE zero = DOUBLE(INT32(0));

    if (d >= zero)
    {
        return Value::Number((INT64)(d + half));
    }
    else
    {
        return Value::Number((INT64)(d - half));
    }
}

/**
 * clamp(value, min, max) - Constrain value to range
 *
 * Usage:
 *   var n = clamp(15, 0, 10);  // 10
 *   var n = clamp(-5, 0, 10);  // 0
 */
NOINLINE Value StdLib_Clamp(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(3) || !ctx.IsNumber(0) || !ctx.IsNumber(1) || !ctx.IsNumber(2))
    {
        return Value::Number(0);
    }

    DOUBLE val = ctx.ToDouble(0);
    DOUBLE minVal = ctx.ToDouble(1);
    DOUBLE maxVal = ctx.ToDouble(2);

    if (val < minVal) return Value::Float(minVal);
    if (val > maxVal) return Value::Float(maxVal);
    return Value::Float(val);
}

/**
 * sign(x) - Get sign of number
 *
 * Usage:
 *   var n = sign(-5);  // -1
 *   var n = sign(0);   // 0
 *   var n = sign(5);   // 1
 */
NOINLINE Value StdLib_Sign(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Number(0);
    }

    DOUBLE d = ctx.ToDouble(0);
    DOUBLE zero = DOUBLE(INT32(0));

    if (d < zero) return Value::Number(-1);
    if (d > zero) return Value::Number(1);
    return Value::Number(0);
}

/**
 * pow(base, exp) - Power function (integer exponent only)
 *
 * Usage:
 *   var n = pow(2, 10);   // 1024
 *   var n = pow(3, 0);    // 1
 */
NOINLINE Value StdLib_Pow(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(2) || !ctx.IsNumber(0) || !ctx.IsNumber(1))
    {
        return Value::Number(0);
    }

    DOUBLE base = ctx.ToDouble(0);
    INT64 exp = ctx.ToNumber(1);

    if (exp == 0) return Value::Number(1);

    BOOL negative = exp < 0;
    if (negative) exp = -exp;

    DOUBLE result = DOUBLE(INT32(1));
    for (INT64 i = 0; i < exp; i++)
    {
        result = result * base;
    }

    if (negative)
    {
        // result = 1 / result (simple inverse)
        DOUBLE one = DOUBLE(INT32(1));
        result = one / result;
    }

    return Value::Float(result);
}

/**
 * sqrt(x) - Square root using Newton-Raphson
 *
 * Usage:
 *   var n = sqrt(16);   // 4
 *   var n = sqrt(2);    // 1.414...
 */
NOINLINE Value StdLib_Sqrt(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Number(0);
    }

    DOUBLE x = ctx.ToDouble(0);
    DOUBLE zero = DOUBLE(INT32(0));

    if (x < zero) return Value::Float(zero);  // Negative -> return 0 (no NaN)
    if (x == zero) return Value::Float(zero);

    // Newton-Raphson: x_{n+1} = (x_n + S/x_n) / 2
    DOUBLE two = DOUBLE(INT32(2));
    DOUBLE guess = x / two;  // Initial guess
    DOUBLE epsilon = DOUBLE(INT32(1)) / DOUBLE(INT32(1000000));  // 0.000001 precision

    for (INT32 i = 0; i < 20; i++)  // Max 20 iterations
    {
        DOUBLE newGuess = (guess + x / guess) / two;
        DOUBLE diff = newGuess - guess;
        if (diff < zero) diff = -diff;
        if (diff < epsilon) break;
        guess = newGuess;
    }

    return Value::Float(guess);
}

// ============================================================================
// ADDITIONAL ARRAY FUNCTIONS
// ============================================================================

/**
 * contains(array, value) - Check if array contains value
 *
 * Usage:
 *   var b = contains([1, 2, 3], 2);  // true
 */
NOINLINE Value StdLib_Contains(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(2) || !ctx.IsArray(0))
    {
        return Value::Bool(FALSE);
    }

    ArrayStorage* arr = ctx.ToArray(0);
    if (!arr) return Value::Bool(FALSE);

    const Value& search = ctx.Arg(1);
    for (UINT8 i = 0; i < arr->count; i++)
    {
        if (arr->elements[i].Equals(search))
        {
            return Value::Bool(TRUE);
        }
    }
    return Value::Bool(FALSE);
}

/**
 * reverse(array) - Reverse array in place
 *
 * Usage:
 *   var arr = [1, 2, 3];
 *   reverse(arr);  // arr is now [3, 2, 1]
 */
NOINLINE Value StdLib_Reverse(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsArray(0))
    {
        return Value::Nil();
    }

    ArrayStorage* arr = ctx.ToArray(0);
    if (!arr || arr->count < 2) return Value::Nil();

    UINT8 left = 0;
    UINT8 right = arr->count - 1;
    while (left < right)
    {
        Value temp = arr->elements[left];
        arr->elements[left] = arr->elements[right];
        arr->elements[right] = temp;
        left++;
        right--;
    }
    return Value::Nil();
}

// ============================================================================
// OPEN STANDARD LIBRARY
// ============================================================================

/**
 * Register all standard library functions with a State.
 *
 * Core functions:
 *   print, len, str, num, type
 *
 * Math functions:
 *   abs, min, max, floor, ceil, int, round, clamp, sign, pow, sqrt
 *
 * String functions:
 *   substr, indexOf, trim, upper, lower, startsWith, endsWith, replace, char, ord
 *
 * Array functions:
 *   push, pop, contains, reverse
 */
NOINLINE void OpenStdLib(State& L) noexcept
{
    // Core functions
    L.Register("print"_embed, EMBED_FUNC(StdLib_Print));
    L.Register("len"_embed, EMBED_FUNC(StdLib_Len));
    L.Register("str"_embed, EMBED_FUNC(StdLib_Str));
    L.Register("num"_embed, EMBED_FUNC(StdLib_Num));
    L.Register("type"_embed, EMBED_FUNC(StdLib_Type));

    // Math functions
    L.Register("abs"_embed, EMBED_FUNC(StdLib_Abs));
    L.Register("min"_embed, EMBED_FUNC(StdLib_Min));
    L.Register("max"_embed, EMBED_FUNC(StdLib_Max));
    L.Register("floor"_embed, EMBED_FUNC(StdLib_Floor));
    L.Register("ceil"_embed, EMBED_FUNC(StdLib_Ceil));
    L.Register("int"_embed, EMBED_FUNC(StdLib_Int));
    L.Register("round"_embed, EMBED_FUNC(StdLib_Round));
    L.Register("clamp"_embed, EMBED_FUNC(StdLib_Clamp));
    L.Register("sign"_embed, EMBED_FUNC(StdLib_Sign));
    L.Register("pow"_embed, EMBED_FUNC(StdLib_Pow));
    L.Register("sqrt"_embed, EMBED_FUNC(StdLib_Sqrt));

    // String functions
    L.Register("substr"_embed, EMBED_FUNC(StdLib_Substr));
    L.Register("indexOf"_embed, EMBED_FUNC(StdLib_IndexOf));
    L.Register("trim"_embed, EMBED_FUNC(StdLib_Trim));
    L.Register("upper"_embed, EMBED_FUNC(StdLib_Upper));
    L.Register("lower"_embed, EMBED_FUNC(StdLib_Lower));
    L.Register("startsWith"_embed, EMBED_FUNC(StdLib_StartsWith));
    L.Register("endsWith"_embed, EMBED_FUNC(StdLib_EndsWith));
    L.Register("replace"_embed, EMBED_FUNC(StdLib_Replace));
    L.Register("char"_embed, EMBED_FUNC(StdLib_Char));
    L.Register("ord"_embed, EMBED_FUNC(StdLib_Ord));

    // Array functions
    L.Register("push"_embed, EMBED_FUNC(StdLib_Push));
    L.Register("pop"_embed, EMBED_FUNC(StdLib_Pop));
    L.Register("contains"_embed, EMBED_FUNC(StdLib_Contains));
    L.Register("reverse"_embed, EMBED_FUNC(StdLib_Reverse));
}

} // namespace PIL
