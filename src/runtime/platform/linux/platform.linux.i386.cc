/**
 * platform.linux.i386.cc - i386 Linux Syscall Implementation
 *
 * Implements the low-level __syscall() function for i386 (32-bit x86).
 * Uses the `int 0x80` software interrupt to invoke Linux kernel system calls.
 *
 * i386 Syscall Calling Convention:
 *   INPUT:
 *     EAX - System call number
 *     EBX, ECX, EDX, ESI, EDI, EBP - Arguments 1-6
 *   OUTPUT:
 *     EAX - Return value (negative errno on error)
 *
 * EBX, ESI, EDI, EBP are callee-saved in cdecl ABI, so we must
 * save/restore them around the syscall.
 */

#if defined(PLATFORM_LINUX_I386)

#include "platform.h"

/**
 * __syscall - Invoke a Linux system call on i386
 *
 * @param nr - System call number
 * @param a1-a6 - Syscall arguments (EBX, ECX, EDX, ESI, EDI, EBP)
 * @return Syscall return value in EAX (negative = -errno on error)
 *
 * Uses push/pop to save callee-saved registers before loading syscall args.
 */
SSIZE __syscall(SSIZE nr,
                USIZE a1, USIZE a2, USIZE a3,
                USIZE a4, USIZE a5, USIZE a6)
{
    SSIZE ret;

    __asm__ volatile(
        /* Save callee-saved registers */
        "pushl %%ebx\n\t"
        "pushl %%esi\n\t"
        "pushl %%edi\n\t"
        "pushl %%ebp\n\t"

        /* Load syscall arguments */
        "movl %1, %%eax\n\t"
        "movl %2, %%ebx\n\t"
        "movl %3, %%ecx\n\t"
        "movl %4, %%edx\n\t"
        "movl %5, %%esi\n\t"
        "movl %6, %%edi\n\t"
        "movl %7, %%ebp\n\t"

        /* Execute syscall */
        "int $0x80\n\t"

        /* Restore callee-saved registers */
        "popl %%ebp\n\t"
        "popl %%edi\n\t"
        "popl %%esi\n\t"
        "popl %%ebx\n\t"

        : "=a"(ret)
        : "g"(nr),
          "g"(a1),
          "g"(a2),
          "g"(a3),
          "g"(a4),
          "g"(a5),
          "g"(a6)
        : "memory", "cc", "ecx", "edx"
    );

    return ret;
}

#endif /* PLATFORM_LINUX_I386 */
