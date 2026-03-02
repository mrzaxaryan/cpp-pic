/**
 * process.cc - Solaris/illumos Process Execution Implementation
 *
 * Provides fork/exec functionality via direct syscalls.
 * Solaris uses multiplexed syscalls: SYS_forksys for fork,
 * SYS_pgrpsys for setsid, and SYS_fcntl with F_DUP2FD for dup2.
 */

#include "platform/system/process.h"
#include "platform/common/solaris/syscall.h"
#include "platform/common/solaris/system.h"

// Fork syscall wrapper
// Solaris uses SYS_forksys (142) with subcode 0 for fork
Result<SSIZE, Error> Process::Fork() noexcept
{
	SSIZE result = System::Call(SYS_FORKSYS, FORKSYS_FORK, 0);
	if (result < 0)
	{
		return Result<SSIZE, Error>::Err(Error::Posix((UINT32)(-result)), Error::Process_ForkFailed);
	}
	return Result<SSIZE, Error>::Ok(result);
}

// Dup2 syscall wrapper
// Solaris has no SYS_dup2 — use fcntl(oldfd, F_DUP2FD, newfd) instead
Result<SSIZE, Error> Process::Dup2(SSIZE oldfd, SSIZE newfd) noexcept
{
	SSIZE result = System::Call(SYS_FCNTL, (USIZE)oldfd, (USIZE)F_DUP2FD, (USIZE)newfd);
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
// Solaris uses SYS_pgrpsys (39) with subcode 3 for setsid
Result<SSIZE, Error> Process::Setsid() noexcept
{
	SSIZE result = System::Call(SYS_PGRPSYS, PGRPSYS_SETSID);
	if (result < 0)
	{
		return Result<SSIZE, Error>::Err(Error::Posix((UINT32)(-result)), Error::Process_SetsidFailed);
	}
	return Result<SSIZE, Error>::Ok(result);
}

