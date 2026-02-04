/**
 * ast.h - Abstract Syntax Tree for PIL (Position Independent Language)
 *
 * Defines AST node types for expressions and statements.
 * Position-independent, no .rdata dependencies, no dynamic allocation.
 *
 * Part of RAL (Runtime Abstraction Layer).
 */

#pragma once

#include "token.h"
#include "memory.h"
#include "double.h"

namespace PIL
{

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

struct Expr;
struct Stmt;

// ============================================================================
// AST NODE TYPES
// ============================================================================

enum class ExprType : UINT8
{
    NUMBER_LITERAL,     // 42, 3.14
    STRING_LITERAL,     // "hello"
    BOOL_LITERAL,       // true, false
    NIL_LITERAL,        // nil
    ARRAY_LITERAL,      // [1, 2, 3]
    IDENTIFIER,         // foo
    BINARY,             // a + b
    UNARY,              // -a, !b
    CALL,               // foo(a, b)
    ASSIGN,             // a = b
    INDEX,              // arr[i]
    INDEX_ASSIGN,       // arr[i] = value
    LOGICAL,            // a && b, a || b
};

enum class StmtType : UINT8
{
    EXPRESSION,         // expression;
    VAR_DECL,           // var x = expr;
    BLOCK,              // { ... }
    IF,                 // if (cond) { } else { }
    WHILE,              // while (cond) { }
    FOR,                // for (init; cond; incr) { }
    FOR_EACH,           // for (var x in collection) { }
    FUNCTION,           // fn name(params) { }
    RETURN,             // return expr;
    BREAK,              // break;
    CONTINUE,           // continue;
};

// ============================================================================
// CONSTANTS
// ============================================================================

constexpr USIZE MAX_STRING_VALUE = 256;
constexpr USIZE MAX_CALL_ARGS = 16;
constexpr USIZE MAX_FUNCTION_PARAMS = 16;
constexpr USIZE MAX_BLOCK_STMTS = 128;
constexpr USIZE MAX_IDENTIFIER_LENGTH = 64;

// ============================================================================
// EXPRESSION NODES
// ============================================================================

// Number literal: 42, 3.14
struct NumberLiteralExpr
{
    DOUBLE value;       // Stored as DOUBLE (works for both int and float)
    BOOL isFloat;       // TRUE if original literal had decimal point
};

// String literal: "hello"
struct StringLiteralExpr
{
    CHAR value[MAX_STRING_VALUE];
    USIZE length;
};

// Bool literal: true, false
struct BoolLiteralExpr
{
    BOOL value;
};

// Identifier: foo
struct IdentifierExpr
{
    CHAR name[MAX_IDENTIFIER_LENGTH];
    USIZE length;
};

// Binary expression: a + b, a < b
struct BinaryExpr
{
    Expr* left;
    Expr* right;
    TokenType op;
};

// Unary expression: -a, !b
struct UnaryExpr
{
    Expr* operand;
    TokenType op;
};

// Call expression: foo(a, b)
struct CallExpr
{
    Expr* callee;
    Expr* args[MAX_CALL_ARGS];
    UINT8 argCount;
};

// Assignment expression: a = b
struct AssignExpr
{
    CHAR name[MAX_IDENTIFIER_LENGTH];
    USIZE nameLength;
    Expr* value;
};

// Index expression: arr[i]
struct IndexExpr
{
    Expr* object;
    Expr* index;
};

// Logical expression: a && b, a || b
struct LogicalExpr
{
    Expr* left;
    Expr* right;
    TokenType op; // AND_AND or OR_OR
};

// Array literal: [1, 2, 3]
struct ArrayLiteralExpr
{
    Expr* elements[MAX_CALL_ARGS];  // Reuse MAX_CALL_ARGS for max elements
    UINT8 elementCount;
};

// Index assignment: arr[i] = value
struct IndexAssignExpr
{
    Expr* object;   // The array/string being indexed
    Expr* index;    // The index expression
    Expr* value;    // The value to assign
};

// ============================================================================
// EXPRESSION UNION
// ============================================================================

struct Expr
{
    ExprType type;
    UINT32 line;
    UINT32 column;

