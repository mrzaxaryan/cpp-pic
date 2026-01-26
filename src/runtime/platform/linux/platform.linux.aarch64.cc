/**
 * platform.linux.aarch64.cc - AArch64 Linux Syscall Implementation
 *
 * Implements the low-level __syscall() function for AArch64 (64-bit ARM).
 * Uses the `svc #0` (supervisor call) instruction to invoke Linux kernel
 * system calls.
 *
 * AArch64 Syscall Calling Convention:
 *   INPUT:
 *     X8 - System call number
 *     X0-X5 - Arguments 1-6
 *   OUTPUT:
 *     X0 - Return value (negative errno on error)
 *   PRESERVED:
 *     X1-X7, X9-X15 are preserved across the syscall
 *
 * svc #0 causes a synchronous exception from EL0 (user) to EL1 (kernel).
 */

#if defined(PLATFORM_LINUX_AARCH64)

#include "platform.h"

/**
 * __syscall - Invoke a Linux system call on AArch64
 *
 * @param nr - System call number (passed in X8)
 * @param a1-a6 - Syscall arguments (X0-X5)
 * @return Syscall return value in X0 (negative = -errno on error)
 *
 * X0 is both input (1st arg) and output (return value), using "+r" constraint.
 */
SSIZE __syscall(SSIZE nr,
                USIZE a1, USIZE a2, USIZE a3,
                USIZE a4, USIZE a5, USIZE a6)
{
    /* Bind arguments to specific registers */
    register SSIZE x8 __asm__("x8") = nr;
    register SSIZE x0 __asm__("x0") = (SSIZE)a1;
    register SSIZE x1 __asm__("x1") = (SSIZE)a2;
    register SSIZE x2 __asm__("x2") = (SSIZE)a3;
    register SSIZE x3 __asm__("x3") = (SSIZE)a4;
    register SSIZE x4 __asm__("x4") = (SSIZE)a5;
    register SSIZE x5 __asm__("x5") = (SSIZE)a6;

    __asm__ volatile(
        "svc #0"
        : "+r"(x0)
        : "r"(x8),
          "r"(x1),
          "r"(x2),
          "r"(x3),
          "r"(x4),
          "r"(x5)
        : "memory", "cc"
    );

    return x0;
}

#endif /* PLATFORM_LINUX_AARCH64 */
