/**
 * parser.h - Recursive Descent Parser for PIL (Position Independent Language)
 *
 * Parses tokens into an AST.
 * Position-independent, no .rdata dependencies.
 *
 * Part of RAL (Runtime Abstraction Layer).
 */

#pragma once

#include "lexer.h"
#include "ast.h"

namespace script
{

// ============================================================================
// PROGRAM AST
// ============================================================================

constexpr USIZE MAX_PROGRAM_STMTS = 256;

struct Program
{
    Stmt* statements[MAX_PROGRAM_STMTS];
    USIZE count;

    Program() noexcept : count(0) {}
};

// ============================================================================
// PARSER CLASS
// ============================================================================

class Parser
{
private:
    Lexer* m_lexer;
    ASTAllocator* m_alloc;
    Token m_current;
    Token m_previous;
    BOOL m_hasError;
    BOOL m_panicMode;
    CHAR m_errorMessage[256];
    UINT32 m_errorLine;
    UINT32 m_errorColumn;

public:
    Parser() noexcept
        : m_lexer(nullptr)
        , m_alloc(nullptr)
        , m_hasError(FALSE)
        , m_panicMode(FALSE)
        , m_errorLine(0)
        , m_errorColumn(0)
    {
        m_errorMessage[0] = '\0';
    }

    // Initialize parser
    NOINLINE void Init(Lexer* lexer, ASTAllocator* alloc) noexcept
    {
        m_lexer = lexer;
        m_alloc = alloc;
        m_hasError = FALSE;
        m_panicMode = FALSE;
        m_errorMessage[0] = '\0';
        m_errorLine = 0;
        m_errorColumn = 0;

        // Prime the parser
        Advance();
    }

    // Parse the entire program
    NOINLINE Program Parse() noexcept
    {
        Program program;

        while (!Check(TokenType::END_OF_FILE))
        {
            Stmt* stmt = Declaration();
            if (stmt && program.count < MAX_PROGRAM_STMTS)
            {
                program.statements[program.count++] = stmt;
            }

            if (m_panicMode)
            {
                Synchronize();
            }
        }

        return program;
    }

    // Check for errors
    FORCE_INLINE BOOL HasError() const noexcept { return m_hasError; }
    FORCE_INLINE const CHAR* GetErrorMessage() const noexcept { return m_errorMessage; }
    FORCE_INLINE UINT32 GetErrorLine() const noexcept { return m_errorLine; }
    FORCE_INLINE UINT32 GetErrorColumn() const noexcept { return m_errorColumn; }

private:
    // ========================================================================
    // TOKEN HANDLING
    // ========================================================================

    NOINLINE void Advance() noexcept
    {
        m_previous = m_current;
        m_current = m_lexer->NextToken();

        if (m_current.IsError())
        {
            ErrorAt(m_current, m_current.value.strValue);
        }
    }

    FORCE_INLINE BOOL Check(TokenType type) const noexcept
    {
        return m_current.Is(type);
    }

    NOINLINE BOOL Match(TokenType type) noexcept
    {
        if (!Check(type)) return FALSE;
        Advance();
        return TRUE;
    }

    NOINLINE void Consume(TokenType type, const CHAR* message) noexcept
    {
        if (Check(type))
        {
            Advance();
            return;
        }
        ErrorAtCurrent(message);
    }

    // ========================================================================
    // ERROR HANDLING
    // ========================================================================

    NOINLINE void ErrorAt(const Token& token, const CHAR* message) noexcept
    {
        if (m_panicMode) return;
        m_panicMode = TRUE;
        m_hasError = TRUE;
        m_errorLine = token.line;
        m_errorColumn = token.column;

        // Copy error message
        USIZE i = 0;
        while (message[i] != '\0' && i < sizeof(m_errorMessage) - 1)
        {
            m_errorMessage[i] = message[i];
            i++;
        }
        m_errorMessage[i] = '\0';
    }

