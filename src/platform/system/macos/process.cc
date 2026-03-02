#include "platform/system/process.h"
#include "platform/common/macos/syscall.h"
#include "platform/common/macos/system.h"

// Fork syscall wrapper
Result<SSIZE, Error> Process::Fork() noexcept
{
	SSIZE result = System::Call(SYS_FORK);
	if (result < 0)
	{
		return Result<SSIZE, Error>::Err(Error::Posix((UINT32)(-result)), Error::Process_ForkFailed);
	}
	return Result<SSIZE, Error>::Ok(result);
}

// Dup2 syscall wrapper
Result<SSIZE, Error> Process::Dup2(SSIZE oldfd, SSIZE newfd) noexcept
{
	SSIZE result = System::Call(SYS_DUP2, (USIZE)oldfd, (USIZE)newfd);
	if (result < 0)
	{
		return Result<SSIZE, Error>::Err(Error::Posix((UINT32)(-result)), Error::Process_Dup2Failed);
	}
	return Result<SSIZE, Error>::Ok(result);
}

// Execve syscall wrapper
Result<SSIZE, Error> Process::Execve(const CHAR* pathname, CHAR* const argv[], CHAR* const envp[]) noexcept
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

