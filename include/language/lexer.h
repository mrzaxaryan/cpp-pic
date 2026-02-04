/**
 * lexer.h - Lexer for PIL (Position Independent Language)
 *
 * Tokenizes source code into a stream of tokens.
 * Position-independent, no .rdata dependencies.
 *
 * Part of RAL (Runtime Abstraction Layer).
 */

#pragma once

#include "token.h"
#include "memory.h"

namespace PIL
{

// ============================================================================
// LEXER CLASS
// ============================================================================

class Lexer
{
private:
    const CHAR* m_source;       // Source code
    USIZE m_sourceLength;       // Length of source code
    USIZE m_current;            // Current position in source
    UINT32 m_line;              // Current line number
    UINT32 m_column;            // Current column number
    UINT32 m_tokenStartColumn;  // Column at start of current token
    BOOL m_hasError;            // Error flag
    CHAR m_errorMessage[128];   // Error message buffer

public:
    // Constructor
    Lexer() noexcept
        : m_source(nullptr)
        , m_sourceLength(0)
        , m_current(0)
        , m_line(1)
        , m_column(1)
        , m_tokenStartColumn(1)
        , m_hasError(FALSE)
    {
        m_errorMessage[0] = '\0';
    }

    // Initialize with source code
    NOINLINE void Init(const CHAR* source, USIZE length) noexcept
    {
        m_source = source;
        m_sourceLength = length;
        m_current = 0;
        m_line = 1;
        m_column = 1;
        m_tokenStartColumn = 1;
        m_hasError = FALSE;
        m_errorMessage[0] = '\0';
    }

    // Get next token
    NOINLINE Token NextToken() noexcept
    {
        SkipWhitespaceAndComments();

        if (IsAtEnd())
        {
            return MakeToken(TokenType::END_OF_FILE);
        }

        m_tokenStartColumn = m_column;
        CHAR c = Advance();

        // Identifiers and keywords
        if (IsAlpha(c))
        {
            return ScanIdentifier();
        }

        // Numbers
        if (IsDigit(c))
        {
            return ScanNumber();
        }

        // String literals
        if (c == '"')
        {
            return ScanString();
        }

        // Single and multi-character tokens
        switch (c)
        {
            case '(': return MakeToken(TokenType::LEFT_PAREN);
            case ')': return MakeToken(TokenType::RIGHT_PAREN);
            case '{': return MakeToken(TokenType::LEFT_BRACE);
            case '}': return MakeToken(TokenType::RIGHT_BRACE);
            case '[': return MakeToken(TokenType::LEFT_BRACKET);
            case ']': return MakeToken(TokenType::RIGHT_BRACKET);
            case ',': return MakeToken(TokenType::COMMA);
            case '.': return MakeToken(TokenType::DOT);
            case ';': return MakeToken(TokenType::SEMICOLON);
            case ':': return MakeToken(TokenType::COLON);
            case '%': return MakeToken(TokenType::PERCENT);

            case '+':
                if (Match('=')) return MakeToken(TokenType::PLUS_EQUAL);
                return MakeToken(TokenType::PLUS);

            case '-':
                if (Match('=')) return MakeToken(TokenType::MINUS_EQUAL);
                return MakeToken(TokenType::MINUS);

            case '*':
                if (Match('=')) return MakeToken(TokenType::STAR_EQUAL);
                return MakeToken(TokenType::STAR);

            case '/':
                if (Match('=')) return MakeToken(TokenType::SLASH_EQUAL);
                return MakeToken(TokenType::SLASH);

            case '=':
                if (Match('=')) return MakeToken(TokenType::EQUAL_EQUAL);
                return MakeToken(TokenType::ASSIGN);

            case '!':
                if (Match('=')) return MakeToken(TokenType::BANG_EQUAL);
                return MakeToken(TokenType::BANG);

            case '<':
                if (Match('=')) return MakeToken(TokenType::LESS_EQUAL);
                return MakeToken(TokenType::LESS);

            case '>':
                if (Match('=')) return MakeToken(TokenType::GREATER_EQUAL);
                return MakeToken(TokenType::GREATER);

            case '&':
                if (Match('&')) return MakeToken(TokenType::AND_AND);
                return MakeErrorToken("Unexpected character '&'"_embed);

            case '|':
                if (Match('|')) return MakeToken(TokenType::OR_OR);
                return MakeErrorToken("Unexpected character '|'"_embed);
        }

        return MakeErrorToken("Unexpected character"_embed);
    }

    // Check if there was an error
    FORCE_INLINE BOOL HasError() const noexcept
    {
        return m_hasError;
    }

    // Get error message
    FORCE_INLINE const CHAR* GetErrorMessage() const noexcept
    {
        return m_errorMessage;
    }

