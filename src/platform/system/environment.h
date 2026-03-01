/**
 * environment.h - Environment Variable Access
 *
 * Provides access to environment variables.
 * Position-independent, no .rdata dependencies.
 *
 * Part of RUNTIME (Runtime Abstraction Layer).
 */

#pragma once

#include "core.h"

/**
 * Environment - Static class for environment variable access
 */
class Environment
{
public:
    /**
     * GetVariable - Get an environment variable value
     *
     * @param name Variable name (CHAR* for cross-platform, null-terminated)
     * @param buffer Output buffer for the value
     * @param bufferSize Size of the output buffer
     * @return Length of the value, or 0 if not found
     *
     * NOTE: On UEFI, this always returns 0 (no environment variables).
     */
    static USIZE GetVariable(const CHAR* name, Span<CHAR> buffer) noexcept;

    // Prevent instantiation
    VOID* operator new(USIZE) = delete;
    VOID operator delete(VOID*) = delete;
};
