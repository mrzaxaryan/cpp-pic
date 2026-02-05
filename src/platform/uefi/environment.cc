/**
 * environment.cc - UEFI Environment Variable Stub
 *
 * UEFI does not have traditional environment variables.
 * This stub always returns empty/not found.
 */

#include "environment.h"

USIZE Environment::GetVariable(const CHAR* name, CHAR* buffer, USIZE bufferSize) noexcept
{
    (void)name;  // Unused

    if (buffer != nullptr && bufferSize > 0)
    {
        buffer[0] = '\0';
    }

    return 0;
}
