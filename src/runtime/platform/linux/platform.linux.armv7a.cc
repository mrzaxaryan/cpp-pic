/**
 * platform.linux.armv7a.cc - ARMv7-A Linux Syscall Implementation
 *
 * Implements the low-level __syscall() function for ARMv7-A (32-bit ARM).
 * Uses the `svc 0` instruction with EABI (Embedded ABI) convention.
 *
 * ARMv7-A EABI Syscall Calling Convention:
 *   INPUT:
 *     R7 - System call number
 *     R0-R5 - Arguments 1-6
 *   OUTPUT:
 *     R0 - Return value (negative errno on error)
 *   PRESERVED:
 *     R4-R11 are callee-saved in AAPCS
 *
 * EABI places syscall number in R7 (unlike OABI which encoded it in svc).
 */

#if defined(PLATFORM_LINUX_ARMV7A)

#include "platform.h"

/**
 * __syscall - Invoke a Linux system call on ARMv7-A (EABI)
 *
 * @param nr - System call number (passed in R7)
 * @param a1-a6 - Syscall arguments (R0-R5)
 * @return Syscall return value in R0 (negative = -errno on error)
 *
 * R0 is both input (1st arg) and output (return value), using "+r" constraint.
 * Uses 'long' type for register variables to match 32-bit register size.
 */
SSIZE __syscall(SSIZE nr,
                USIZE a1, USIZE a2, USIZE a3,
                USIZE a4, USIZE a5, USIZE a6)
{
    /* Bind arguments to specific ARM registers */
    register long r0 __asm__("r0") = (long)a1;
    register long r1 __asm__("r1") = (long)a2;
    register long r2 __asm__("r2") = (long)a3;
    register long r3 __asm__("r3") = (long)a4;
    register long r4 __asm__("r4") = (long)a5;
    register long r5 __asm__("r5") = (long)a6;
    register long r7 __asm__("r7") = (long)nr;

    __asm__ volatile(
        "svc 0"
        : "+r"(r0)
        : "r"(r1),
          "r"(r2),
          "r"(r3),
          "r"(r4),
          "r"(r5),
          "r"(r7)
        : "memory", "cc"
    );

    return (SSIZE)r0;
}

#endif /* PLATFORM_LINUX_ARMV7A */