    FORCE_INLINE void ErrorAtCurrent(const CHAR* message) noexcept
    {
        ErrorAt(m_current, message);
    }

    FORCE_INLINE void Error(const CHAR* message) noexcept
    {
        ErrorAt(m_previous, message);
    }

    NOINLINE void Synchronize() noexcept
    {
        m_panicMode = FALSE;

        while (!Check(TokenType::END_OF_FILE))
        {
            if (m_previous.Is(TokenType::SEMICOLON)) return;

            switch (m_current.type)
            {
                case TokenType::FN:
                case TokenType::VAR:
                case TokenType::FOR:
                case TokenType::IF:
                case TokenType::WHILE:
                case TokenType::RETURN:
                case TokenType::BREAK:
                case TokenType::CONTINUE:
                    return;
                default:
                    break;
            }

            Advance();
        }
    }

    // ========================================================================
    // DECLARATIONS
    // ========================================================================

    NOINLINE Stmt* Declaration() noexcept
    {
        if (Match(TokenType::FN))
        {
            return FunctionDeclaration();
        }
        if (Match(TokenType::VAR))
        {
            return VarDeclaration();
        }
        return Statement();
    }

    NOINLINE Stmt* FunctionDeclaration() noexcept
    {
        Consume(TokenType::IDENTIFIER, "Expected function name"_embed);

        const CHAR* name = m_previous.value.strValue;
        USIZE nameLen = m_previous.length;
        UINT32 line = m_previous.line;
        UINT32 col = m_previous.column;

        Stmt* fnStmt = MakeFunctionStmt(*m_alloc, name, nameLen, line, col);
        if (!fnStmt) return nullptr;

        Consume(TokenType::LEFT_PAREN, "Expected '(' after function name"_embed);

        // Parse parameters
        if (!Check(TokenType::RIGHT_PAREN))
        {
            do
            {
                if (fnStmt->function.paramCount >= MAX_FUNCTION_PARAMS)
                {
                    ErrorAtCurrent("Too many parameters"_embed);
                    break;
                }

                Consume(TokenType::IDENTIFIER, "Expected parameter name"_embed);

                USIZE paramLen = m_previous.length;
                USIZE copyLen = paramLen < MAX_IDENTIFIER_LENGTH - 1 ? paramLen : MAX_IDENTIFIER_LENGTH - 1;
                for (USIZE i = 0; i < copyLen; i++)
                {
                    fnStmt->function.params[fnStmt->function.paramCount][i] = m_previous.value.strValue[i];
                }
                fnStmt->function.params[fnStmt->function.paramCount][copyLen] = '\0';
                fnStmt->function.paramLengths[fnStmt->function.paramCount] = copyLen;
                fnStmt->function.paramCount++;

            } while (Match(TokenType::COMMA));
        }

        Consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters"_embed);
        Consume(TokenType::LEFT_BRACE, "Expected '{' before function body"_embed);

        fnStmt->function.body = BlockStatement();

        return fnStmt;
    }

    NOINLINE Stmt* VarDeclaration() noexcept
    {
        Consume(TokenType::IDENTIFIER, "Expected variable name"_embed);

        // Copy name to local buffer before advancing (m_previous will be overwritten)
        CHAR name[MAX_IDENTIFIER_LENGTH];
        USIZE nameLen = m_previous.length < MAX_IDENTIFIER_LENGTH - 1 ? m_previous.length : MAX_IDENTIFIER_LENGTH - 1;
        for (USIZE i = 0; i < nameLen; i++)
        {
            name[i] = m_previous.value.strValue[i];
        }
        name[nameLen] = '\0';
        UINT32 line = m_previous.line;
        UINT32 col = m_previous.column;

        Expr* initializer = nullptr;
        if (Match(TokenType::ASSIGN))
        {
            initializer = Expression();
        }

        Consume(TokenType::SEMICOLON, "Expected ';' after variable declaration"_embed);

        return MakeVarDeclStmt(*m_alloc, name, nameLen, initializer, line, col);
    }

