/**
 * interpreter.h - Tree-Walking Interpreter for PICScript
 *
 * Executes the AST directly.
 * Position-independent, no .rdata dependencies.
 *
 * Part of RAL (Runtime Abstraction Layer).
 */

#pragma once

#include "parser.h"
#include "value.h"

namespace script
{

// ============================================================================
// RETURN EXCEPTION (simulated via flag)
// ============================================================================

struct ReturnValue
{
    Value value;
    BOOL hasReturn;

    ReturnValue() noexcept : hasReturn(FALSE) {}
    ReturnValue(const Value& v) noexcept : value(v), hasReturn(TRUE) {}
};

// ============================================================================
// INTERPRETER CLASS
// ============================================================================

class Interpreter
{
private:
    Environment m_globals;
    Environment* m_env;
    BOOL m_hasError;
    CHAR m_errorMessage[256];
    UINT32 m_errorLine;
    ReturnValue m_returnValue;

    // Output callback for print function
    typedef void (*OutputCallback)(const CHAR* str, USIZE len);
    OutputCallback m_outputCallback;

public:
    Interpreter() noexcept
        : m_env(&m_globals)
        , m_hasError(FALSE)
        , m_errorLine(0)
        , m_outputCallback(nullptr)
    {
        m_errorMessage[0] = '\0';
    }

    // Set output callback
    void SetOutputCallback(OutputCallback cb) noexcept
    {
        m_outputCallback = cb;
    }

    // Register a native function (legacy)
    NOINLINE void RegisterNative(const CHAR* name, USIZE nameLen, NativeFn fn) noexcept
    {
        m_globals.Define(name, nameLen, Value::NativeFunction(fn));
    }

    // Register a C++ function with state (Lua-like)
    NOINLINE void RegisterCFunction(const CHAR* name, USIZE nameLen, CFunction fn, State* state) noexcept
    {
        m_globals.Define(name, nameLen, Value::CFunc(fn, state));
    }

    // Execute a program
    NOINLINE void Execute(const Program& program) noexcept
    {
        m_hasError = FALSE;
        m_errorMessage[0] = '\0';

        for (USIZE i = 0; i < program.count; i++)
        {
            ExecuteStmt(program.statements[i]);
            if (m_hasError) break;
        }
    }

    // Check for errors
    FORCE_INLINE BOOL HasError() const noexcept { return m_hasError; }
    FORCE_INLINE const CHAR* GetErrorMessage() const noexcept { return m_errorMessage; }
    FORCE_INLINE UINT32 GetErrorLine() const noexcept { return m_errorLine; }

    // Get environment for builtins
    Environment* GetEnvironment() noexcept { return m_env; }

    // Output helper (for builtins)
    void Output(const CHAR* str, USIZE len) noexcept
    {
        if (m_outputCallback)
        {
            m_outputCallback(str, len);
        }
    }

private:
    // ========================================================================
    // ERROR HANDLING
    // ========================================================================

    NOINLINE void RuntimeError(const CHAR* message, UINT32 line) noexcept
    {
        m_hasError = TRUE;
        m_errorLine = line;

        USIZE i = 0;
        while (message[i] != '\0' && i < sizeof(m_errorMessage) - 1)
        {
            m_errorMessage[i] = message[i];
            i++;
        }
        m_errorMessage[i] = '\0';
    }

    // ========================================================================
    // STATEMENT EXECUTION
    // ========================================================================

    NOINLINE void ExecuteStmt(Stmt* stmt) noexcept
    {
        if (!stmt || m_hasError || m_returnValue.hasReturn) return;

        switch (stmt->type)
        {
            case StmtType::EXPRESSION:
                ExecuteExprStmt(stmt);
                break;
            case StmtType::VAR_DECL:
                ExecuteVarDecl(stmt);
                break;
            case StmtType::BLOCK:
                ExecuteBlock(stmt);
                break;
            case StmtType::IF:
                ExecuteIf(stmt);
                break;
            case StmtType::WHILE:
                ExecuteWhile(stmt);
                break;
            case StmtType::FUNCTION:
                ExecuteFunction(stmt);
                break;
            case StmtType::RETURN:
                ExecuteReturn(stmt);
                break;
            default:
                RuntimeError("Unknown statement type"_embed, stmt->line);
                break;
        }
    }

    NOINLINE void ExecuteExprStmt(Stmt* stmt) noexcept
    {
        Evaluate(stmt->expression.expression);
    }

