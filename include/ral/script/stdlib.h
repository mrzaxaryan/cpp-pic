/**
 * stdlib.h - Standard Library for PICScript (Lua-like API)
 *
 * Native functions using the new CFunction API.
 * print() outputs directly to Console (no callback needed).
 *
 * Position-independent, no .rdata dependencies.
 *
 * Part of RAL (Runtime Abstraction Layer).
 *
 * USAGE:
 *   script::State L;
 *   script::OpenStdLib(L);  // Registers print, len, str, num, type
 *   L.DoString("print(\"Hello!\");");
 */

#pragma once

#include "value.h"
#include "pal/io/console.h"

// Forward declaration - State is defined in state.h
namespace script { class State; }

namespace script
{

// ============================================================================
// VALUE TO STRING HELPER
// ============================================================================

NOINLINE USIZE ValueToString(const Value& value, CHAR* buffer, USIZE bufferSize) noexcept
{
    USIZE len = 0;

    switch (value.type)
    {
        case ValueType::NIL:
            if (bufferSize >= 4)
            {
                buffer[0] = 'n'; buffer[1] = 'i'; buffer[2] = 'l';
                len = 3;
            }
            break;

        case ValueType::BOOL:
            if (value.boolValue)
            {
                if (bufferSize >= 5)
                {
                    buffer[0] = 't'; buffer[1] = 'r'; buffer[2] = 'u'; buffer[3] = 'e';
                    len = 4;
                }
            }
            else
            {
                if (bufferSize >= 6)
                {
                    buffer[0] = 'f'; buffer[1] = 'a'; buffer[2] = 'l';
                    buffer[3] = 's'; buffer[4] = 'e';
                    len = 5;
                }
            }
            break;

        case ValueType::NUMBER:
            {
                DOUBLE d = value.numberValue;
                DOUBLE zero = DOUBLE(INT32(0));

                // Check if it's effectively an integer
                INT64 intPart = (INT64)d;
                DOUBLE reconstructed = DOUBLE(INT32(intPart));

                if (d == reconstructed)
                {
                    // Format as integer
                    INT64 n = intPart;
                    BOOL negative = FALSE;
                    if (n < 0)
                    {
                        negative = TRUE;
                        n = -n;
                    }

                    // Build number string in reverse
                    CHAR temp[32];
                    USIZE tempLen = 0;
                    if (n == 0)
                    {
                        temp[tempLen++] = '0';
                    }
                    else
                    {
                        while (n > 0 && tempLen < 31)
                        {
                            temp[tempLen++] = '0' + (n % 10);
                            n /= 10;
                        }
                    }

                    // Add sign and reverse
                    if (negative && len < bufferSize - 1)
                    {
                        buffer[len++] = '-';
                    }
                    for (SSIZE i = (SSIZE)tempLen - 1; i >= 0 && len < bufferSize - 1; i--)
                    {
                        buffer[len++] = temp[i];
                    }
                }
                else
                {
                    // Format as float
                    BOOL negative = d < zero;
                    if (negative) d = -d;

                    INT64 wholePart = (INT64)d;
                    DOUBLE fracPart = d - DOUBLE(INT32(wholePart));

                    // Format whole part
                    CHAR temp[32];
                    USIZE tempLen = 0;
                    if (wholePart == 0)
                    {
                        temp[tempLen++] = '0';
                    }
                    else
                    {
                        INT64 w = wholePart;
                        while (w > 0 && tempLen < 31)
                        {
                            temp[tempLen++] = '0' + (w % 10);
                            w /= 10;
                        }
                    }

                    if (negative && len < bufferSize - 1)
                    {
                        buffer[len++] = '-';
                    }
                    for (SSIZE i = (SSIZE)tempLen - 1; i >= 0 && len < bufferSize - 1; i--)
                    {
                        buffer[len++] = temp[i];
                    }

                    // Add decimal point
                    if (len < bufferSize - 1) buffer[len++] = '.';

                    // Get up to 6 decimal digits
                    DOUBLE ten = DOUBLE(INT32(10));
                    USIZE decStart = len;
                    for (INT32 digit = 0; digit < 6 && len < bufferSize - 1; digit++)
                    {
                        fracPart = fracPart * ten;
                        INT32 dig = (INT32)fracPart;
                        buffer[len++] = '0' + dig;
                        fracPart = fracPart - DOUBLE(dig);
                    }

                    // Remove trailing zeros (but keep at least one decimal digit)
                    while (len > decStart + 1 && buffer[len - 1] == '0')
                    {
                        len--;
                    }
                }
            }
            break;

        case ValueType::STRING:
            for (USIZE i = 0; i < value.strLength && len < bufferSize - 1; i++)
            {
                buffer[len++] = value.strValue[i];
            }
            break;

        case ValueType::FUNCTION:
            {
                // <fn name>
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
            }
            break;

        case ValueType::NATIVE_FUNCTION:
        case ValueType::CFUNCTION:
            if (bufferSize >= 9)
            {
                buffer[0] = '<'; buffer[1] = 'n'; buffer[2] = 'a';
                buffer[3] = 't'; buffer[4] = 'i'; buffer[5] = 'v';
                buffer[6] = 'e'; buffer[7] = '>';
                len = 8;
            }
            break;

        case ValueType::ARRAY:
            {
                // Format as [elem1, elem2, ...]
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
                        // Recursively format element (with reduced buffer)
                        CHAR elemBuf[64];
                        USIZE elemLen = ValueToString(arr->elements[i], elemBuf, sizeof(elemBuf));
                        for (USIZE j = 0; j < elemLen && len < bufferSize - 2; j++)
                        {
                            buffer[len++] = elemBuf[j];
                        }
                    }
                }
                if (len < bufferSize - 1) buffer[len++] = ']';
            }
            break;