    union
    {
        NumberLiteralExpr number;
        StringLiteralExpr string;
        BoolLiteralExpr boolean;
        IdentifierExpr identifier;
        BinaryExpr binary;
        UnaryExpr unary;
        CallExpr call;
        AssignExpr assign;
        IndexExpr index;
        IndexAssignExpr indexAssign;
        LogicalExpr logical;
        ArrayLiteralExpr arrayLiteral;
    };

    Expr() noexcept : type(ExprType::NIL_LITERAL), line(0), column(0) {}
};

// ============================================================================
// STATEMENT NODES
// ============================================================================

// Expression statement: expr;
struct ExpressionStmt
{
    Expr* expression;
};

// Variable declaration: var x = expr;
struct VarDeclStmt
{
    CHAR name[MAX_IDENTIFIER_LENGTH];
    USIZE nameLength;
    Expr* initializer; // Can be nullptr
};

// Block statement: { ... }
struct BlockStmt
{
    Stmt* statements[MAX_BLOCK_STMTS];
    USIZE count;
};

// If statement: if (cond) { } else { }
struct IfStmt
{
    Expr* condition;
    Stmt* thenBranch;
    Stmt* elseBranch; // Can be nullptr
};

// While statement: while (cond) { }
struct WhileStmt
{
    Expr* condition;
    Stmt* body;
};

// For statement: for (init; cond; incr) { }
struct ForStmt
{
    Stmt* initializer; // Can be nullptr (var decl or expr stmt)
    Expr* condition;   // Can be nullptr
    Expr* increment;   // Can be nullptr
    Stmt* body;
};

// For-each statement: for (var x in collection) { } or for (var i, x in collection) { }
struct ForEachStmt
{
    CHAR valueName[MAX_IDENTIFIER_LENGTH];  // Loop variable name (the value)
    USIZE valueNameLength;
    CHAR indexName[MAX_IDENTIFIER_LENGTH];  // Optional index variable name
    USIZE indexNameLength;                  // 0 if no index variable
    BOOL hasIndex;                          // TRUE if index variable is present
    Expr* collection;                       // The collection to iterate over
    Stmt* body;
};

// Function declaration: fn name(params) { }
struct FunctionStmt
{
    CHAR name[MAX_IDENTIFIER_LENGTH];
    USIZE nameLength;
    CHAR params[MAX_FUNCTION_PARAMS][MAX_IDENTIFIER_LENGTH];
    USIZE paramLengths[MAX_FUNCTION_PARAMS];
    UINT8 paramCount;
    Stmt* body; // BlockStmt
};

// Return statement: return expr;
struct ReturnStmt
{
    Expr* value; // Can be nullptr
};

// ============================================================================
// STATEMENT UNION
// ============================================================================

struct Stmt
{
    StmtType type;
    UINT32 line;
    UINT32 column;

    union
    {
        ExpressionStmt expression;
        VarDeclStmt varDecl;
        BlockStmt block;
        IfStmt ifStmt;
        WhileStmt whileStmt;
        ForStmt forStmt;
        ForEachStmt forEachStmt;
        FunctionStmt function;
        ReturnStmt returnStmt;
    };

    Stmt() noexcept : type(StmtType::EXPRESSION), line(0), column(0) {}
};

// ============================================================================
// AST ALLOCATOR (Stack-based, fixed-size pool)
// ============================================================================

constexpr USIZE MAX_AST_EXPRS = 512;
constexpr USIZE MAX_AST_STMTS = 256;

class ASTAllocator
{
private:
    Expr m_exprPool[MAX_AST_EXPRS];
    Stmt m_stmtPool[MAX_AST_STMTS];
    USIZE m_exprIndex;
    USIZE m_stmtIndex;

public:
    ASTAllocator() noexcept
        : m_exprIndex(0)
        , m_stmtIndex(0)
    {
    }

    // Reset allocator for reuse
    void Reset() noexcept
    {
        m_exprIndex = 0;
        m_stmtIndex = 0;
    }

    // Allocate a new expression node
    Expr* AllocExpr() noexcept
    {
        if (m_exprIndex >= MAX_AST_EXPRS)
        {
            return nullptr; // Out of memory
        }
        Expr* expr = &m_exprPool[m_exprIndex++];
        Memory::Zero(expr, sizeof(Expr));
        return expr;
    }