    // ========================================================================
    // STATEMENTS
    // ========================================================================

    NOINLINE Stmt* Statement() noexcept
    {
        if (Match(TokenType::IF))
        {
            return IfStatement();
        }
        if (Match(TokenType::WHILE))
        {
            return WhileStatement();
        }
        if (Match(TokenType::FOR))
        {
            return ForStatement();
        }
        if (Match(TokenType::RETURN))
        {
            return ReturnStatement();
        }
        if (Match(TokenType::BREAK))
        {
            return BreakStatement();
        }
        if (Match(TokenType::CONTINUE))
        {
            return ContinueStatement();
        }
        if (Match(TokenType::LEFT_BRACE))
        {
            return BlockStatement();
        }

        return ExpressionStatement();
    }

    NOINLINE Stmt* IfStatement() noexcept
    {
        UINT32 line = m_previous.line;
        UINT32 col = m_previous.column;

        Consume(TokenType::LEFT_PAREN, "Expected '(' after 'if'"_embed);
        Expr* condition = Expression();
        Consume(TokenType::RIGHT_PAREN, "Expected ')' after condition"_embed);

        Stmt* thenBranch = Statement();
        Stmt* elseBranch = nullptr;

        if (Match(TokenType::ELSE))
        {
            elseBranch = Statement();
        }

        return MakeIfStmt(*m_alloc, condition, thenBranch, elseBranch, line, col);
    }

    NOINLINE Stmt* WhileStatement() noexcept
    {
        UINT32 line = m_previous.line;
        UINT32 col = m_previous.column;

        Consume(TokenType::LEFT_PAREN, "Expected '(' after 'while'"_embed);
        Expr* condition = Expression();
        Consume(TokenType::RIGHT_PAREN, "Expected ')' after condition"_embed);

        Stmt* body = Statement();

        return MakeWhileStmt(*m_alloc, condition, body, line, col);
    }

    NOINLINE Stmt* ForStatement() noexcept
    {
        UINT32 line = m_previous.line;
        UINT32 col = m_previous.column;

        Consume(TokenType::LEFT_PAREN, "Expected '(' after 'for'"_embed);

        // Check for for-each vs traditional for
        // for-each: for (var x in collection) or for (var i, x in collection)
        // traditional: for (init; cond; incr)
        if (Match(TokenType::VAR))
        {
            // Must be followed by identifier
            Consume(TokenType::IDENTIFIER, "Expected variable name"_embed);

            // Save the first identifier
            CHAR firstName[MAX_IDENTIFIER_LENGTH];
            USIZE firstLen = m_previous.length < MAX_IDENTIFIER_LENGTH - 1 ? m_previous.length : MAX_IDENTIFIER_LENGTH - 1;
            for (USIZE i = 0; i < firstLen; i++)
            {
                firstName[i] = m_previous.value.strValue[i];
            }
            firstName[firstLen] = '\0';
            UINT32 varLine = m_previous.line;
            UINT32 varCol = m_previous.column;

            // Check what comes next to determine loop type
            if (Check(TokenType::IN))
            {
                // for (var x in collection) - for-each without index
                Advance(); // consume 'in'

                Expr* collection = Expression();
                Consume(TokenType::RIGHT_PAREN, "Expected ')' after for-each collection"_embed);
                Stmt* body = Statement();

                return MakeForEachStmt(*m_alloc, firstName, firstLen, nullptr, 0, FALSE, collection, body, line, col);
            }
            else if (Check(TokenType::COMMA))
            {
                // for (var i, x in collection) - for-each with index
                Advance(); // consume ','

                Consume(TokenType::IDENTIFIER, "Expected identifier after ','"_embed);

                CHAR secondName[MAX_IDENTIFIER_LENGTH];
                USIZE secondLen = m_previous.length < MAX_IDENTIFIER_LENGTH - 1 ? m_previous.length : MAX_IDENTIFIER_LENGTH - 1;
                for (USIZE i = 0; i < secondLen; i++)
                {
                    secondName[i] = m_previous.value.strValue[i];
                }
                secondName[secondLen] = '\0';

                Consume(TokenType::IN, "Expected 'in' after loop variables"_embed);

                Expr* collection = Expression();
                Consume(TokenType::RIGHT_PAREN, "Expected ')' after for-each collection"_embed);
                Stmt* body = Statement();

                // firstName is index, secondName is value
                return MakeForEachStmt(*m_alloc, secondName, secondLen, firstName, firstLen, TRUE, collection, body, line, col);
            }
            else
            {
                // Traditional for loop with var initializer: for (var i = expr; ...)
                // We've already consumed 'var' and identifier, now parse the rest of var declaration
                Expr* varInit = nullptr;
                if (Match(TokenType::ASSIGN))
                {
                    varInit = Expression();
                }
                Consume(TokenType::SEMICOLON, "Expected ';' after variable declaration"_embed);

                Stmt* initializer = MakeVarDeclStmt(*m_alloc, firstName, firstLen, varInit, varLine, varCol);

                // Continue with condition and increment
                return ParseTraditionalForBody(initializer, line, col);
            }
        }

        // Traditional for loop without var initializer
        Stmt* initializer = nullptr;
        if (Match(TokenType::SEMICOLON))
        {
            // No initializer
        }
        else
        {
            initializer = ExpressionStatement();
        }

        return ParseTraditionalForBody(initializer, line, col);
    }

