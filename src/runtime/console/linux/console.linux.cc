/**
 * console.linux.cc - Linux Console Output Implementation
 */

#if defined(PLATFORM_LINUX)

#include "console.h"
#include "linux/syscall.h"
#include "string.h"
#include "memory.h"

/**
 * Console::Write (CHAR*) - Write narrow string to stdout
 *
 * Writes a string of narrow (8-bit) characters to standard output.
 * On Linux, this is a direct pass-through to the write() syscall.
 */
UINT32 Console::Write(const CHAR *output, USIZE outputLength)
{
    /* Invoke write syscall: write(STDOUT_FILENO, output, outputLength) */
    SSIZE result = Syscall::Write(STDOUT_FILENO, output, outputLength);

    /* Linux syscalls return negative errno on error */
    if (result < 0)
        return 0;

    return (UINT32)result;
}

/**
 * Console::Write (WCHAR*) - Write wide string to stdout
 *
 * Writes a string of wide (16-bit) characters to standard output.
 * Since Linux terminals expect UTF-8, we convert UTF-16 to UTF-8
 * using String::WideToUtf8 before writing.
 */
UINT32 Console::Write(const WCHAR *text, USIZE wcharCount)
{
    /* Validate input parameters */
    if (!text || !wcharCount)
        return 0;

    /*
     * Chunk size for processing
     *
     * 256 WCHARs * 3 bytes/WCHAR max = 768 bytes UTF-8 max
     * This fits comfortably in a 1024-byte buffer with margin.
     */
    const USIZE CHUNK_SIZE = 256;

    /* Temporary buffer for current chunk (+1 for null terminator) */
    WCHAR chunk[CHUNK_SIZE + 1];

    /* UTF-8 output buffer (3x chunk size for worst case + margin) */
    CHAR utf8Buffer[1024];

    /* Total characters written across all chunks */
    UINT32 totalWritten = 0;

    /*
     * Process input in chunks
     */
    while (wcharCount > 0)
    {
        /* Determine current chunk size */
        USIZE currentChunk = (wcharCount > CHUNK_SIZE) ? CHUNK_SIZE : wcharCount;

        /* Copy chunk to temporary buffer (byte count = WCHAR count * sizeof(WCHAR)) */
        Memory::Copy(chunk, text, currentChunk * sizeof(WCHAR));

        /* Null-terminate for String::WideToUtf8 */
        chunk[currentChunk] = 0;

        /* Convert to UTF-8 */
        USIZE utf8Len = String::WideToUtf8(chunk, utf8Buffer, sizeof(utf8Buffer));

        /* Write UTF-8 buffer to stdout */
        totalWritten += Write(utf8Buffer, utf8Len);

        /* Advance to next chunk */
        text += currentChunk;
        wcharCount -= currentChunk;
    }

    return totalWritten;
}

#endif /* PLATFORM_LINUX */
