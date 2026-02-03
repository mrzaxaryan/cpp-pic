/**
 * state.h - State Management for PIL (Position Independent Language)
 *
 * Provides a State-based API for managing script state and registering
 * native C++ functions.
 *
 * Position-independent, no .rdata dependencies.
 *
 * Part of RAL (Runtime Abstraction Layer).
 *
 * USAGE:
 *   script::State L;
 *   L.SetOutput(MyOutputFunc);
 *   L.Register("print", MyPrintFunction);
 *   L.DoString("print(\"Hello!\");");
 */

#pragma once

#include "interpreter.h"

namespace script
{

// ============================================================================
// STATE CLASS
// ============================================================================

class State
{
private:
    // Script engine components
    Lexer m_lexer;
    ASTAllocator m_allocator;
    Parser m_parser;
    Interpreter m_interpreter;

    // Output callback
    typedef void (*OutputFn)(const CHAR* str, USIZE len);
    OutputFn m_outputFn;

    // Error state
    BOOL m_hasError;
    CHAR m_errorMessage[512];
    UINT32 m_errorLine;

    // User data pointer (for extensions like FileIO)
    PVOID m_userData;

public:
    // ========================================================================
    // CONSTRUCTION
    // ========================================================================

    State() noexcept
        : m_outputFn(nullptr)
        , m_hasError(FALSE)
        , m_errorLine(0)
        , m_userData(nullptr)
    {
        m_errorMessage[0] = '\0';
    }

    // ========================================================================
    // OUTPUT CONFIGURATION
    // ========================================================================

    /**
     * Set output callback for print and other output functions.
     */
    void SetOutput(OutputFn fn) noexcept
    {
        m_outputFn = fn;
        m_interpreter.SetOutputCallback(fn);
    }

    /**
     * Write string to output.
     * Used by native functions for output.
     */
    void Write(const CHAR* str, USIZE len) noexcept
    {
        if (m_outputFn)
        {
            m_outputFn(str, len);
        }
    }

    /**
     * Write null-terminated string to output.
     */
    void Write(const CHAR* str) noexcept
    {
        if (m_outputFn && str)
        {
            USIZE len = 0;
            while (str[len] != '\0') len++;
            m_outputFn(str, len);
        }
    }

    /**
     * Write newline to output.
     */
    void WriteLine() noexcept
    {
        if (m_outputFn)
        {
            m_outputFn("\n"_embed, 1);
        }
    }

    // ========================================================================
    // FUNCTION REGISTRATION
    // ========================================================================

    /**
     * Register a C++ function with the script state.
     *
     * The function will be callable from scripts by the given name.
     *
     * @param name Function name as it will be called in scripts
     * @param func C++ function pointer (CFunction signature)
     * @return TRUE if registered successfully
     *
     * Example:
     *   Value MyFunc(FunctionContext& ctx) {
     *       ctx.state->Write("Hello!\n");
     *       return Value::Nil();
     *   }
     *   L.Register("myfunc", MyFunc);
     */
    NOINLINE BOOL Register(const CHAR* name, CFunction func) noexcept
    {
        if (!name || !func) return FALSE;

        // Get name length
        USIZE nameLen = 0;
        while (name[nameLen] != '\0')
        {
            nameLen++;
        }

        // Register with interpreter as CFunction (passes this State pointer)
        m_interpreter.RegisterCFunction(name, nameLen, func, this);
        return TRUE;
    }

    /**
     * Register using character array (avoids .rdata).
     */
    template <USIZE N>
    FORCE_INLINE BOOL Register(const CHAR (&name)[N], CFunction func) noexcept
    {
        return Register(static_cast<const CHAR*>(name), func);
    }

    // ========================================================================
    // SCRIPT EXECUTION
    // ========================================================================