    // Helper to parse the rest of a traditional for loop (condition, increment, body)
    NOINLINE Stmt* ParseTraditionalForBody(Stmt* initializer, UINT32 line, UINT32 col) noexcept
    {
        // Condition
        Expr* condition = nullptr;
        if (!Check(TokenType::SEMICOLON))
        {
            condition = Expression();
        }
        Consume(TokenType::SEMICOLON, "Expected ';' after loop condition"_embed);

        // Increment
        Expr* increment = nullptr;
        if (!Check(TokenType::RIGHT_PAREN))
        {
            increment = Expression();
        }
        Consume(TokenType::RIGHT_PAREN, "Expected ')' after for clauses"_embed);

        Stmt* body = Statement();

        // Desugar to while loop
        // If there's an increment, wrap body in a block with increment at the end
        if (increment)
        {
            Stmt* block = MakeBlockStmt(*m_alloc, line, col);
            if (block)
            {
                block->block.statements[0] = body;
                block->block.statements[1] = MakeExprStmt(*m_alloc, increment, line, col);
                block->block.count = 2;
                body = block;
            }
        }

        // If no condition, use true
        if (!condition)
        {
            condition = MakeBoolExpr(*m_alloc, TRUE, line, col);
        }

        // Create while loop
        body = MakeWhileStmt(*m_alloc, condition, body, line, col);

        // If there's an initializer, wrap in a block
        if (initializer)
        {
            Stmt* block = MakeBlockStmt(*m_alloc, line, col);
            if (block)
            {
                block->block.statements[0] = initializer;
                block->block.statements[1] = body;
                block->block.count = 2;
                body = block;
            }
        }

        return body;
    }

    NOINLINE Stmt* ReturnStatement() noexcept
    {
        UINT32 line = m_previous.line;
        UINT32 col = m_previous.column;

        Expr* value = nullptr;
        if (!Check(TokenType::SEMICOLON))
        {
            value = Expression();
        }

        Consume(TokenType::SEMICOLON, "Expected ';' after return value"_embed);

        return MakeReturnStmt(*m_alloc, value, line, col);
    }

    NOINLINE Stmt* BreakStatement() noexcept
    {
        UINT32 line = m_previous.line;
        UINT32 col = m_previous.column;

        Consume(TokenType::SEMICOLON, "Expected ';' after 'break'"_embed);

        return MakeBreakStmt(*m_alloc, line, col);
    }

