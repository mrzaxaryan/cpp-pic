/**
 * value.h - Value System for PIL (Position Independent Language)
 *
 * Runtime value representation with stack-based storage.
 * Position-independent, no .rdata dependencies.
 *
 * Part of RAL (Runtime Abstraction Layer).
 */

#pragma once

#include "ast.h"      // includes core/types/numeric/double.h and core/string/string.h

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
    ARRAY,              // Fixed-size array
    FUNCTION,
    NATIVE_FUNCTION,
    CFUNCTION,          // C++ function with state
};

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

struct Value;
class Environment;
struct FunctionContext;
class State;
struct ArrayStorage;  // Forward declaration for array storage

// ============================================================================
// NATIVE FUNCTION TYPES
// ============================================================================

// Legacy native function signature (for backwards compatibility)
typedef Value (*NativeFn)(Value* args, UINT8 argCount, Environment* env);

// C++ function signature with context
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
        DOUBLE numberValue;    // Changed from INT64 to DOUBLE for float support
        struct
        {
            CHAR strValue[MAX_STRING_VALUE];
            USIZE strLength;
        };
        FunctionValue function;
        NativeFn nativeFn;
        CFunctionValue cfunction;
        ArrayStorage* array;     // Pointer to array storage (lives in pool)
    };

    // Default constructor - nil
    Value() noexcept : type(ValueType::NIL) {}

    // Copy constructor
    Value(const Value& other) noexcept : type(other.type)
    {
        switch (type)
        {
            case ValueType::BOOL:
                boolValue = other.boolValue;
                break;
            case ValueType::NUMBER:
                numberValue = other.numberValue;
                break;
            case ValueType::STRING:
                strLength = other.strLength;
                for (USIZE i = 0; i < strLength && i < MAX_STRING_VALUE; i++)
                {
                    strValue[i] = other.strValue[i];
                }
                if (strLength < MAX_STRING_VALUE) strValue[strLength] = '\0';
                break;
            case ValueType::FUNCTION:
                function = other.function;
                break;
            case ValueType::NATIVE_FUNCTION:
                nativeFn = other.nativeFn;
                break;
            case ValueType::CFUNCTION:
                cfunction = other.cfunction;
                break;
            case ValueType::ARRAY:
                array = other.array;
                break;
            default:
                break;
        }
    }

    // Copy assignment operator
    Value& operator=(const Value& other) noexcept
    {
        if (this != &other)
        {
            type = other.type;
            switch (type)
            {
                case ValueType::BOOL:
                    boolValue = other.boolValue;
                    break;
                case ValueType::NUMBER:
                    numberValue = other.numberValue;
                    break;
                case ValueType::STRING:
                    strLength = other.strLength;
                    for (USIZE i = 0; i < strLength && i < MAX_STRING_VALUE; i++)
                    {
                        strValue[i] = other.strValue[i];
                    }
                    if (strLength < MAX_STRING_VALUE) strValue[strLength] = '\0';
                    break;
                case ValueType::FUNCTION:
                    function = other.function;
                    break;
                case ValueType::NATIVE_FUNCTION:
                    nativeFn = other.nativeFn;
                    break;
                case ValueType::CFUNCTION:
                    cfunction = other.cfunction;
                    break;
                case ValueType::ARRAY:
                    array = other.array;
                    break;
                default:
                    break;
            }
        }
        return *this;
    }

    // Bool constructor
    static Value Bool(BOOL b) noexcept
    {
        Value v;
        v.type = ValueType::BOOL;
        v.boolValue = b;
        return v;
    }

    // Number constructor (integer - converts to DOUBLE internally)
    static Value Number(INT64 n) noexcept
    {
        Value v;
        v.type = ValueType::NUMBER;
        // Use INT64 constructor (PIC-safe, no native double)
        // May lose precision for values > 2^53
        v.numberValue = DOUBLE(n);
        return v;
    }

    // Float constructor (DOUBLE - direct storage)
    static Value Float(DOUBLE d) noexcept
    {
        Value v;
        v.type = ValueType::NUMBER;
        v.numberValue = d;
        return v;
    }

    // String constructor
    static Value String(const CHAR* s, USIZE len) noexcept
    {
        Value v;
        v.type = ValueType::STRING;
        v.strLength = String::Copy(v.strValue, MAX_STRING_VALUE, s, len);
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

    // CFunction constructor (with state)
    static Value CFunc(CFunction fn, State* state) noexcept
    {
        Value v;
        v.type = ValueType::CFUNCTION;
        v.cfunction.func = fn;
        v.cfunction.state = state;
        return v;
    }

    // Array constructor (takes pointer to ArrayStorage from pool)
    static Value Array(ArrayStorage* storage) noexcept
    {
        Value v;
        v.type = ValueType::ARRAY;
        v.array = storage;
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
    FORCE_INLINE BOOL IsArray() const noexcept { return type == ValueType::ARRAY; }
    FORCE_INLINE BOOL IsCallable() const noexcept { return IsFunction() || IsNativeFunction() || IsCFunction(); }

    // Check if number is effectively an integer (no fractional part)
    NOINLINE BOOL IsInteger() const noexcept
    {
        if (type != ValueType::NUMBER) return FALSE;
        INT64 intPart = (INT64)numberValue;
        DOUBLE reconstructed = DOUBLE(INT32(intPart));
        return numberValue == reconstructed;
    }

    // Get as INT64 (truncates toward zero)
    FORCE_INLINE INT64 AsInt() const noexcept
    {
        return (INT64)numberValue;
    }

    // Get as DOUBLE
    FORCE_INLINE DOUBLE AsDouble() const noexcept
    {
        return numberValue;
    }

    // Safe getters with default values
    FORCE_INLINE DOUBLE AsDoubleOr(DOUBLE defaultVal) const noexcept
    {
        return type == ValueType::NUMBER ? numberValue : defaultVal;
    }

    FORCE_INLINE INT64 AsIntOr(INT64 defaultVal) const noexcept
    {
        return type == ValueType::NUMBER ? (INT64)numberValue : defaultVal;
    }

    FORCE_INLINE const CHAR* AsStringOr(const CHAR* defaultVal) const noexcept
    {
        return type == ValueType::STRING ? strValue : defaultVal;
    }

    FORCE_INLINE BOOL AsBoolOr(BOOL defaultVal) const noexcept
    {
        return type == ValueType::BOOL ? boolValue : defaultVal;
    }

    // TryGet methods (returns TRUE if successful)
    FORCE_INLINE BOOL TryGetNumber(DOUBLE& out) const noexcept
    {
        if (type != ValueType::NUMBER) return FALSE;
        out = numberValue;
        return TRUE;
    }

    FORCE_INLINE BOOL TryGetInt(INT64& out) const noexcept
    {
        if (type != ValueType::NUMBER) return FALSE;
        out = (INT64)numberValue;
        return TRUE;
    }

    FORCE_INLINE BOOL TryGetString(const CHAR*& out, USIZE& len) const noexcept
    {
        if (type != ValueType::STRING) return FALSE;
        out = strValue;
        len = strLength;
        return TRUE;
    }

    FORCE_INLINE BOOL TryGetBool(BOOL& out) const noexcept
    {
        if (type != ValueType::BOOL) return FALSE;
        out = boolValue;
        return TRUE;
    }

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
            case ValueType::ARRAY:
                // Arrays are equal if they point to same storage
                // (deep equality would require forward declaration complexity)
                return array == other.array;
            default:
                return FALSE;
        }
    }
};

