/**
 * value.h - Value System for PICScript
 *
 * Runtime value representation with stack-based storage.
 * Position-independent, no .rdata dependencies.
 *
 * Part of RAL (Runtime Abstraction Layer).
 */

#pragma once

#include "ast.h"

namespace script
{

// ============================================================================
// VALUE TYPES
// ============================================================================

enum class ValueType : UINT8
{
    NIL,
    BOOL,
    NUMBER,
    STRING,
    FUNCTION,
    NATIVE_FUNCTION,
    CFUNCTION,          // C++ function with state (Lua-like)
};

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

struct Value;
class Environment;
struct FunctionContext;
class State;

// ============================================================================
// NATIVE FUNCTION TYPES
// ============================================================================

// Legacy native function signature (for backwards compatibility)
typedef Value (*NativeFn)(Value* args, UINT8 argCount, Environment* env);

// New C++ function signature with context (Lua-like)
typedef Value (*CFunction)(FunctionContext& ctx);

// ============================================================================
// CFUNCTION VALUE (stores function + state pointer)
// ============================================================================

struct CFunctionValue
{
    CFunction func;
    State* state;
};

// ============================================================================
// FUNCTION VALUE
// ============================================================================

struct FunctionValue
{
    const FunctionStmt* declaration;
    Environment* closure;
};

// ============================================================================
// VALUE STRUCT
// ============================================================================

struct Value
{
    ValueType type;

    union
    {
        BOOL boolValue;
        INT64 numberValue;
        struct
        {
            CHAR strValue[MAX_STRING_VALUE];
            USIZE strLength;
        };
        FunctionValue function;
        NativeFn nativeFn;
        CFunctionValue cfunction;
    };

    // Default constructor - nil
    Value() noexcept : type(ValueType::NIL) {}

    // Bool constructor
    static Value Bool(BOOL b) noexcept
    {
        Value v;
        v.type = ValueType::BOOL;
        v.boolValue = b;
        return v;
    }

    // Number constructor
    static Value Number(INT64 n) noexcept
    {
        Value v;
        v.type = ValueType::NUMBER;
        v.numberValue = n;
        return v;
    }

    // String constructor
    static Value String(const CHAR* s, USIZE len) noexcept
    {
        Value v;
        v.type = ValueType::STRING;
        USIZE copyLen = len < MAX_STRING_VALUE - 1 ? len : MAX_STRING_VALUE - 1;
        for (USIZE i = 0; i < copyLen; i++)
        {
            v.strValue[i] = s[i];
        }
        v.strValue[copyLen] = '\0';
        v.strLength = copyLen;
        return v;
    }

    // Function constructor
    static Value Function(const FunctionStmt* decl, Environment* closure) noexcept
    {
        Value v;
        v.type = ValueType::FUNCTION;
        v.function.declaration = decl;
        v.function.closure = closure;
        return v;
    }

    // Native function constructor (legacy)
    static Value NativeFunction(NativeFn fn) noexcept
    {
        Value v;
        v.type = ValueType::NATIVE_FUNCTION;
        v.nativeFn = fn;
        return v;
    }

    // CFunction constructor (Lua-like, with state)
    static Value CFunc(CFunction fn, State* state) noexcept
    {
        Value v;
        v.type = ValueType::CFUNCTION;
        v.cfunction.func = fn;
        v.cfunction.state = state;
        return v;
    }

    // Nil constructor
    static Value Nil() noexcept
    {
        return Value();
    }

    // Type checks
    FORCE_INLINE BOOL IsNil() const noexcept { return type == ValueType::NIL; }
    FORCE_INLINE BOOL IsBool() const noexcept { return type == ValueType::BOOL; }
    FORCE_INLINE BOOL IsNumber() const noexcept { return type == ValueType::NUMBER; }
    FORCE_INLINE BOOL IsString() const noexcept { return type == ValueType::STRING; }
    FORCE_INLINE BOOL IsFunction() const noexcept { return type == ValueType::FUNCTION; }
    FORCE_INLINE BOOL IsNativeFunction() const noexcept { return type == ValueType::NATIVE_FUNCTION; }
    FORCE_INLINE BOOL IsCFunction() const noexcept { return type == ValueType::CFUNCTION; }
    FORCE_INLINE BOOL IsCallable() const noexcept { return IsFunction() || IsNativeFunction() || IsCFunction(); }

    // Truthiness: nil and false are falsy, everything else is truthy
    FORCE_INLINE BOOL IsTruthy() const noexcept
    {
        if (type == ValueType::NIL) return FALSE;
        if (type == ValueType::BOOL) return boolValue;
        return TRUE;
    }

