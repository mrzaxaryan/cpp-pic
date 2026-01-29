#include "console.h"
#include "syscall.h"
#include "primitives.h"

// Linux stdout file descriptor
#define STDOUT_FILENO 1

// Linux syscall numbers for this file
#if defined(ARCHITECTURE_X86_64)
constexpr USIZE SYS_WRITE = 1;
#elif defined(ARCHITECTURE_I386)
constexpr USIZE SYS_WRITE = 4;
#elif defined(ARCHITECTURE_AARCH64)
constexpr USIZE SYS_WRITE = 64;
#elif defined(ARCHITECTURE_ARMV7A)
constexpr USIZE SYS_WRITE = 4;
#endif

// Write ANSI/ASCII string to console (straightforward)
UINT32 Console::Write(const CHAR *text, USIZE length)
{
    SSIZE result = Syscall::syscall3(SYS_WRITE, STDOUT_FILENO, (USIZE)text, length);
    return (result >= 0) ? (UINT32)result : 0;
}

// Write wide string to console (convert UTF-16 to UTF-8 first)
UINT32 Console::Write(const WCHAR *text, USIZE length)
{
    // UTF-16 to UTF-8 conversion buffer (stack-allocated)
    // Each UTF-16 character can become up to 4 UTF-8 bytes
    // We'll use a reasonable buffer size and process in chunks if needed
    constexpr USIZE BUFFER_SIZE = 1024;
    CHAR utf8Buffer[BUFFER_SIZE];
    UINT32 totalWritten = 0;

    for (USIZE i = 0; i < length; )
    {
        USIZE utf8Pos = 0;

        // Convert as many characters as fit in buffer
        while (i < length && utf8Pos < BUFFER_SIZE - 4)
        {
            UINT32 codepoint = text[i++];

            // Handle UTF-16 surrogate pairs
            if (codepoint >= 0xD800 && codepoint <= 0xDBFF && i < length)
            {
                UINT32 low = text[i];
                if (low >= 0xDC00 && low <= 0xDFFF)
                {
                    codepoint = 0x10000 + ((codepoint & 0x3FF) << 10) + (low & 0x3FF);
                    i++;
                }
            }

            // Convert codepoint to UTF-8
            if (codepoint < 0x80)
            {
                // 1-byte sequence (ASCII)
                utf8Buffer[utf8Pos++] = (CHAR)codepoint;
            }
            else if (codepoint < 0x800)
            {
                // 2-byte sequence
                utf8Buffer[utf8Pos++] = (CHAR)(0xC0 | (codepoint >> 6));
                utf8Buffer[utf8Pos++] = (CHAR)(0x80 | (codepoint & 0x3F));
            }
            else if (codepoint < 0x10000)
            {
                // 3-byte sequence
                utf8Buffer[utf8Pos++] = (CHAR)(0xE0 | (codepoint >> 12));
                utf8Buffer[utf8Pos++] = (CHAR)(0x80 | ((codepoint >> 6) & 0x3F));
                utf8Buffer[utf8Pos++] = (CHAR)(0x80 | (codepoint & 0x3F));
            }
            else if (codepoint < 0x110000)
            {
                // 4-byte sequence
                utf8Buffer[utf8Pos++] = (CHAR)(0xF0 | (codepoint >> 18));
                utf8Buffer[utf8Pos++] = (CHAR)(0x80 | ((codepoint >> 12) & 0x3F));
                utf8Buffer[utf8Pos++] = (CHAR)(0x80 | ((codepoint >> 6) & 0x3F));
                utf8Buffer[utf8Pos++] = (CHAR)(0x80 | (codepoint & 0x3F));
            }
        }

        // Write the converted UTF-8 buffer
        if (utf8Pos > 0)
        {
            SSIZE written = Syscall::syscall3(SYS_WRITE, STDOUT_FILENO, (USIZE)utf8Buffer, utf8Pos);
            if (written > 0)
                totalWritten += written;
        }
    }

    return totalWritten;
}
