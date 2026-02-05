/**
 * environment.cc - Linux Environment Variable Implementation
 *
 * Reads environment variables from /proc/self/environ.
 * Position-independent, no .rdata dependencies.
 */

#include "environment.h"
#include "syscall.h"
#include "system.h"
#include "memory.h"

// Helper to compare strings (case-sensitive for Linux)
static BOOL CompareEnvName(const CHAR* envEntry, const CHAR* name) noexcept
{
    while (*name != '\0')
    {
        if (*envEntry != *name)
        {
            return FALSE;
        }
        envEntry++;
        name++;
    }

    // After name, should be '='
    return *envEntry == '=';
}

USIZE Environment::GetVariable(const CHAR* name, CHAR* buffer, USIZE bufferSize) noexcept
{
    if (name == nullptr || buffer == nullptr || bufferSize == 0)
    {
        return 0;
    }

    // Open /proc/self/environ
    const CHAR* procEnvPath = "/proc/self/environ"_embed;
#if defined(ARCHITECTURE_AARCH64)
    // aarch64 only has openat syscall
    SSIZE fd = System::Call(SYS_OPENAT, (USIZE)-100, (USIZE)procEnvPath, 0, 0);
    if (fd < 0)
    {
        buffer[0] = '\0';
        return 0;
    }
#else
    SSIZE fd = System::Call(SYS_OPEN, (USIZE)procEnvPath, 0 /* O_RDONLY */, 0);
    if (fd < 0)
    {
        // Try openat with AT_FDCWD (-100) for newer kernels
        fd = System::Call(SYS_OPENAT, (USIZE)-100, (USIZE)procEnvPath, 0, 0);
        if (fd < 0)
        {
            buffer[0] = '\0';
            return 0;
        }
    }
#endif

    // Read environment block (entries separated by null bytes)
    CHAR envBuf[4096];
    SSIZE bytesRead = System::Call(SYS_READ, (USIZE)fd, (USIZE)envBuf, sizeof(envBuf) - 1);
    System::Call(SYS_CLOSE, (USIZE)fd);

    if (bytesRead <= 0)
    {
        buffer[0] = '\0';
        return 0;
    }

    envBuf[bytesRead] = '\0';

    // Search for the variable
    const CHAR* ptr = envBuf;
    const CHAR* end = envBuf + bytesRead;

    while (ptr < end && *ptr != '\0')
    {
        if (CompareEnvName(ptr, name))
        {
            // Find the '=' and skip past it
            const CHAR* value = ptr;
            while (*value != '=' && *value != '\0')
            {
                value++;
            }
            if (*value == '=')
            {
                value++;  // Skip the '='

                // Copy value to buffer
                USIZE len = 0;
                while (*value != '\0' && len < bufferSize - 1)
                {
                    buffer[len++] = *value++;
                }
                buffer[len] = '\0';
                return len;
            }
        }

        // Skip to next entry (after null terminator)
        while (ptr < end && *ptr != '\0')
        {
            ptr++;
        }
        ptr++;  // Skip the null terminator
    }

    // Variable not found
    buffer[0] = '\0';
    return 0;
}
