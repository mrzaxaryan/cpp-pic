/**
 * syscall.h - Linux System Call Interface for Position-Independent Code
 *
 * This header provides a minimal, position-independent interface to Linux
 * kernel system calls. Instead of using libc wrappers (which would introduce
 * dependencies on .got/.plt sections), we invoke syscalls directly via
 * inline assembly.
 */

#if defined(PLATFORM_LINUX)
#pragma once

#include "platform.h"

/*
 * These constants match the Linux kernel ABI and are architecture-independent.
 * They are defined here to avoid any libc header dependencies.
 */
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/*
 * These flags control the access permissions of mapped memory pages.
 * They can be OR'd together (e.g., PROT_READ | PROT_WRITE).
 */
#define PROT_READ  0x01
#define PROT_WRITE 0x02

/*
 * These flags control how memory is mapped and shared.
 * For heap allocation, we use MAP_PRIVATE | MAP_ANONYMOUS to get
 * private, zero-initialized memory pages from the kernel.
 */
#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20

/*
 * On failure, mmap returns MAP_FAILED (which is (void*)-1, not NULL).
 * This is a POSIX requirement that differs from typical error handling.
 */
#define MAP_FAILED ((PVOID)(-1))

/* 
 * All methods are static - no instance needed. This provides:
 *   1. No global state to initialize
 *   2. No vtable in .rdata (important for PIC)
 *   3. Direct function calls (no virtual dispatch overhead)
 *
 * Each method is a thin wrapper around the corresponding syscall,
 * with the architecture-specific implementation in platform.linux.*.cc
 */
class Syscall
{
public:
    /* ------------------------------------------------------------------------
     * Write - Write data to a file descriptor
     * ------------------------------------------------------------------------
     * Writes up to 'count' bytes from 'buf' to file descriptor 'fd'.
     */
    static SSIZE Write(INT32 fd, PCVOID buf, USIZE count);

    /* ------------------------------------------------------------------------
     * Exit - Terminate the calling process
     * ------------------------------------------------------------------------
     * Terminates the process immediately with the given exit status.
     * This function never returns.
     */
    static SSIZE Exit(INT32 status);

    /* ------------------------------------------------------------------------
     * Mmap - Map memory pages
     * ------------------------------------------------------------------------
     * Creates a new mapping in the virtual address space of the calling process.
     * Used for memory allocation in our runtime.
     */
    static PVOID Mmap(PVOID addr, USIZE length, INT32 prot, INT32 flags, INT32 fd, SSIZE offset);

    /* ------------------------------------------------------------------------
     * Munmap - Unmap memory pages
     * ------------------------------------------------------------------------
     * Deletes the mapping for the specified address range.
     * Used for memory deallocation in our runtime.
     */
    static SSIZE Munmap(PVOID addr, USIZE length);
};

#endif // PLATFORM_LINUX
