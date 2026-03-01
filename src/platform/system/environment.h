/**
 * @file environment.h
 * @brief Environment variable access
 *
 * @details Provides position-independent access to environment variables across
 * platforms. On Windows, variables are read from the PEB environment block. On
 * Linux, macOS, and Solaris, variables are retrieved from the process environ
 * pointer. On UEFI, GetVariable() always returns 0 as environment variables are
 * not available. No .rdata dependencies.
 */

#pragma once

#include "core/core.h"

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
