/**
 * environment.cc - macOS Environment Variable Implementation
 *
 * macOS does not have /proc/self/environ.
 * This is a stub implementation that returns 0 (not found) for all variables.
 * Future enhancement: use sysctl(kern.procargs2) to read process environment.
 */

#include "environment.h"

USIZE Environment::GetVariable(const CHAR* name, CHAR* buffer, USIZE bufferSize) noexcept
{
	if (name == nullptr || buffer == nullptr || bufferSize == 0)
	{
		return 0;
	}

	// macOS has no /proc filesystem. Environment variables are not accessible
	// via a simple file read in freestanding mode.
	// Return empty result.
	buffer[0] = '\0';
	return 0;
}
