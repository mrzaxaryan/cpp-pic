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
                INT64 n = value.numberValue;
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
 * len(string) - Get string length
 *
 * Usage:
 *   var n = len("hello");  // 5
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
                // Parse string to number
                INT64 result = 0;
                INT32 sign = 1;
                USIZE i = 0;

                // Skip whitespace
                while (i < arg.strLength && arg.strValue[i] == ' ')
                {
                    i++;
                }

                // Check sign
                if (i < arg.strLength && arg.strValue[i] == '-')
                {
                    sign = -1;
                    i++;
                }
                else if (i < arg.strLength && arg.strValue[i] == '+')
                {
                    i++;
                }

                // Parse digits
                while (i < arg.strLength && arg.strValue[i] >= '0' && arg.strValue[i] <= '9')
                {
                    result = result * 10 + (arg.strValue[i] - '0');
                    i++;
                }

                return Value::Number(sign < 0 ? -result : result);
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

    INT64 n = ctx.ToNumber(0);
    return Value::Number(n < 0 ? -n : n);
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

    INT64 a = ctx.ToNumber(0);
    INT64 b = ctx.ToNumber(1);
    return Value::Number(a < b ? a : b);
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

    INT64 a = ctx.ToNumber(0);
    INT64 b = ctx.ToNumber(1);
    return Value::Number(a > b ? a : b);
}

// ============================================================================
// OPEN STANDARD LIBRARY
// ============================================================================

/**
 * Register all standard library functions with a State.
 *
 * Functions registered (in order):
 *   1. print  - Print values to output
 *   2. len    - Get string length
 *   3. str    - Convert to string
 *   4. num    - Convert to number
 *   5. type   - Get type name
 *   6. abs    - Absolute value
 *   7. min    - Minimum of two numbers
 *   8. max    - Maximum of two numbers
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
}

} // namespace script
