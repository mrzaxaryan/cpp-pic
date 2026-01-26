/**
 * platform.linux.x86_64.cc - x86_64 Linux Syscall Implementation
 *
 * Implements the low-level __syscall() function for x86_64 (AMD64).
 * Uses the `syscall` instruction to invoke Linux kernel system calls.
 *
 * x86_64 Syscall Calling Convention:
 *   INPUT:
 *     RAX - System call number
 *     RDI, RSI, RDX, R10, R8, R9 - Arguments 1-6
 *   OUTPUT:
 *     RAX - Return value (negative errno on error)
 *   CLOBBERED:
 *     RCX - Holds return address after syscall
 *     R11 - Holds RFLAGS after syscall
 *
 * Note: R10 is used instead of RCX for 4th argument because syscall
 * uses RCX internally to save RIP.
 */

#if defined(PLATFORM_LINUX_X86_64)

#include "platform.h"

/**
 * __syscall - Invoke a Linux system call on x86_64
 *
 * @param nr - System call number
 * @param a1-a6 - Syscall arguments (RDI, RSI, RDX, R10, R8, R9)
 * @return Syscall return value in RAX (negative = -errno on error)
 *
 * GCC constraint letters: "a"=RAX, "D"=RDI, "S"=RSI, "d"=RDX
 * R10, R8, R9 use register variables with "r" constraint.
 */
SSIZE __syscall(SSIZE nr,
                USIZE a1, USIZE a2, USIZE a3,
                USIZE a4, USIZE a5, USIZE a6)
{
    SSIZE ret;

    /* Load arguments 4-6 into R10, R8, R9 using register variables */
    register SSIZE r10 __asm__("r10") = (SSIZE)a4;
    register SSIZE r8  __asm__("r8")  = (SSIZE)a5;
    register SSIZE r9  __asm__("r9")  = (SSIZE)a6;

    __asm__ volatile(
        "syscall"
        : "=a"(ret)
        : "a"(nr),
          "D"(a1),
          "S"(a2),
          "d"(a3),
          "r"(r10),
          "r"(r8),
          "r"(r9)
        : "rcx", "r11", "memory"
    );

    return ret;
}

#endif /* PLATFORM_LINUX_X86_64 */