// ============================================================================
// ARRAY STORAGE AND POOL
// ============================================================================

constexpr USIZE MAX_ARRAY_SIZE = 16;      // Max elements per array
constexpr USIZE MAX_ARRAY_POOL = 64;      // Max arrays in pool

/**
 * ArrayStorage - Holds array elements
 * Lives in ArrayPool, referenced by Value::array pointer
 */
struct ArrayStorage
{
    Value elements[MAX_ARRAY_SIZE];
    UINT8 count;

    ArrayStorage() noexcept : count(0) {}

    // Get element by index (no bounds check - caller must verify)
    FORCE_INLINE Value& Get(UINT8 index) noexcept
    {
        return elements[index];
    }

    FORCE_INLINE const Value& Get(UINT8 index) const noexcept
    {
        return elements[index];
    }

    // Set element by index (no bounds check - caller must verify)
    FORCE_INLINE void Set(UINT8 index, const Value& value) noexcept
    {
        elements[index] = value;
    }

    // Deep equality comparison
    NOINLINE BOOL DeepEquals(const ArrayStorage& other) const noexcept
    {
        if (count != other.count) return FALSE;
        for (UINT8 i = 0; i < count; i++)
        {
            if (!elements[i].Equals(other.elements[i])) return FALSE;
        }
        return TRUE;
    }
};

