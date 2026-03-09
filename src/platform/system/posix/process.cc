/**
 * @file process.cc
 * @brief Shared POSIX process management implementation
 *
 * @details Cross-platform process creation via fork+execve with optional I/O
 * redirection, plus wait4/kill for lifecycle management. Platform differences:
 * - Linux aarch64/riscv: clone(SIGCHLD) for fork, dup3 for dup2
 * - Solaris: SYS_forksys for fork, fcntl(F_DUP2FD) for dup2,
 *   SYS_pgrpsys for setsid, waitid instead of wait4
 * - macOS/FreeBSD: standard POSIX syscalls
 */

#include "platform/system/process.h"
#if defined(PLATFORM_LINUX)
#include "platform/kernel/linux/syscall.h"
#include "platform/kernel/linux/system.h"
#elif defined(PLATFORM_ANDROID)
#include "platform/kernel/android/syscall.h"
#include "platform/kernel/android/system.h"
#elif defined(PLATFORM_MACOS)
#include "platform/kernel/macos/syscall.h"
#include "platform/kernel/macos/system.h"
#elif defined(PLATFORM_IOS)
#include "platform/kernel/ios/syscall.h"
#include "platform/kernel/ios/system.h"
#elif defined(PLATFORM_SOLARIS)
#include "platform/kernel/solaris/syscall.h"
#include "platform/kernel/solaris/system.h"
#elif defined(PLATFORM_FREEBSD)
#include "platform/kernel/freebsd/syscall.h"
#include "platform/kernel/freebsd/system.h"
#endif

#include "core/memory/memory.h"

// ============================================================================
// Local POSIX helpers (not exposed in header)
// ============================================================================

static SSIZE PosixFork() noexcept
{
#if (defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)) && (defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_RISCV32))
	constexpr USIZE SIGCHLD_VAL = 17;
	return System::Call(SYS_CLONE, SIGCHLD_VAL, 0, 0, 0, 0);
#elif defined(PLATFORM_SOLARIS)
	return System::Call(SYS_FORKSYS, FORKSYS_FORK, 0);
#else
	return System::Call(SYS_FORK);
#endif
}

static SSIZE PosixDup2(SSIZE oldfd, SSIZE newfd) noexcept
{
#if (defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)) && (defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_RISCV32))
	return System::Call(SYS_DUP3, (USIZE)oldfd, (USIZE)newfd, 0);
#elif defined(PLATFORM_SOLARIS)
	return System::Call(SYS_FCNTL, (USIZE)oldfd, (USIZE)F_DUP2FD, (USIZE)newfd);
#else
	return System::Call(SYS_DUP2, (USIZE)oldfd, (USIZE)newfd);
#endif
}

static SSIZE PosixSetsid() noexcept
{
#if defined(PLATFORM_SOLARIS)
	return System::Call(SYS_PGRPSYS, PGRPSYS_SETSID);
#else
	return System::Call(SYS_SETSID);
#endif
}

// ============================================================================
// Process::Create
// ============================================================================

Result<Process, Error> Process::Create(
	const CHAR *path,
	const CHAR *const args[],
	SSIZE stdinFd,
	SSIZE stdoutFd,
	SSIZE stderrFd) noexcept
{
	if (path == nullptr || args == nullptr)
	{
		return Result<Process, Error>::Err(Error::Process_CreateFailed);
	}

	SSIZE pid = PosixFork();
	if (pid < 0)
	{
		return Result<Process, Error>::Err(
			Error::Posix((UINT32)(-pid)), Error::Process_CreateFailed);
	}

	if (pid == 0)
	{
		// Child process
		BOOL hasRedirect = (stdinFd != -1 || stdoutFd != -1 || stderrFd != -1);

		if (hasRedirect)
		{
			// Detach from controlling terminal when redirecting I/O
			(void)PosixSetsid();

			// Redirect each fd that was specified
			if (stdinFd != -1 && PosixDup2(stdinFd, STDIN_FILENO) < 0)
				System::Call(SYS_EXIT, 1);
			if (stdoutFd != -1 && PosixDup2(stdoutFd, STDOUT_FILENO) < 0)
				System::Call(SYS_EXIT, 1);
			if (stderrFd != -1 && PosixDup2(stderrFd, STDERR_FILENO) < 0)
				System::Call(SYS_EXIT, 1);

			// Close original fds if they are not standard fds
			// (avoid closing if they were redirected to a standard fd)
			SSIZE fds[3] = {stdinFd, stdoutFd, stderrFd};
			for (INT32 i = 0; i < 3; ++i)
			{
				if (fds[i] > STDERR_FILENO)
				{
					// Check not already used as another redirect target
					BOOL stillNeeded = false;
					for (INT32 j = i + 1; j < 3; ++j)
					{
						if (fds[j] == fds[i])
						{
							stillNeeded = true;
							break;
						}
					}
					if (!stillNeeded)
						System::Call(SYS_CLOSE, (USIZE)fds[i]);
				}
			}
		}

		// Build envp (empty environment)
		USIZE envp[1];
		envp[0] = 0;

		// Execute — does not return on success
		System::Call(SYS_EXECVE, (USIZE)path, (USIZE)args, (USIZE)envp);

		// If execve returned, exit child
		System::Call(SYS_EXIT, 1);
	}

	// Parent — return Process with child PID
	return Result<Process, Error>::Ok(Process(pid));
}