    NOINLINE Stmt* ContinueStatement() noexcept
    {
        UINT32 line = m_previous.line;
        UINT32 col = m_previous.column;

        Consume(TokenType::SEMICOLON, "Expected ';' after 'continue'"_embed);

        return MakeContinueStmt(*m_alloc, line, col);
    }

    NOINLINE Stmt* BlockStatement() noexcept
    {
        UINT32 line = m_previous.line;
        UINT32 col = m_previous.column;

        Stmt* block = MakeBlockStmt(*m_alloc, line, col);
        if (!block) return nullptr;

        while (!Check(TokenType::RIGHT_BRACE) && !Check(TokenType::END_OF_FILE))
        {
            Stmt* stmt = Declaration();
            if (stmt && block->block.count < MAX_BLOCK_STMTS)
            {
                block->block.statements[block->block.count++] = stmt;
            }
        }

        Consume(TokenType::RIGHT_BRACE, "Expected '}' after block"_embed);

        return block;
    }

    NOINLINE Stmt* ExpressionStatement() noexcept
    {
        UINT32 line = m_current.line;
        UINT32 col = m_current.column;

        Expr* expr = Expression();
        Consume(TokenType::SEMICOLON, "Expected ';' after expression"_embed);

        return MakeExprStmt(*m_alloc, expr, line, col);
    }

    // ========================================================================
    // EXPRESSIONS (Precedence Climbing)
    // ========================================================================

    NOINLINE Expr* Expression() noexcept
    {
        return Assignment();
    }

    NOINLINE Expr* Assignment() noexcept
    {
        Expr* expr = Or();

        if (Match(TokenType::ASSIGN))
        {
            UINT32 line = m_previous.line;
            UINT32 col = m_previous.column;
            Expr* value = Assignment();

            if (expr->type == ExprType::IDENTIFIER)
            {
                return MakeAssignExpr(*m_alloc, expr->identifier.name, expr->identifier.length, value, line, col);
            }

            // Array index assignment: arr[i] = value
            if (expr->type == ExprType::INDEX)
            {
                return MakeIndexAssignExpr(*m_alloc, expr->index.object, expr->index.index, value, line, col);
            }

            Error("Invalid assignment target"_embed);
        }

        // Compound assignment: +=, -=, *=, /=
        if (Match(TokenType::PLUS_EQUAL) || Match(TokenType::MINUS_EQUAL) ||
            Match(TokenType::STAR_EQUAL) || Match(TokenType::SLASH_EQUAL))
        {
            TokenType op = m_previous.type;
            UINT32 line = m_previous.line;
            UINT32 col = m_previous.column;
            Expr* value = Assignment();

            if (expr->type == ExprType::IDENTIFIER)
            {
                // Desugar: a += b -> a = a + b
                TokenType binOp;
                switch (op)
                {
                    case TokenType::PLUS_EQUAL:  binOp = TokenType::PLUS;  break;
                    case TokenType::MINUS_EQUAL: binOp = TokenType::MINUS; break;
                    case TokenType::STAR_EQUAL:  binOp = TokenType::STAR;  break;
                    case TokenType::SLASH_EQUAL: binOp = TokenType::SLASH; break;
                    default: binOp = TokenType::PLUS; break;
                }

                Expr* binExpr = MakeBinaryExpr(*m_alloc, expr, binOp, value, line, col);
                return MakeAssignExpr(*m_alloc, expr->identifier.name, expr->identifier.length, binExpr, line, col);
            }

            // Array compound assignment: arr[i] += value
            if (expr->type == ExprType::INDEX)
            {
                TokenType binOp;
                switch (op)
                {
                    case TokenType::PLUS_EQUAL:  binOp = TokenType::PLUS;  break;
                    case TokenType::MINUS_EQUAL: binOp = TokenType::MINUS; break;
                    case TokenType::STAR_EQUAL:  binOp = TokenType::STAR;  break;
                    case TokenType::SLASH_EQUAL: binOp = TokenType::SLASH; break;
                    default: binOp = TokenType::PLUS; break;
                }

                // Desugar: arr[i] += b -> arr[i] = arr[i] + b
                Expr* binExpr = MakeBinaryExpr(*m_alloc, expr, binOp, value, line, col);
                return MakeIndexAssignExpr(*m_alloc, expr->index.object, expr->index.index, binExpr, line, col);
            }

            Error("Invalid assignment target"_embed);
        }

        return expr;
    }

