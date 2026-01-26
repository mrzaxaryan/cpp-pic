/**
 * platform.linux.cc - Linux Platform Core Functions
 *
 * Implements platform-specific functions common across all Linux architectures
 * (x86_64, i386, aarch64, armv7a).
 */

#if defined(PLATFORM_LINUX)

#include "linux/syscall.h"

/**
 * ExitProcess - Terminate the current process
 *
 * Linux equivalent of Windows' ExitProcess() / NtTerminateProcess().
 *
 * @param code - Exit status code (0 = success, 1-255 = error)
 *
 * Syscall numbers by architecture:
 *   x86_64: 60, i386: 1, aarch64: 93, armv7a: 1
 *
 * Note: Uses exit syscall (terminates calling thread only).
 * For single-threaded programs, equivalent to exit_group.
 */
NO_RETURN VOID ExitProcess(USIZE code)
{
    Syscall::Exit((INT32)code);
    __builtin_unreachable();
}

#endif /* PLATFORM_LINUX */