        default:
            break;
    }

    if (len < bufferSize)
    {
        buffer[len] = '\0';
    }
    return len;
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
            Console::Write<CHAR>(" "_embed);
        }

        USIZE len = ValueToString(ctx.Arg(i), buffer, sizeof(buffer));
        Console::Write(buffer, len);
    }

    Console::Write<CHAR>("\n"_embed);
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
// OPEN STANDARD LIBRARY
// ============================================================================

/**
 * Register all standard library functions with a State.
 *
 * Functions registered (in order):
 *   1. print  - Print values to output
 *   2. len    - Get string/array length
 *   3. str    - Convert to string
 *   4. num    - Convert to number
 *   5. type   - Get type name
 *   6. abs    - Absolute value
 *   7. min    - Minimum of two numbers
 *   8. max    - Maximum of two numbers
 *   9. floor  - Round down to nearest integer
 *  10. ceil   - Round up to nearest integer
 *  11. int    - Truncate to integer (toward zero)
 *  12. push   - Add element to end of array
 *  13. pop    - Remove and return last element of array
 */
NOINLINE void OpenStdLib(State& L) noexcept
{
    // Register standard library functions (PIC-safe with _embed)
    L.Register("print"_embed, EMBED_FUNC(StdLib_Print));
    L.Register("len"_embed, EMBED_FUNC(StdLib_Len));
    L.Register("str"_embed, EMBED_FUNC(StdLib_Str));
    L.Register("num"_embed, EMBED_FUNC(StdLib_Num));
    L.Register("type"_embed, EMBED_FUNC(StdLib_Type));
    L.Register("abs"_embed, EMBED_FUNC(StdLib_Abs));
    L.Register("min"_embed, EMBED_FUNC(StdLib_Min));
    L.Register("max"_embed, EMBED_FUNC(StdLib_Max));
    L.Register("floor"_embed, EMBED_FUNC(StdLib_Floor));
    L.Register("ceil"_embed, EMBED_FUNC(StdLib_Ceil));
    L.Register("int"_embed, EMBED_FUNC(StdLib_Int));
    L.Register("push"_embed, EMBED_FUNC(StdLib_Push));
    L.Register("pop"_embed, EMBED_FUNC(StdLib_Pop));
}

} // namespace script