    NOINLINE Expr* Or() noexcept
    {
        Expr* expr = And();

        while (Match(TokenType::OR_OR))
        {
            TokenType op = m_previous.type;
            UINT32 line = m_previous.line;
            UINT32 col = m_previous.column;
            Expr* right = And();
            expr = MakeLogicalExpr(*m_alloc, expr, op, right, line, col);
        }

        return expr;
    }

    NOINLINE Expr* And() noexcept
    {
        Expr* expr = Equality();

        while (Match(TokenType::AND_AND))
        {
            TokenType op = m_previous.type;
            UINT32 line = m_previous.line;
            UINT32 col = m_previous.column;
            Expr* right = Equality();
            expr = MakeLogicalExpr(*m_alloc, expr, op, right, line, col);
        }

        return expr;
    }

    NOINLINE Expr* Equality() noexcept
    {
        Expr* expr = Comparison();

        while (Match(TokenType::EQUAL_EQUAL) || Match(TokenType::BANG_EQUAL))
        {
            TokenType op = m_previous.type;
            UINT32 line = m_previous.line;
            UINT32 col = m_previous.column;
            Expr* right = Comparison();
            expr = MakeBinaryExpr(*m_alloc, expr, op, right, line, col);
        }

        return expr;
    }

    NOINLINE Expr* Comparison() noexcept
    {
        Expr* expr = Term();

        while (Match(TokenType::LESS) || Match(TokenType::GREATER) ||
               Match(TokenType::LESS_EQUAL) || Match(TokenType::GREATER_EQUAL))
        {
            TokenType op = m_previous.type;
            UINT32 line = m_previous.line;
            UINT32 col = m_previous.column;
            Expr* right = Term();
            expr = MakeBinaryExpr(*m_alloc, expr, op, right, line, col);
        }

        return expr;
    }

    NOINLINE Expr* Term() noexcept
    {
        Expr* expr = Factor();

        while (Match(TokenType::PLUS) || Match(TokenType::MINUS))
        {
            TokenType op = m_previous.type;
            UINT32 line = m_previous.line;
            UINT32 col = m_previous.column;
            Expr* right = Factor();
            expr = MakeBinaryExpr(*m_alloc, expr, op, right, line, col);
        }

        return expr;
    }

    NOINLINE Expr* Factor() noexcept
    {
        Expr* expr = Unary();

        while (Match(TokenType::STAR) || Match(TokenType::SLASH) || Match(TokenType::PERCENT))
        {
            TokenType op = m_previous.type;
            UINT32 line = m_previous.line;
            UINT32 col = m_previous.column;
            Expr* right = Unary();
            expr = MakeBinaryExpr(*m_alloc, expr, op, right, line, col);
        }

        return expr;
    }

    NOINLINE Expr* Unary() noexcept
    {
        if (Match(TokenType::BANG) || Match(TokenType::MINUS))
        {
            TokenType op = m_previous.type;
            UINT32 line = m_previous.line;
            UINT32 col = m_previous.column;
            Expr* operand = Unary();
            return MakeUnaryExpr(*m_alloc, op, operand, line, col);
        }

        return Call();
    }

