/**
 * process.cc - UEFI Process Execution Stub
 *
 * UEFI does not support process creation - all functions return failure.
 */

#include "process.h"

// UEFI doesn't have fork
SSIZE Process::Fork() noexcept
{
    return PROCESS_INVALID_PID;
}

// UEFI doesn't have dup2
SSIZE Process::Dup2(SSIZE oldfd, SSIZE newfd) noexcept
{
    (void)oldfd;
    (void)newfd;
    return PROCESS_INVALID_PID;
}

// UEFI doesn't have execve
SSIZE Process::Execve(const CHAR* pathname, CHAR* const argv[], CHAR* const envp[]) noexcept
{
    (void)pathname;
    (void)argv;
    (void)envp;
    return PROCESS_INVALID_PID;
}

// UEFI doesn't have setsid
SSIZE Process::Setsid() noexcept
{
    return PROCESS_INVALID_PID;
}

// UEFI doesn't support process creation
SSIZE Process::BindSocketToShell(SSIZE socketFd, const CHAR* cmd) noexcept
{
    (void)socketFd;
    (void)cmd;
    return PROCESS_INVALID_PID;
}
