/**
 * token.h - Token Types for PIL (Position Independent Language)
 *
 * Defines token types and Token struct for the lexer.
 * Position-independent, no .rdata dependencies.
 *
 * Part of RAL (Runtime Abstraction Layer).
 */

#pragma once

#include "pir/core/types/primitives.h"
#include "pir/core/types/embedded/embedded_string.h"
#include "pir/core/string/string.h"  // String utilities

namespace script
{

// ============================================================================
// TOKEN TYPES
// ============================================================================

enum class TokenType : UINT8
{
    // Literals
    NUMBER,         // 123, 3.14
    STRING,         // "hello"
    IDENTIFIER,     // foo, bar

    // Keywords
    VAR,            // var
    FN,             // fn
    IF,             // if
    ELSE,           // else
    WHILE,          // while
    FOR,            // for
    IN,             // in
    RETURN,         // return
    BREAK,          // break
    CONTINUE,       // continue
    TRUE_,          // true
    FALSE_,         // false
    NIL,            // nil

    // Single-character tokens
    LEFT_PAREN,     // (
    RIGHT_PAREN,    // )
    LEFT_BRACE,     // {
    RIGHT_BRACE,    // }
    LEFT_BRACKET,   // [
    RIGHT_BRACKET,  // ]
    COMMA,          // ,
    DOT,            // .
    SEMICOLON,      // ;
    COLON,          // :

    // Operators
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /
    PERCENT,        // %
    ASSIGN,         // =
    BANG,           // !
    LESS,           // <
    GREATER,        // >

    // Two-character operators
    EQUAL_EQUAL,    // ==
    BANG_EQUAL,     // !=
    LESS_EQUAL,     // <=
    GREATER_EQUAL,  // >=
    AND_AND,        // &&
    OR_OR,          // ||
    PLUS_EQUAL,     // +=
    MINUS_EQUAL,    // -=
    STAR_EQUAL,     // *=
    SLASH_EQUAL,    // /=

    // Special
    END_OF_FILE,    // end of input
    ERROR           // lexer error
};

// ============================================================================
// TOKEN VALUE STORAGE
// ============================================================================

// Maximum length for string/identifier tokens
constexpr USIZE MAX_TOKEN_LENGTH = 256;

// Token value union (no dynamic allocation)
union TokenValue
{
    INT64 intValue;                     // Integer literal
    CHAR strValue[MAX_TOKEN_LENGTH];    // String/identifier (stack allocated)

    TokenValue() noexcept : intValue(0) {}
};

// ============================================================================
// TOKEN STRUCT
// ============================================================================

struct Token
{
    TokenType type;
    TokenValue value;
    UINT32 line;
    UINT32 column;
    USIZE length;   // Length of string/identifier
    BOOL isFloat;   // TRUE if number has decimal point

    Token() noexcept
        : type(TokenType::END_OF_FILE)
        , line(1)
        , column(1)
        , length(0)
        , isFloat(FALSE)
    {
    }

    Token(TokenType t, UINT32 ln, UINT32 col) noexcept
        : type(t)
        , line(ln)
        , column(col)
        , length(0)
        , isFloat(FALSE)
    {
    }

    // Check if token is a specific type
    FORCE_INLINE BOOL Is(TokenType t) const noexcept
    {
        return type == t;
    }

    // Check if token is one of several types
    FORCE_INLINE BOOL IsOneOf(TokenType t1, TokenType t2) const noexcept
    {
        return type == t1 || type == t2;
    }

    // Check if token is an error
    FORCE_INLINE BOOL IsError() const noexcept
    {
        return type == TokenType::ERROR;
    }

    // Check if token is end of file
    FORCE_INLINE BOOL IsEOF() const noexcept
    {
        return type == TokenType::END_OF_FILE;
    }

    // Check if token is a literal
    FORCE_INLINE BOOL IsLiteral() const noexcept
    {
        return type == TokenType::NUMBER ||
               type == TokenType::STRING ||
               type == TokenType::TRUE_ ||
               type == TokenType::FALSE_ ||
               type == TokenType::NIL;
    }