    NOINLINE void ExecuteVarDecl(Stmt* stmt) noexcept
    {
        Value value = Value::Nil();
        if (stmt->varDecl.initializer)
        {
            value = Evaluate(stmt->varDecl.initializer);
        }

        if (!m_env->Define(stmt->varDecl.name, stmt->varDecl.nameLength, value))
        {
            RuntimeError("Failed to define variable"_embed, stmt->line);
        }
    }

    NOINLINE void ExecuteBlock(Stmt* stmt) noexcept
    {
        m_env->PushScope();

        for (USIZE i = 0; i < stmt->block.count; i++)
        {
            ExecuteStmt(stmt->block.statements[i]);
            if (m_hasError || m_returnValue.hasReturn) break;
        }

        m_env->PopScope();
    }

    NOINLINE void ExecuteIf(Stmt* stmt) noexcept
    {
        Value condition = Evaluate(stmt->ifStmt.condition);

        if (condition.IsTruthy())
        {
            ExecuteStmt(stmt->ifStmt.thenBranch);
        }
        else if (stmt->ifStmt.elseBranch)
        {
            ExecuteStmt(stmt->ifStmt.elseBranch);
        }
    }

    NOINLINE void ExecuteWhile(Stmt* stmt) noexcept
    {
        while (!m_hasError && !m_returnValue.hasReturn)
        {
            Value condition = Evaluate(stmt->whileStmt.condition);
            if (!condition.IsTruthy()) break;

            ExecuteStmt(stmt->whileStmt.body);
        }
    }

    NOINLINE void ExecuteFunction(Stmt* stmt) noexcept
    {
        Value fn = Value::Function(&stmt->function, m_env);
        m_env->Define(stmt->function.name, stmt->function.nameLength, fn);
    }

    NOINLINE void ExecuteReturn(Stmt* stmt) noexcept
    {
        Value value = Value::Nil();
        if (stmt->returnStmt.value)
        {
            value = Evaluate(stmt->returnStmt.value);
        }
        m_returnValue = ReturnValue(value);
    }

    // ========================================================================
    // EXPRESSION EVALUATION
    // ========================================================================

    NOINLINE Value Evaluate(Expr* expr) noexcept
    {
        if (!expr || m_hasError) return Value::Nil();

        switch (expr->type)
        {
            case ExprType::NUMBER_LITERAL:
                return Value::Number(expr->number.intValue);

            case ExprType::STRING_LITERAL:
                return Value::String(expr->string.value, expr->string.length);

            case ExprType::BOOL_LITERAL:
                return Value::Bool(expr->boolean.value);

            case ExprType::NIL_LITERAL:
                return Value::Nil();

            case ExprType::IDENTIFIER:
                return EvaluateIdentifier(expr);

            case ExprType::BINARY:
                return EvaluateBinary(expr);

            case ExprType::UNARY:
                return EvaluateUnary(expr);

            case ExprType::CALL:
                return EvaluateCall(expr);

            case ExprType::ASSIGN:
                return EvaluateAssign(expr);

            case ExprType::LOGICAL:
                return EvaluateLogical(expr);

            default:
                RuntimeError("Unknown expression type"_embed, expr->line);
                return Value::Nil();
        }
    }

    NOINLINE Value EvaluateIdentifier(Expr* expr) noexcept
    {
        Value value;
        if (!m_env->Get(expr->identifier.name, expr->identifier.length, value))
        {
            RuntimeError("Undefined variable"_embed, expr->line);
            return Value::Nil();
        }
        return value;
    }

