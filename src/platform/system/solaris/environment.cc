/**
 * environment.cc - Solaris/illumos Environment Variable Implementation
 *
 * Solaris does not have /proc/self/environ.
 * This is a stub implementation that returns 0 (not found) for all variables.
 * Future enhancement: use /proc/self/psinfo + /proc/self/as to read process
 * environment, or walk the initial stack to find envp.
 */

#include "platform/system/environment.h"

USIZE Environment::GetVariable(const CHAR *name, Span<CHAR> buffer) noexcept
{
	if (name == nullptr || buffer.Size() == 0)
	{
		return 0;
	}

	// Solaris has no /proc/self/environ. Environment variables are not
	// accessible via a simple file read in freestanding mode.
	// Return empty result.
	buffer[0] = '\0';
	return 0;
}