    // Get current line
    FORCE_INLINE UINT32 GetLine() const noexcept
    {
        return m_line;
    }

    // Get current column
    FORCE_INLINE UINT32 GetColumn() const noexcept
    {
        return m_column;
    }

private:
    // ========================================================================
    // CHARACTER HELPERS
    // ========================================================================

    FORCE_INLINE BOOL IsAtEnd() const noexcept
    {
        return m_current >= m_sourceLength;
    }

    FORCE_INLINE CHAR Peek() const noexcept
    {
        if (IsAtEnd()) return '\0';
        return m_source[m_current];
    }

    FORCE_INLINE CHAR PeekNext() const noexcept
    {
        if (m_current + 1 >= m_sourceLength) return '\0';
        return m_source[m_current + 1];
    }

    FORCE_INLINE CHAR Advance() noexcept
    {
        CHAR c = m_source[m_current++];
        m_column++;
        return c;
    }

    FORCE_INLINE BOOL Match(CHAR expected) noexcept
    {
        if (IsAtEnd()) return FALSE;
        if (m_source[m_current] != expected) return FALSE;
        m_current++;
        m_column++;
        return TRUE;
    }

    FORCE_INLINE BOOL IsAlpha(CHAR c) const noexcept
    {
        return (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
               c == '_';
    }

    FORCE_INLINE BOOL IsDigit(CHAR c) const noexcept
    {
        return c >= '0' && c <= '9';
    }

    FORCE_INLINE BOOL IsAlphaNumeric(CHAR c) const noexcept
    {
        return IsAlpha(c) || IsDigit(c);
    }

    // ========================================================================
    // WHITESPACE AND COMMENTS
    // ========================================================================

    NOINLINE void SkipWhitespaceAndComments() noexcept
    {
        for (;;)
        {
            if (IsAtEnd()) return;

            CHAR c = Peek();
            switch (c)
            {
                case ' ':
                case '\t':
                case '\r':
                    Advance();
                    break;

                case '\n':
                    m_line++;
                    m_column = 0; // Will be incremented by Advance
                    Advance();
                    break;

                case '/':
                    if (PeekNext() == '/')
                    {
                        // Single-line comment
                        while (!IsAtEnd() && Peek() != '\n')
                        {
                            Advance();
                        }
                    }
                    else if (PeekNext() == '*')
                    {
                        // Multi-line comment
                        Advance(); // consume '/'
                        Advance(); // consume '*'
                        while (!IsAtEnd())
                        {
                            if (Peek() == '*' && PeekNext() == '/')
                            {
                                Advance(); // consume '*'
                                Advance(); // consume '/'
                                break;
                            }
                            if (Peek() == '\n')
                            {
                                m_line++;
                                m_column = 0;
                            }
                            Advance();
                        }
                    }
                    else
                    {
                        return; // Not a comment, return
                    }
                    break;

                default:
                    return;
            }
        }
    }

    // ========================================================================
    // TOKEN SCANNING
    // ========================================================================

    NOINLINE Token ScanIdentifier() noexcept
    {
        USIZE start = m_current - 1; // Include the first character

        while (!IsAtEnd() && IsAlphaNumeric(Peek()))
        {
            Advance();
        }

        USIZE length = m_current - start;
        if (length >= MAX_TOKEN_LENGTH)
        {
            return MakeErrorToken("Identifier too long"_embed);
        }

        // Check for keywords
        TokenType type = CheckKeyword(start, length);

        Token token(type, m_line, m_tokenStartColumn);

        if (type == TokenType::IDENTIFIER)
        {
            // Copy identifier name
            for (USIZE i = 0; i < length && i < MAX_TOKEN_LENGTH - 1; i++)
            {
                token.value.strValue[i] = m_source[start + i];
            }
            token.value.strValue[length] = '\0';
            token.length = length;
        }

        return token;
    }

    NOINLINE TokenType CheckKeyword(USIZE start, USIZE length) const noexcept
    {
        // Check keywords using first character dispatch and _embed string comparison.
        // Using _embed strings ensures position-independence (no .rodata dependencies).
        switch (m_source[start])
        {
            case 'b':
                if (length == 5 && Memory::Compare(&m_source[start], "break"_embed, 5) == 0)
                    return TokenType::BREAK;
                break;

            case 'c':
                if (length == 8 && Memory::Compare(&m_source[start], "continue"_embed, 8) == 0)
                    return TokenType::CONTINUE;
                break;

            case 'e':
                if (length == 4 && Memory::Compare(&m_source[start], "else"_embed, 4) == 0)
                    return TokenType::ELSE;
                break;

            case 'f':
                if (length == 2 && Memory::Compare(&m_source[start], "fn"_embed, 2) == 0)
                    return TokenType::FN;
                else if (length == 3 && Memory::Compare(&m_source[start], "for"_embed, 3) == 0)
                    return TokenType::FOR;
                else if (length == 5 && Memory::Compare(&m_source[start], "false"_embed, 5) == 0)
                    return TokenType::FALSE_;
                break;

            case 'i':
                if (length == 2 && Memory::Compare(&m_source[start], "if"_embed, 2) == 0)
                    return TokenType::IF;
                else if (length == 2 && Memory::Compare(&m_source[start], "in"_embed, 2) == 0)
                    return TokenType::IN;
                break;

            case 'n':
                if (length == 3 && Memory::Compare(&m_source[start], "nil"_embed, 3) == 0)
                    return TokenType::NIL;
                break;

            case 'r':
                if (length == 6 && Memory::Compare(&m_source[start], "return"_embed, 6) == 0)
                    return TokenType::RETURN;
                break;

            case 't':
                if (length == 4 && Memory::Compare(&m_source[start], "true"_embed, 4) == 0)
                    return TokenType::TRUE_;
                break;

            case 'v':
                if (length == 3 && Memory::Compare(&m_source[start], "var"_embed, 3) == 0)
                    return TokenType::VAR;
                break;

            case 'w':
                if (length == 5 && Memory::Compare(&m_source[start], "while"_embed, 5) == 0)
                    return TokenType::WHILE;
                break;
        }

        return TokenType::IDENTIFIER;
    }

    NOINLINE Token ScanNumber() noexcept
    {
        USIZE start = m_current - 1;
        BOOL hasDecimal = FALSE;

        while (!IsAtEnd() && IsDigit(Peek()))
        {
            Advance();
        }

        // Check for decimal part
        if (Peek() == '.' && IsDigit(PeekNext()))
        {
            hasDecimal = TRUE;
            Advance(); // consume '.'

            while (!IsAtEnd() && IsDigit(Peek()))
            {
                Advance();
            }
        }

        USIZE length = m_current - start;
        if (length >= MAX_TOKEN_LENGTH)
        {
            return MakeErrorToken("Number too long"_embed);
        }

        Token token(TokenType::NUMBER, m_line, m_tokenStartColumn);
        token.isFloat = hasDecimal;

        // Always store as string - parser will convert
        for (USIZE i = 0; i < length && i < MAX_TOKEN_LENGTH - 1; i++)
        {
            token.value.strValue[i] = m_source[start + i];
        }
        token.value.strValue[length] = '\0';
        token.length = length;

        return token;
    }

    NOINLINE Token ScanString() noexcept
    {
        USIZE destIndex = 0;

        Token token(TokenType::STRING, m_line, m_tokenStartColumn);

        while (!IsAtEnd() && Peek() != '"')
        {
            if (Peek() == '\n')
            {
                return MakeErrorToken("Unterminated string (newline)"_embed);
            }

            CHAR c = Advance();

            // Handle escape sequences
            if (c == '\\' && !IsAtEnd())
            {
                CHAR escaped = Advance();
                switch (escaped)
                {
                    case 'n':  c = '\n'; break;
                    case 't':  c = '\t'; break;
                    case 'r':  c = '\r'; break;
                    case '\\': c = '\\'; break;
                    case '"':  c = '"';  break;
                    case '0':  c = '\0'; break;
                    default:
                        return MakeErrorToken("Invalid escape sequence"_embed);
                }
            }

            if (destIndex >= MAX_TOKEN_LENGTH - 1)
            {
                return MakeErrorToken("String too long"_embed);
            }

            token.value.strValue[destIndex++] = c;
        }

        if (IsAtEnd())
        {
            return MakeErrorToken("Unterminated string"_embed);
        }

        Advance(); // Consume closing quote

        token.value.strValue[destIndex] = '\0';
        token.length = destIndex;

        return token;
    }

    // ========================================================================
    // TOKEN CREATION
    // ========================================================================

    FORCE_INLINE Token MakeToken(TokenType type) const noexcept
    {
        return Token(type, m_line, m_tokenStartColumn);
    }

    NOINLINE Token MakeErrorToken(const CHAR* message) noexcept
    {
        m_hasError = TRUE;

        // Copy error message
        USIZE i = 0;
        while (message[i] != '\0' && i < sizeof(m_errorMessage) - 1)
        {
            m_errorMessage[i] = message[i];
            i++;
        }
        m_errorMessage[i] = '\0';

        Token token(TokenType::ERROR, m_line, m_tokenStartColumn);

        // Also store in token
        for (USIZE j = 0; j < i; j++)
        {
            token.value.strValue[j] = message[j];
        }
        token.value.strValue[i] = '\0';
        token.length = i;

        return token;
    }
};

} // namespace PIL