    NOINLINE Expr* Call() noexcept
    {
        Expr* expr = Primary();

        for (;;)
        {
            if (Match(TokenType::LEFT_PAREN))
            {
                expr = FinishCall(expr);
            }
            else if (Match(TokenType::LEFT_BRACKET))
            {
                UINT32 line = m_previous.line;
                UINT32 col = m_previous.column;
                Expr* index = Expression();
                Consume(TokenType::RIGHT_BRACKET, "Expected ']' after index"_embed);

                Expr* indexExpr = m_alloc->AllocExpr();
                if (indexExpr)
                {
                    indexExpr->type = ExprType::INDEX;
                    indexExpr->line = line;
                    indexExpr->column = col;
                    indexExpr->index.object = expr;
                    indexExpr->index.index = index;
                    expr = indexExpr;
                }
            }
            else
            {
                break;
            }
        }

        return expr;
    }

    NOINLINE Expr* FinishCall(Expr* callee) noexcept
    {
        UINT32 line = m_previous.line;
        UINT32 col = m_previous.column;

        Expr* callExpr = MakeCallExpr(*m_alloc, callee, line, col);
        if (!callExpr) return nullptr;

        if (!Check(TokenType::RIGHT_PAREN))
        {
            do
            {
                if (callExpr->call.argCount >= MAX_CALL_ARGS)
                {
                    ErrorAtCurrent("Too many arguments"_embed);
                    break;
                }
                callExpr->call.args[callExpr->call.argCount++] = Expression();
            } while (Match(TokenType::COMMA));
        }

        Consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments"_embed);

        return callExpr;
    }

    NOINLINE Expr* Primary() noexcept
    {
        UINT32 line = m_current.line;
        UINT32 col = m_current.column;

        if (Match(TokenType::FALSE_))
        {
            return MakeBoolExpr(*m_alloc, FALSE, line, col);
        }

        if (Match(TokenType::TRUE_))
        {
            return MakeBoolExpr(*m_alloc, TRUE, line, col);
        }

        if (Match(TokenType::NIL))
        {
            return MakeNilExpr(*m_alloc, line, col);
        }

        if (Match(TokenType::NUMBER))
        {
            if (m_previous.isFloat)
            {
                // Parse float from string using DOUBLE::Parse()
                DOUBLE value = DOUBLE::Parse(m_previous.value.strValue);
                return MakeFloatExpr(*m_alloc, value, TRUE, line, col);
            }
            else
            {
                // Parse integer from string
                INT64 value = 0;
                for (USIZE i = 0; i < m_previous.length; i++)
                {
                    value = value * 10 + (m_previous.value.strValue[i] - '0');
                }
                return MakeNumberExpr(*m_alloc, value, line, col);
            }
        }

        if (Match(TokenType::STRING))
        {
            return MakeStringExpr(*m_alloc, m_previous.value.strValue, m_previous.length, line, col);
        }

        if (Match(TokenType::IDENTIFIER))
        {
            return MakeIdentifierExpr(*m_alloc, m_previous.value.strValue, m_previous.length, line, col);
        }

        if (Match(TokenType::LEFT_PAREN))
        {
            Expr* expr = Expression();
            Consume(TokenType::RIGHT_PAREN, "Expected ')' after expression"_embed);
            return expr;
        }

        // Array literal: [1, 2, 3]
        if (Match(TokenType::LEFT_BRACKET))
        {
            Expr* arrayExpr = MakeArrayExpr(*m_alloc, line, col);
            if (!arrayExpr) return nullptr;

            // Parse elements
            if (!Check(TokenType::RIGHT_BRACKET))
            {
                do
                {
                    if (arrayExpr->arrayLiteral.elementCount >= MAX_CALL_ARGS)
                    {
                        ErrorAtCurrent("Too many array elements"_embed);
                        break;
                    }
                    arrayExpr->arrayLiteral.elements[arrayExpr->arrayLiteral.elementCount++] = Expression();
                } while (Match(TokenType::COMMA));
            }

            Consume(TokenType::RIGHT_BRACKET, "Expected ']' after array elements"_embed);
            return arrayExpr;
        }

        ErrorAtCurrent("Expected expression"_embed);
        return MakeNilExpr(*m_alloc, line, col);
    }
};

} // namespace script