    /**
     * Execute a script string.
     *
     * @param source Script source code
     * @param length Length of source code
     * @return TRUE if executed successfully
     */
    NOINLINE BOOL DoString(const CHAR* source, USIZE length) noexcept
    {
        m_hasError = FALSE;
        m_errorMessage[0] = '\0';
        m_errorLine = 0;
        m_allocator.Reset();

        // Parse
        m_lexer.Init(source, length);
        m_parser.Init(&m_lexer, &m_allocator);

        Program program = m_parser.Parse();

        if (m_parser.HasError())
        {
            SetError("parse error: "_embed, m_parser.GetErrorMessage(), m_parser.GetErrorLine());
            return FALSE;
        }

        // Execute
        m_interpreter.Execute(program);

        if (m_interpreter.HasError())
        {
            SetError("runtime error: "_embed, m_interpreter.GetErrorMessage(), m_interpreter.GetErrorLine());
            return FALSE;
        }

        return TRUE;
    }

    /**
     * Execute a null-terminated script string.
     */
    NOINLINE BOOL DoString(const CHAR* source) noexcept
    {
        USIZE len = 0;
        while (source[len] != '\0') len++;
        return DoString(source, len);
    }

    // ========================================================================
    // GLOBAL VARIABLES
    // ========================================================================

    /**
     * Set a global variable.
     */
    NOINLINE void SetGlobal(const CHAR* name, USIZE nameLen, const Value& value) noexcept
    {
        m_interpreter.GetEnvironment()->Define(name, nameLen, value);
    }

    /**
     * Set a global number.
     */
    void SetGlobalNumber(const CHAR* name, USIZE nameLen, INT64 value) noexcept
    {
        SetGlobal(name, nameLen, Value::Number(value));
    }

    /**
     * Set a global string.
     */
    void SetGlobalString(const CHAR* name, USIZE nameLen, const CHAR* value, USIZE valueLen) noexcept
    {
        SetGlobal(name, nameLen, Value::String(value, valueLen));
    }

    /**
     * Set a global boolean.
     */
    void SetGlobalBool(const CHAR* name, USIZE nameLen, BOOL value) noexcept
    {
        SetGlobal(name, nameLen, Value::Bool(value));
    }

    /**
     * Set a global float (DOUBLE).
     */
    void SetGlobalFloat(const CHAR* name, USIZE nameLen, DOUBLE value) noexcept
    {
        SetGlobal(name, nameLen, Value::Float(value));
    }

    /**
     * Get a global variable.
     */
    NOINLINE BOOL GetGlobal(const CHAR* name, USIZE nameLen, Value& outValue) noexcept
    {
        return m_interpreter.GetEnvironment()->Get(name, nameLen, outValue);
    }

    // ========================================================================
    // ERROR HANDLING
    // ========================================================================

    FORCE_INLINE BOOL HasError() const noexcept { return m_hasError; }
    FORCE_INLINE const CHAR* GetError() const noexcept { return m_errorMessage; }
    FORCE_INLINE UINT32 GetErrorLine() const noexcept { return m_errorLine; }

    /**
     * Clear error state.
     */
    void ClearError() noexcept
    {
        m_hasError = FALSE;
        m_errorMessage[0] = '\0';
        m_errorLine = 0;
    }

    // ========================================================================
    // ADVANCED ACCESS
    // ========================================================================

    /**
     * Get raw interpreter reference.
     */
    Interpreter& GetInterpreter() noexcept { return m_interpreter; }

    /**
     * Set user data pointer (for extensions like FileIO).
     */
    void SetUserData(PVOID data) noexcept { m_userData = data; }

    /**
     * Get user data pointer.
     */
    PVOID GetUserData() const noexcept { return m_userData; }

private:
    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    NOINLINE void SetError(const CHAR* prefix, const CHAR* message, UINT32 line) noexcept
    {
        m_hasError = TRUE;
        m_errorLine = line;

        USIZE pos = 0;
        for (USIZE i = 0; prefix[i] != '\0' && pos < sizeof(m_errorMessage) - 1; i++)
        {
            m_errorMessage[pos++] = prefix[i];
        }
        for (USIZE i = 0; message[i] != '\0' && pos < sizeof(m_errorMessage) - 1; i++)
        {
            m_errorMessage[pos++] = message[i];
        }
        m_errorMessage[pos] = '\0';
    }
};

} // namespace script