// ============================================================================
// Process::Wait
// ============================================================================

Result<SSIZE, Error> Process::Wait() noexcept
{
	if (!IsValid())
		return Result<SSIZE, Error>::Err(Error::Process_WaitFailed);

#if defined(PLATFORM_SOLARIS)
	// Solaris uses waitid(P_PID, pid, &siginfo, WEXITED)
	UINT8 siginfo[256];
	Memory::Zero(siginfo, sizeof(siginfo));
	SSIZE result = System::Call(SYS_WAITID, (USIZE)P_PID, (USIZE)id, (USIZE)siginfo, (USIZE)WEXITED);
	if (result < 0)
	{
		return Result<SSIZE, Error>::Err(
			Error::Posix((UINT32)(-result)), Error::Process_WaitFailed);
	}
	// Extract si_status from siginfo_t
	// LP64: offset 24 (3 ints + pad + pid + pad + utime(8) = 24 for status)
	// ILP32: offset 20 (3 ints + pid + utime(4) = 20 for status)
#if defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_AARCH64)
	INT32 exitCode = *(INT32 *)(siginfo + 24);
#else
	INT32 exitCode = *(INT32 *)(siginfo + 20);
#endif
	id = INVALID_ID;
	return Result<SSIZE, Error>::Ok((SSIZE)exitCode);
#else
	// All other POSIX platforms use wait4
	INT32 status = 0;
	SSIZE result = System::Call(SYS_WAIT4, (USIZE)id, (USIZE)&status, 0, 0);
	if (result < 0)
	{
		return Result<SSIZE, Error>::Err(
			Error::Posix((UINT32)(-result)), Error::Process_WaitFailed);
	}
	id = INVALID_ID;
	// WEXITSTATUS: bits 15..8 of status
	SSIZE exitCode = (SSIZE)((status >> 8) & 0xFF);
	return Result<SSIZE, Error>::Ok(exitCode);
#endif
}

// ============================================================================
// Process::Terminate
// ============================================================================

Result<void, Error> Process::Terminate() noexcept
{
	if (!IsValid())
		return Result<void, Error>::Err(Error::Process_TerminateFailed);

	SSIZE result = System::Call(SYS_KILL, (USIZE)id, (USIZE)SIGKILL);
	if (result < 0)
	{
		return Result<void, Error>::Err(
			Error::Posix((UINT32)(-result)), Error::Process_TerminateFailed);
	}
	return Result<void, Error>::Ok();
}

// ============================================================================
// Process::IsRunning
// ============================================================================

BOOL Process::IsRunning() const noexcept
{
	if (!IsValid())
		return false;

	// kill(pid, 0) checks if process exists without sending a signal
	SSIZE result = System::Call(SYS_KILL, (USIZE)id, 0);
	return result >= 0;
}

// ============================================================================
// Process::Close
// ============================================================================

Result<void, Error> Process::Close() noexcept
{
	if (!IsValid())
		return Result<void, Error>::Ok();

	// Try to reap zombie (non-blocking) to avoid resource leak
#if defined(PLATFORM_SOLARIS)
	UINT8 siginfo[256];
	Memory::Zero(siginfo, sizeof(siginfo));
	(void)System::Call(SYS_WAITID, (USIZE)P_PID, (USIZE)id, (USIZE)siginfo, (USIZE)(WEXITED | WNOHANG));
#else
	INT32 status = 0;
	(void)System::Call(SYS_WAIT4, (USIZE)id, (USIZE)&status, (USIZE)WNOHANG, 0);
#endif

	id = INVALID_ID;
	return Result<void, Error>::Ok();
}
