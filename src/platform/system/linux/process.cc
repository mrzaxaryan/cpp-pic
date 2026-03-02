/**
 * process.cc - Linux Process Execution Implementation
 *
 * Provides fork/exec functionality via direct syscalls.
 */

#include "platform/system/process.h"
#include "platform/common/linux/syscall.h"
#include "platform/common/linux/system.h"

// Fork syscall wrapper
Result<SSIZE, Error> Process::Fork() noexcept
{
#if defined(ARCHITECTURE_AARCH64)
	// aarch64 uses clone with SIGCHLD flag for fork-like behavior
	constexpr USIZE SIGCHLD = 17;
	SSIZE result = System::Call(SYS_CLONE, SIGCHLD, 0, 0, 0, 0);
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
#if defined(ARCHITECTURE_AARCH64)
	// aarch64 uses dup3 with flags=0 instead of dup2
	SSIZE result = System::Call(SYS_DUP3, (USIZE)oldfd, (USIZE)newfd, 0);
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
	SSIZE result = System::Call(SYS_SETSID);
	if (result < 0)
	{
		return Result<SSIZE, Error>::Err(Error::Posix((UINT32)(-result)), Error::Process_SetsidFailed);
	}
	return Result<SSIZE, Error>::Ok(result);
}