    // Equality
    NOINLINE BOOL Equals(const Value& other) const noexcept
    {
        if (type != other.type) return FALSE;

        switch (type)
        {
            case ValueType::NIL:    return TRUE;
            case ValueType::BOOL:   return boolValue == other.boolValue;
            case ValueType::NUMBER: return numberValue == other.numberValue;
            case ValueType::STRING:
                if (strLength != other.strLength) return FALSE;
                for (USIZE i = 0; i < strLength; i++)
                {
                    if (strValue[i] != other.strValue[i]) return FALSE;
                }
                return TRUE;
            case ValueType::FUNCTION:
                return function.declaration == other.function.declaration;
            case ValueType::NATIVE_FUNCTION:
                return nativeFn == other.nativeFn;
            case ValueType::CFUNCTION:
                return cfunction.func == other.cfunction.func &&
                       cfunction.state == other.cfunction.state;
            default:
                return FALSE;
        }
    }
};

// ============================================================================
// ENVIRONMENT (Variable Scope) - Hash-optimized
// ============================================================================

constexpr USIZE MAX_VARIABLES = 64;
constexpr USIZE MAX_SCOPE_DEPTH = 32;

// DJB2 hash for fast variable lookup
FORCE_INLINE UINT32 HashName(const CHAR* name, USIZE len) noexcept
{
    UINT32 hash = 5381;
    for (USIZE i = 0; i < len; i++)
    {
        hash = ((hash << 5) + hash) + (UINT8)name[i];
    }
    return hash;
}

// String comparison helper (reduces code duplication)
FORCE_INLINE BOOL StrEquals(const CHAR* a, const CHAR* b, USIZE len) noexcept
{
    for (USIZE i = 0; i < len; i++)
    {
        if (a[i] != b[i]) return FALSE;
    }
    return TRUE;
}

struct Variable
{
    UINT32 hash;                        // Pre-computed hash for O(1) lookup
    USIZE nameLength;
    CHAR name[MAX_IDENTIFIER_LENGTH];
    Value value;
};

struct Scope
{
    Variable variables[MAX_VARIABLES];
    USIZE count;

    Scope() noexcept : count(0) {}
};

class Environment
{
private:
    Scope m_scopes[MAX_SCOPE_DEPTH];
    USIZE m_depth;

    // Find variable in scope by hash (fast path)
    FORCE_INLINE Variable* FindInScope(Scope& scope, UINT32 hash, const CHAR* name, USIZE nameLen) noexcept
    {
        for (USIZE i = 0; i < scope.count; i++)
        {
            if (scope.variables[i].hash == hash &&
                scope.variables[i].nameLength == nameLen &&
                StrEquals(scope.variables[i].name, name, nameLen))
            {
                return &scope.variables[i];
            }
        }
        return nullptr;
    }

    FORCE_INLINE const Variable* FindInScope(const Scope& scope, UINT32 hash, const CHAR* name, USIZE nameLen) const noexcept
    {
        for (USIZE i = 0; i < scope.count; i++)
        {
            if (scope.variables[i].hash == hash &&
                scope.variables[i].nameLength == nameLen &&
                StrEquals(scope.variables[i].name, name, nameLen))
            {
                return &scope.variables[i];
            }
        }
        return nullptr;
    }

public:
    Environment() noexcept : m_depth(1) {}

    FORCE_INLINE BOOL PushScope() noexcept
    {
        if (m_depth >= MAX_SCOPE_DEPTH) return FALSE;
        m_scopes[m_depth++].count = 0;
        return TRUE;
    }

    FORCE_INLINE void PopScope() noexcept
    {
        if (m_depth > 1) m_depth--;
    }

    NOINLINE BOOL Define(const CHAR* name, USIZE nameLen, const Value& value) noexcept
    {
        if (m_depth == 0) return FALSE;

        Scope& scope = m_scopes[m_depth - 1];
        UINT32 hash = HashName(name, nameLen);

        // Check if already defined
        if (Variable* var = FindInScope(scope, hash, name, nameLen))
        {
            var->value = value;
            return TRUE;
        }

        if (scope.count >= MAX_VARIABLES) return FALSE;

        // Add new variable
        Variable& var = scope.variables[scope.count++];
        var.hash = hash;
        var.nameLength = nameLen < MAX_IDENTIFIER_LENGTH - 1 ? nameLen : MAX_IDENTIFIER_LENGTH - 1;
        for (USIZE i = 0; i < var.nameLength; i++)
        {
            var.name[i] = name[i];
        }
        var.name[var.nameLength] = '\0';
        var.value = value;
        return TRUE;
    }

    NOINLINE BOOL Assign(const CHAR* name, USIZE nameLen, const Value& value) noexcept
    {
        UINT32 hash = HashName(name, nameLen);
        for (SSIZE d = (SSIZE)m_depth - 1; d >= 0; d--)
        {
            if (Variable* var = FindInScope(m_scopes[d], hash, name, nameLen))
            {
                var->value = value;
                return TRUE;
            }
        }
        return FALSE;
    }

