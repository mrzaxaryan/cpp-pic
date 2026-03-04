/**
 * process.cc - UEFI Process Execution Stub
 *
 * UEFI does not support process creation - all functions return failure.
 */

#include "platform/system/process.h"

// UEFI doesn't have fork
Result<SSIZE, Error> Process::Fork() noexcept
{
	return Result<SSIZE, Error>::Err(Error::Process_ForkFailed);
}

// UEFI doesn't have dup2
Result<SSIZE, Error> Process::Dup2(SSIZE oldfd, SSIZE newfd) noexcept
{
	(void)oldfd;
	(void)newfd;
	return Result<SSIZE, Error>::Err(Error::Process_Dup2Failed);
}

// UEFI doesn't have execve
Result<SSIZE, Error> Process::Execve(const CHAR* pathname, CHAR* const argv[], CHAR* const envp[]) noexcept
{
	(void)pathname;
	(void)argv;
	(void)envp;
	return Result<SSIZE, Error>::Err(Error::Process_ExecveFailed);
}

// UEFI doesn't have setsid
Result<SSIZE, Error> Process::Setsid() noexcept
{
	return Result<SSIZE, Error>::Err(Error::Process_SetsidFailed);
}

// UEFI doesn't support process creation
Result<SSIZE, Error> Process::BindSocketToShell(SSIZE socketFd, const CHAR* cmd) noexcept
{
	(void)socketFd;
	(void)cmd;
	return Result<SSIZE, Error>::Err(Error::Process_BindShellFailed);
}