/**
 * ArrayPool - Fixed-size pool for array storage
 * Used by Interpreter to allocate arrays
 */
class ArrayPool
{
private:
    ArrayStorage m_pool[MAX_ARRAY_POOL];
    USIZE m_index;

public:
    ArrayPool() noexcept : m_index(0) {}

    // Reset pool for reuse
    void Reset() noexcept
    {
        m_index = 0;
    }

    // Allocate a new array storage
    ArrayStorage* Alloc() noexcept
    {
        if (m_index >= MAX_ARRAY_POOL)
        {
            return nullptr;  // Pool exhausted
        }
        ArrayStorage* storage = &m_pool[m_index++];
        storage->count = 0;
        return storage;
    }

    // Get usage statistics
    USIZE GetCount() const noexcept { return m_index; }
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
        var.nameLength = String::Copy(var.name, MAX_IDENTIFIER_LENGTH, name, nameLen);
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

// Get type name as string (PIC-safe, writes to buffer)
// Buffer should be at least 16 bytes
// Returns the length of the string written
NOINLINE USIZE GetValueTypeName(ValueType type, CHAR* buffer, USIZE bufferSize) noexcept
{
    if (!buffer || bufferSize < 2) return 0;

    switch (type)
    {
        case ValueType::NIL:             return String::CopyEmbed("nil"_embed, buffer, bufferSize);
        case ValueType::BOOL:            return String::CopyEmbed("bool"_embed, buffer, bufferSize);
        case ValueType::NUMBER:          return String::CopyEmbed("number"_embed, buffer, bufferSize);
        case ValueType::STRING:          return String::CopyEmbed("string"_embed, buffer, bufferSize);
        case ValueType::ARRAY:           return String::CopyEmbed("array"_embed, buffer, bufferSize);
        case ValueType::FUNCTION:        return String::CopyEmbed("function"_embed, buffer, bufferSize);
        case ValueType::NATIVE_FUNCTION: return String::CopyEmbed("native"_embed, buffer, bufferSize);
        case ValueType::CFUNCTION:       return String::CopyEmbed("cfunction"_embed, buffer, bufferSize);
        default:                         return String::CopyEmbed("unknown"_embed, buffer, bufferSize);
    }
}

// ============================================================================
// FUNCTION CONTEXT (for CFunction calls)
// ============================================================================

/**
 * Context passed to C++ functions registered with State.
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

    FORCE_INLINE BOOL IsArray(UINT8 index) const noexcept
    {
        return index < argCount && args[index].IsArray();
    }

    FORCE_INLINE INT64 ToNumber(UINT8 index) const noexcept
    {
        return index < argCount ? (INT64)args[index].numberValue : 0;
    }

    FORCE_INLINE DOUBLE ToDouble(UINT8 index) const noexcept
    {
        return index < argCount ? args[index].numberValue : DOUBLE(INT32(0));
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

    FORCE_INLINE ArrayStorage* ToArray(UINT8 index) const noexcept
    {
        return index < argCount && args[index].IsArray() ? args[index].array : nullptr;
    }

    FORCE_INLINE UINT8 ToArrayLength(UINT8 index) const noexcept
    {
        return index < argCount && args[index].IsArray() && args[index].array
            ? args[index].array->count : 0;
    }
};

} // namespace script