    // Allocate a new statement node
    Stmt* AllocStmt() noexcept
    {
        if (m_stmtIndex >= MAX_AST_STMTS)
        {
            return nullptr; // Out of memory
        }
        Stmt* stmt = &m_stmtPool[m_stmtIndex++];
        Memory::Zero(stmt, sizeof(Stmt));
        return stmt;
    }

    // Get usage statistics
    USIZE GetExprCount() const noexcept { return m_exprIndex; }
    USIZE GetStmtCount() const noexcept { return m_stmtIndex; }
};

// ============================================================================
// AST HELPER FUNCTIONS
// ============================================================================

// Create number literal expression (integer)
FORCE_INLINE Expr* MakeNumberExpr(ASTAllocator& alloc, INT64 value, UINT32 line, UINT32 col) noexcept
{
    Expr* expr = alloc.AllocExpr();
    if (!expr) return nullptr;
    expr->type = ExprType::NUMBER_LITERAL;
    expr->line = line;
    expr->column = col;
    // Convert INT64 to DOUBLE (PIC-safe, no native double)
    // May lose precision for values > 2^53
    expr->number.value = DOUBLE(value);
    expr->number.isFloat = FALSE;
    return expr;
}

// Create number literal expression (DOUBLE)
FORCE_INLINE Expr* MakeFloatExpr(ASTAllocator& alloc, DOUBLE value, BOOL isFloat, UINT32 line, UINT32 col) noexcept
{
    Expr* expr = alloc.AllocExpr();
    if (!expr) return nullptr;
    expr->type = ExprType::NUMBER_LITERAL;
    expr->line = line;
    expr->column = col;
    expr->number.value = value;
    expr->number.isFloat = isFloat;
    return expr;
}

// Create string literal expression
FORCE_INLINE Expr* MakeStringExpr(ASTAllocator& alloc, const CHAR* value, USIZE length, UINT32 line, UINT32 col) noexcept
{
    Expr* expr = alloc.AllocExpr();
    if (!expr) return nullptr;
    expr->type = ExprType::STRING_LITERAL;
    expr->line = line;
    expr->column = col;
    expr->string.length = String::Copy(expr->string.value, MAX_STRING_VALUE, value, length);
    return expr;
}

// Create bool literal expression
FORCE_INLINE Expr* MakeBoolExpr(ASTAllocator& alloc, BOOL value, UINT32 line, UINT32 col) noexcept
{
    Expr* expr = alloc.AllocExpr();
    if (!expr) return nullptr;
    expr->type = ExprType::BOOL_LITERAL;
    expr->line = line;
    expr->column = col;
    expr->boolean.value = value;
    return expr;
}

// Create nil literal expression
FORCE_INLINE Expr* MakeNilExpr(ASTAllocator& alloc, UINT32 line, UINT32 col) noexcept
{
    Expr* expr = alloc.AllocExpr();
    if (!expr) return nullptr;
    expr->type = ExprType::NIL_LITERAL;
    expr->line = line;
    expr->column = col;
    return expr;
}

// Create identifier expression
FORCE_INLINE Expr* MakeIdentifierExpr(ASTAllocator& alloc, const CHAR* name, USIZE length, UINT32 line, UINT32 col) noexcept
{
    Expr* expr = alloc.AllocExpr();
    if (!expr) return nullptr;
    expr->type = ExprType::IDENTIFIER;
    expr->line = line;
    expr->column = col;
    expr->identifier.length = String::Copy(expr->identifier.name, MAX_IDENTIFIER_LENGTH, name, length);
    return expr;
}

// Create binary expression
FORCE_INLINE Expr* MakeBinaryExpr(ASTAllocator& alloc, Expr* left, TokenType op, Expr* right, UINT32 line, UINT32 col) noexcept
{
    Expr* expr = alloc.AllocExpr();
    if (!expr) return nullptr;
    expr->type = ExprType::BINARY;
    expr->line = line;
    expr->column = col;
    expr->binary.left = left;
    expr->binary.op = op;
    expr->binary.right = right;
    return expr;
}

// Create unary expression
FORCE_INLINE Expr* MakeUnaryExpr(ASTAllocator& alloc, TokenType op, Expr* operand, UINT32 line, UINT32 col) noexcept
{
    Expr* expr = alloc.AllocExpr();
    if (!expr) return nullptr;
    expr->type = ExprType::UNARY;
    expr->line = line;
    expr->column = col;
    expr->unary.op = op;
    expr->unary.operand = operand;
    return expr;
}

// Create call expression
FORCE_INLINE Expr* MakeCallExpr(ASTAllocator& alloc, Expr* callee, UINT32 line, UINT32 col) noexcept
{
    Expr* expr = alloc.AllocExpr();
    if (!expr) return nullptr;
    expr->type = ExprType::CALL;
    expr->line = line;
    expr->column = col;
    expr->call.callee = callee;
    expr->call.argCount = 0;
    return expr;
}

// Create assignment expression
FORCE_INLINE Expr* MakeAssignExpr(ASTAllocator& alloc, const CHAR* name, USIZE length, Expr* value, UINT32 line, UINT32 col) noexcept
{
    Expr* expr = alloc.AllocExpr();
    if (!expr) return nullptr;
    expr->type = ExprType::ASSIGN;
    expr->line = line;
    expr->column = col;
    expr->assign.nameLength = String::Copy(expr->assign.name, MAX_IDENTIFIER_LENGTH, name, length);
    expr->assign.value = value;
    return expr;
}

// Create logical expression
FORCE_INLINE Expr* MakeLogicalExpr(ASTAllocator& alloc, Expr* left, TokenType op, Expr* right, UINT32 line, UINT32 col) noexcept
{
    Expr* expr = alloc.AllocExpr();
    if (!expr) return nullptr;
    expr->type = ExprType::LOGICAL;
    expr->line = line;
    expr->column = col;
    expr->logical.left = left;
    expr->logical.op = op;
    expr->logical.right = right;
    return expr;
}

// Create array literal expression
FORCE_INLINE Expr* MakeArrayExpr(ASTAllocator& alloc, UINT32 line, UINT32 col) noexcept
{
    Expr* expr = alloc.AllocExpr();
    if (!expr) return nullptr;
    expr->type = ExprType::ARRAY_LITERAL;
    expr->line = line;
    expr->column = col;
    expr->arrayLiteral.elementCount = 0;
    return expr;
}

// Create index assignment expression: arr[i] = value
FORCE_INLINE Expr* MakeIndexAssignExpr(ASTAllocator& alloc, Expr* object, Expr* index, Expr* value, UINT32 line, UINT32 col) noexcept
{
    Expr* expr = alloc.AllocExpr();
    if (!expr) return nullptr;
    expr->type = ExprType::INDEX_ASSIGN;
    expr->line = line;
    expr->column = col;
    expr->indexAssign.object = object;
    expr->indexAssign.index = index;
    expr->indexAssign.value = value;
    return expr;
}

// Create expression statement
FORCE_INLINE Stmt* MakeExprStmt(ASTAllocator& alloc, Expr* expression, UINT32 line, UINT32 col) noexcept
{
    Stmt* stmt = alloc.AllocStmt();
    if (!stmt) return nullptr;
    stmt->type = StmtType::EXPRESSION;
    stmt->line = line;
    stmt->column = col;
    stmt->expression.expression = expression;
    return stmt;
}

// Create variable declaration statement
FORCE_INLINE Stmt* MakeVarDeclStmt(ASTAllocator& alloc, const CHAR* name, USIZE length, Expr* init, UINT32 line, UINT32 col) noexcept
{
    Stmt* stmt = alloc.AllocStmt();
    if (!stmt) return nullptr;
    stmt->type = StmtType::VAR_DECL;
    stmt->line = line;
    stmt->column = col;
    stmt->varDecl.nameLength = String::Copy(stmt->varDecl.name, MAX_IDENTIFIER_LENGTH, name, length);
    stmt->varDecl.initializer = init;
    return stmt;
}

// Create block statement
FORCE_INLINE Stmt* MakeBlockStmt(ASTAllocator& alloc, UINT32 line, UINT32 col) noexcept
{
    Stmt* stmt = alloc.AllocStmt();
    if (!stmt) return nullptr;
    stmt->type = StmtType::BLOCK;
    stmt->line = line;
    stmt->column = col;
    stmt->block.count = 0;
    return stmt;
}

// Create if statement
FORCE_INLINE Stmt* MakeIfStmt(ASTAllocator& alloc, Expr* cond, Stmt* thenBr, Stmt* elseBr, UINT32 line, UINT32 col) noexcept
{
    Stmt* stmt = alloc.AllocStmt();
    if (!stmt) return nullptr;
    stmt->type = StmtType::IF;
    stmt->line = line;
    stmt->column = col;
    stmt->ifStmt.condition = cond;
    stmt->ifStmt.thenBranch = thenBr;
    stmt->ifStmt.elseBranch = elseBr;
    return stmt;
}

// Create while statement
FORCE_INLINE Stmt* MakeWhileStmt(ASTAllocator& alloc, Expr* cond, Stmt* body, UINT32 line, UINT32 col) noexcept
{
    Stmt* stmt = alloc.AllocStmt();
    if (!stmt) return nullptr;
    stmt->type = StmtType::WHILE;
    stmt->line = line;
    stmt->column = col;
    stmt->whileStmt.condition = cond;
    stmt->whileStmt.body = body;
    return stmt;
}

// Create for-each statement
FORCE_INLINE Stmt* MakeForEachStmt(ASTAllocator& alloc, const CHAR* valueName, USIZE valueLen,
                                    const CHAR* indexName, USIZE indexLen, BOOL hasIndex,
                                    Expr* collection, Stmt* body, UINT32 line, UINT32 col) noexcept
{
    Stmt* stmt = alloc.AllocStmt();
    if (!stmt) return nullptr;
    stmt->type = StmtType::FOR_EACH;
    stmt->line = line;
    stmt->column = col;

    // Copy value name
    stmt->forEachStmt.valueNameLength = String::Copy(stmt->forEachStmt.valueName, MAX_IDENTIFIER_LENGTH, valueName, valueLen);

    // Copy index name if present
    stmt->forEachStmt.hasIndex = hasIndex;
    if (hasIndex && indexName)
    {
        stmt->forEachStmt.indexNameLength = String::Copy(stmt->forEachStmt.indexName, MAX_IDENTIFIER_LENGTH, indexName, indexLen);
    }
    else
    {
        stmt->forEachStmt.indexName[0] = '\0';
        stmt->forEachStmt.indexNameLength = 0;
    }

    stmt->forEachStmt.collection = collection;
    stmt->forEachStmt.body = body;
    return stmt;
}

// Create function declaration statement
FORCE_INLINE Stmt* MakeFunctionStmt(ASTAllocator& alloc, const CHAR* name, USIZE length, UINT32 line, UINT32 col) noexcept
{
    Stmt* stmt = alloc.AllocStmt();
    if (!stmt) return nullptr;
    stmt->type = StmtType::FUNCTION;
    stmt->line = line;
    stmt->column = col;
    stmt->function.nameLength = String::Copy(stmt->function.name, MAX_IDENTIFIER_LENGTH, name, length);
    stmt->function.paramCount = 0;
    stmt->function.body = nullptr;
    return stmt;
}

// Create return statement
FORCE_INLINE Stmt* MakeReturnStmt(ASTAllocator& alloc, Expr* value, UINT32 line, UINT32 col) noexcept
{
    Stmt* stmt = alloc.AllocStmt();
    if (!stmt) return nullptr;
    stmt->type = StmtType::RETURN;
    stmt->line = line;
    stmt->column = col;
    stmt->returnStmt.value = value;
    return stmt;
}

// Create break statement
FORCE_INLINE Stmt* MakeBreakStmt(ASTAllocator& alloc, UINT32 line, UINT32 col) noexcept
{
    Stmt* stmt = alloc.AllocStmt();
    if (!stmt) return nullptr;
    stmt->type = StmtType::BREAK;
    stmt->line = line;
    stmt->column = col;
    return stmt;
}

// Create continue statement
FORCE_INLINE Stmt* MakeContinueStmt(ASTAllocator& alloc, UINT32 line, UINT32 col) noexcept
{
    Stmt* stmt = alloc.AllocStmt();
    if (!stmt) return nullptr;
    stmt->type = StmtType::CONTINUE;
    stmt->line = line;
    stmt->column = col;
    return stmt;
}

} // namespace PIL