    // Check if token is a comparison operator
    FORCE_INLINE BOOL IsComparison() const noexcept
    {
        return type == TokenType::LESS ||
               type == TokenType::GREATER ||
               type == TokenType::LESS_EQUAL ||
               type == TokenType::GREATER_EQUAL ||
               type == TokenType::EQUAL_EQUAL ||
               type == TokenType::BANG_EQUAL;
    }
};

// ============================================================================
// TOKEN TYPE NAMES (for debugging)
// ============================================================================

// Writes the token type name to the provided buffer (PIC-safe, no .rdata)
// Buffer should be at least 16 bytes
// Returns the length of the string written
NOINLINE USIZE GetTokenTypeName(TokenType type, CHAR* buffer, USIZE bufferSize) noexcept
{
    if (!buffer || bufferSize < 2) return 0;

    switch (type)
    {
        case TokenType::NUMBER:        return String::CopyEmbed("NUMBER"_embed, buffer, bufferSize);
        case TokenType::STRING:        return String::CopyEmbed("STRING"_embed, buffer, bufferSize);
        case TokenType::IDENTIFIER:    return String::CopyEmbed("IDENTIFIER"_embed, buffer, bufferSize);
        case TokenType::VAR:           return String::CopyEmbed("VAR"_embed, buffer, bufferSize);
        case TokenType::FN:            return String::CopyEmbed("FN"_embed, buffer, bufferSize);
        case TokenType::IF:            return String::CopyEmbed("IF"_embed, buffer, bufferSize);
        case TokenType::ELSE:          return String::CopyEmbed("ELSE"_embed, buffer, bufferSize);
        case TokenType::WHILE:         return String::CopyEmbed("WHILE"_embed, buffer, bufferSize);
        case TokenType::FOR:           return String::CopyEmbed("FOR"_embed, buffer, bufferSize);
        case TokenType::IN:            return String::CopyEmbed("IN"_embed, buffer, bufferSize);
        case TokenType::RETURN:        return String::CopyEmbed("RETURN"_embed, buffer, bufferSize);
        case TokenType::BREAK:         return String::CopyEmbed("BREAK"_embed, buffer, bufferSize);
        case TokenType::CONTINUE:      return String::CopyEmbed("CONTINUE"_embed, buffer, bufferSize);
        case TokenType::TRUE_:         return String::CopyEmbed("TRUE"_embed, buffer, bufferSize);
        case TokenType::FALSE_:        return String::CopyEmbed("FALSE"_embed, buffer, bufferSize);
        case TokenType::NIL:           return String::CopyEmbed("NIL"_embed, buffer, bufferSize);
        case TokenType::LEFT_PAREN:    return String::CopyEmbed("LEFT_PAREN"_embed, buffer, bufferSize);
        case TokenType::RIGHT_PAREN:   return String::CopyEmbed("RIGHT_PAREN"_embed, buffer, bufferSize);
        case TokenType::LEFT_BRACE:    return String::CopyEmbed("LEFT_BRACE"_embed, buffer, bufferSize);
        case TokenType::RIGHT_BRACE:   return String::CopyEmbed("RIGHT_BRACE"_embed, buffer, bufferSize);
        case TokenType::LEFT_BRACKET:  return String::CopyEmbed("LEFT_BRACKET"_embed, buffer, bufferSize);
        case TokenType::RIGHT_BRACKET: return String::CopyEmbed("RIGHT_BRACKET"_embed, buffer, bufferSize);
        case TokenType::COMMA:         return String::CopyEmbed("COMMA"_embed, buffer, bufferSize);
        case TokenType::DOT:           return String::CopyEmbed("DOT"_embed, buffer, bufferSize);
        case TokenType::SEMICOLON:     return String::CopyEmbed("SEMICOLON"_embed, buffer, bufferSize);
        case TokenType::COLON:         return String::CopyEmbed("COLON"_embed, buffer, bufferSize);
        case TokenType::PLUS:          return String::CopyEmbed("PLUS"_embed, buffer, bufferSize);
        case TokenType::MINUS:         return String::CopyEmbed("MINUS"_embed, buffer, bufferSize);
        case TokenType::STAR:          return String::CopyEmbed("STAR"_embed, buffer, bufferSize);
        case TokenType::SLASH:         return String::CopyEmbed("SLASH"_embed, buffer, bufferSize);
        case TokenType::PERCENT:       return String::CopyEmbed("PERCENT"_embed, buffer, bufferSize);
        case TokenType::ASSIGN:        return String::CopyEmbed("ASSIGN"_embed, buffer, bufferSize);
        case TokenType::BANG:          return String::CopyEmbed("BANG"_embed, buffer, bufferSize);
        case TokenType::LESS:          return String::CopyEmbed("LESS"_embed, buffer, bufferSize);
        case TokenType::GREATER:       return String::CopyEmbed("GREATER"_embed, buffer, bufferSize);
        case TokenType::EQUAL_EQUAL:   return String::CopyEmbed("EQUAL_EQUAL"_embed, buffer, bufferSize);
        case TokenType::BANG_EQUAL:    return String::CopyEmbed("BANG_EQUAL"_embed, buffer, bufferSize);
        case TokenType::LESS_EQUAL:    return String::CopyEmbed("LESS_EQUAL"_embed, buffer, bufferSize);
        case TokenType::GREATER_EQUAL: return String::CopyEmbed("GREATER_EQUAL"_embed, buffer, bufferSize);
        case TokenType::AND_AND:       return String::CopyEmbed("AND_AND"_embed, buffer, bufferSize);
        case TokenType::OR_OR:         return String::CopyEmbed("OR_OR"_embed, buffer, bufferSize);
        case TokenType::PLUS_EQUAL:    return String::CopyEmbed("PLUS_EQUAL"_embed, buffer, bufferSize);
        case TokenType::MINUS_EQUAL:   return String::CopyEmbed("MINUS_EQUAL"_embed, buffer, bufferSize);
        case TokenType::STAR_EQUAL:    return String::CopyEmbed("STAR_EQUAL"_embed, buffer, bufferSize);
        case TokenType::SLASH_EQUAL:   return String::CopyEmbed("SLASH_EQUAL"_embed, buffer, bufferSize);
        case TokenType::END_OF_FILE:   return String::CopyEmbed("EOF"_embed, buffer, bufferSize);
        case TokenType::ERROR:         return String::CopyEmbed("ERROR"_embed, buffer, bufferSize);
        default:                       return String::CopyEmbed("UNKNOWN"_embed, buffer, bufferSize);
    }
}

} // namespace script