    NOINLINE BOOL Get(const CHAR* name, USIZE nameLen, Value& outValue) const noexcept
    {
        UINT32 hash = HashName(name, nameLen);
        for (SSIZE d = (SSIZE)m_depth - 1; d >= 0; d--)
        {
            if (const Variable* var = FindInScope(m_scopes[d], hash, name, nameLen))
            {
                outValue = var->value;
                return TRUE;
            }
        }
        return FALSE;
    }

    FORCE_INLINE USIZE GetDepth() const noexcept { return m_depth; }
};

// ============================================================================
// VALUE HELPERS
// ============================================================================

// Helper to copy embedded string to buffer
template<typename T>
FORCE_INLINE USIZE CopyValueTypeToBuffer(const T& src, CHAR* buffer, USIZE bufferSize) noexcept
{
    USIZE len = 0;
    const CHAR* s = src;
    while (s[len] != '\0' && len < bufferSize - 1)
    {
        buffer[len] = s[len];
        len++;
    }
    buffer[len] = '\0';
    return len;
}

// Get type name as string (PIC-safe, writes to buffer)
// Buffer should be at least 16 bytes
// Returns the length of the string written
NOINLINE USIZE GetValueTypeName(ValueType type, CHAR* buffer, USIZE bufferSize) noexcept
{
    if (!buffer || bufferSize < 2) return 0;

    switch (type)
    {
        case ValueType::NIL:             return CopyValueTypeToBuffer("nil"_embed, buffer, bufferSize);
        case ValueType::BOOL:            return CopyValueTypeToBuffer("bool"_embed, buffer, bufferSize);
        case ValueType::NUMBER:          return CopyValueTypeToBuffer("number"_embed, buffer, bufferSize);
        case ValueType::STRING:          return CopyValueTypeToBuffer("string"_embed, buffer, bufferSize);
        case ValueType::FUNCTION:        return CopyValueTypeToBuffer("function"_embed, buffer, bufferSize);
        case ValueType::NATIVE_FUNCTION: return CopyValueTypeToBuffer("native"_embed, buffer, bufferSize);
        case ValueType::CFUNCTION:       return CopyValueTypeToBuffer("cfunction"_embed, buffer, bufferSize);
        default:                         return CopyValueTypeToBuffer("unknown"_embed, buffer, bufferSize);
    }
}

// ============================================================================
// FUNCTION CONTEXT (for CFunction calls)
// ============================================================================

/**
 * Context passed to C++ functions registered with State.
 * Similar to lua_State* in Lua.
 */
struct FunctionContext
{
    State* state;
    Value* args;
    UINT8 argCount;

    // Get argument count
    FORCE_INLINE UINT8 GetArgCount() const noexcept { return argCount; }

    // Check argument count
    FORCE_INLINE BOOL CheckArgs(UINT8 expected) const noexcept
    {
        return argCount == expected;
    }

    FORCE_INLINE BOOL CheckArgsMin(UINT8 minExpected) const noexcept
    {
        return argCount >= minExpected;
    }

    // Get argument by index (0-based)
    FORCE_INLINE Value& Arg(UINT8 index) noexcept
    {
        return args[index];
    }

    FORCE_INLINE const Value& Arg(UINT8 index) const noexcept
    {
        return args[index];
    }

    // Type-checked argument getters
    FORCE_INLINE BOOL IsNumber(UINT8 index) const noexcept
    {
        return index < argCount && args[index].IsNumber();
    }

    FORCE_INLINE BOOL IsString(UINT8 index) const noexcept
    {
        return index < argCount && args[index].IsString();
    }

    FORCE_INLINE BOOL IsBool(UINT8 index) const noexcept
    {
        return index < argCount && args[index].IsBool();
    }

    FORCE_INLINE BOOL IsNil(UINT8 index) const noexcept
    {
        return index < argCount && args[index].IsNil();
    }

    FORCE_INLINE INT64 ToNumber(UINT8 index) const noexcept
    {
        return index < argCount ? args[index].numberValue : 0;
    }

    FORCE_INLINE const CHAR* ToString(UINT8 index) const noexcept
    {
        // Returns nullptr for invalid index (caller should check IsString() first)
        return index < argCount && args[index].IsString() ? args[index].strValue : nullptr;
    }

    FORCE_INLINE USIZE ToStringLength(UINT8 index) const noexcept
    {
        return index < argCount && args[index].IsString() ? args[index].strLength : 0;
    }

    FORCE_INLINE BOOL ToBool(UINT8 index) const noexcept
    {
        return index < argCount ? args[index].IsTruthy() : FALSE;
    }
};

} // namespace script