    NOINLINE Value EvaluateBinary(Expr* expr) noexcept
    {
        Value left = Evaluate(expr->binary.left);
        Value right = Evaluate(expr->binary.right);
        TokenType op = expr->binary.op;

        // Fast path: both operands are numbers (most common case)
        if (left.IsNumber() && right.IsNumber())
        {
            INT64 l = left.numberValue;
            INT64 r = right.numberValue;
            switch (op)
            {
                case TokenType::PLUS:          return Value::Number(l + r);
                case TokenType::MINUS:         return Value::Number(l - r);
                case TokenType::STAR:          return Value::Number(l * r);
                case TokenType::SLASH:
                    if (r == 0) { RuntimeError("Division by zero"_embed, expr->line); return Value::Nil(); }
                    return Value::Number(l / r);
                case TokenType::PERCENT:
                    if (r == 0) { RuntimeError("Division by zero"_embed, expr->line); return Value::Nil(); }
                    return Value::Number(l % r);
                case TokenType::LESS:          return Value::Bool(l < r);
                case TokenType::GREATER:       return Value::Bool(l > r);
                case TokenType::LESS_EQUAL:    return Value::Bool(l <= r);
                case TokenType::GREATER_EQUAL: return Value::Bool(l >= r);
                case TokenType::EQUAL_EQUAL:   return Value::Bool(l == r);
                case TokenType::BANG_EQUAL:    return Value::Bool(l != r);
                default: break;
            }
        }

        // String concatenation
        if (op == TokenType::PLUS && left.IsString() && right.IsString())
        {
            CHAR buffer[MAX_STRING_VALUE];
            USIZE pos = 0;
            for (USIZE i = 0; i < left.strLength && pos < MAX_STRING_VALUE - 1; i++)
                buffer[pos++] = left.strValue[i];
            for (USIZE i = 0; i < right.strLength && pos < MAX_STRING_VALUE - 1; i++)
                buffer[pos++] = right.strValue[i];
            buffer[pos] = '\0';
            return Value::String(buffer, pos);
        }

        // Equality (works for all types)
        if (op == TokenType::EQUAL_EQUAL) return Value::Bool(left.Equals(right));
        if (op == TokenType::BANG_EQUAL)  return Value::Bool(!left.Equals(right));

        // Type error for numeric operations
        RuntimeError("Type error"_embed, expr->line);
        return Value::Nil();
    }

    NOINLINE Value EvaluateUnary(Expr* expr) noexcept
    {
        Value operand = Evaluate(expr->unary.operand);

        switch (expr->unary.op)
        {
            case TokenType::MINUS:
                if (operand.IsNumber())
                {
                    return Value::Number(-operand.numberValue);
                }
                RuntimeError("Operand must be a number"_embed, expr->line);
                return Value::Nil();

            case TokenType::BANG:
                return Value::Bool(!operand.IsTruthy());

            default:
                RuntimeError("Unknown unary operator"_embed, expr->line);
                return Value::Nil();
        }
    }

    NOINLINE Value EvaluateCall(Expr* expr) noexcept
    {
        Value callee = Evaluate(expr->call.callee);

        // Evaluate arguments
        Value args[MAX_CALL_ARGS];
        UINT8 argCount = expr->call.argCount;
        for (UINT8 i = 0; i < argCount; i++)
        {
            args[i] = Evaluate(expr->call.args[i]);
        }

        // Dispatch by callable type
        switch (callee.type)
        {
            case ValueType::NATIVE_FUNCTION:
                return callee.nativeFn(args, argCount, m_env);

            case ValueType::CFUNCTION:
            {
                FunctionContext ctx = { callee.cfunction.state, args, argCount };
                return callee.cfunction.func(ctx);
            }

            case ValueType::FUNCTION:
                return CallFunction(callee.function.declaration, args, argCount, expr->line);

            default:
                RuntimeError("Not callable"_embed, expr->line);
                return Value::Nil();
        }
    }

    NOINLINE Value CallFunction(const FunctionStmt* decl, Value* args, UINT8 argCount, UINT32 line) noexcept
    {
        if (argCount != decl->paramCount)
        {
            RuntimeError("Argument count"_embed, line);
            return Value::Nil();
        }

        m_env->PushScope();

        // Bind parameters
        for (UINT8 i = 0; i < argCount; i++)
        {
            m_env->Define(decl->params[i], decl->paramLengths[i], args[i]);
        }

        // Execute function body
        m_returnValue.hasReturn = FALSE;
        ExecuteStmt(decl->body);
        m_env->PopScope();

        // Get return value
        if (m_returnValue.hasReturn)
        {
            Value result = m_returnValue.value;
            m_returnValue.hasReturn = FALSE;
            return result;
        }
        return Value::Nil();
    }

    NOINLINE Value EvaluateAssign(Expr* expr) noexcept
    {
        Value value = Evaluate(expr->assign.value);
        if (!m_env->Assign(expr->assign.name, expr->assign.nameLength, value))
        {
            RuntimeError("Undefined"_embed, expr->line);
            return Value::Nil();
        }
        return value;
    }

    NOINLINE Value EvaluateLogical(Expr* expr) noexcept
    {
        Value left = Evaluate(expr->logical.left);
        // Short-circuit: || returns on truthy, && returns on falsy
        BOOL shortCircuit = (expr->logical.op == TokenType::OR_OR) ? left.IsTruthy() : !left.IsTruthy();
        return shortCircuit ? left : Evaluate(expr->logical.right);
    }
};

} // namespace script
