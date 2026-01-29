#pragma once

#include "primitives.h"

// Linux syscall numbers
namespace Syscall
{
    // x86_64 syscall wrappers
#if defined(ARCHITECTURE_X86_64)

    // Syscall with 0 arguments
    inline SSIZE syscall0(USIZE number)
    {
        SSIZE ret;
        __asm__ volatile(
            "mov %1, %%rax\n"
            "syscall\n"
            "mov %%rax, %0\n"
            : "=r"(ret)
            : "r"(number)
            : "rax", "rcx", "r11", "memory"
        );
        return ret;
    }

    // Syscall with 1 argument
    inline SSIZE syscall1(USIZE number, USIZE arg1)
    {
        SSIZE ret;
        __asm__ volatile(
            "mov %1, %%rdi\n"
            "mov %2, %%rax\n"
            "syscall\n"
            "mov %%rax, %0\n"
            : "=r"(ret)
            : "r"(arg1), "r"(number)
            : "rax", "rdi", "rcx", "r11", "memory"
        );
        return ret;
    }

    // Syscall with 2 arguments
    inline SSIZE syscall2(USIZE number, USIZE arg1, USIZE arg2)
    {
        SSIZE ret;
        __asm__ volatile(
            "mov %1, %%rdi\n"
            "mov %2, %%rsi\n"
            "mov %3, %%rax\n"
            "syscall\n"
            "mov %%rax, %0\n"
            : "=r"(ret)
            : "r"(arg1), "r"(arg2), "r"(number)
            : "rax", "rdi", "rsi", "rcx", "r11", "memory"
        );
        return ret;
    }

    // Syscall with 3 arguments
    inline SSIZE syscall3(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
    {
        SSIZE ret;
        __asm__ volatile(
            "mov %1, %%rdi\n"
            "mov %2, %%rsi\n"
            "mov %3, %%rdx\n"
            "mov %4, %%rax\n"
            "syscall\n"
            "mov %%rax, %0\n"
            : "=r"(ret)
            : "r"(arg1), "r"(arg2), "r"(arg3), "r"(number)
            : "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
        );
        return ret;
    }

    // Syscall with 4 arguments
    inline SSIZE syscall4(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
    {
        SSIZE ret;
        __asm__ volatile(
            "mov %1, %%rdi\n"
            "mov %2, %%rsi\n"
            "mov %3, %%rdx\n"
            "mov %4, %%r10\n"
            "mov %5, %%rax\n"
            "syscall\n"
            "mov %%rax, %0\n"
            : "=r"(ret)
            : "r"(arg1), "r"(arg2), "r"(arg3), "r"(arg4), "r"(number)
            : "rax", "rdi", "rsi", "rdx", "r10", "rcx", "r11", "memory"
        );
        return ret;
    }

    // Syscall with 5 arguments
    inline SSIZE syscall5(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
    {
        SSIZE ret;
        __asm__ volatile(
            "mov %1, %%rdi\n"
            "mov %2, %%rsi\n"
            "mov %3, %%rdx\n"
            "mov %4, %%r10\n"
            "mov %5, %%r8\n"
            "mov %6, %%rax\n"
            "syscall\n"
            "mov %%rax, %0\n"
            : "=r"(ret)
            : "r"(arg1), "r"(arg2), "r"(arg3), "r"(arg4), "r"(arg5), "r"(number)
            : "rax", "rdi", "rsi", "rdx", "r10", "r8", "rcx", "r11", "memory"
        );
        return ret;
    }

    // Syscall with 6 arguments
    inline SSIZE syscall6(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
    {
        SSIZE ret;
        __asm__ volatile(
            "mov %1, %%rdi\n"
            "mov %2, %%rsi\n"
            "mov %3, %%rdx\n"
            "mov %4, %%r10\n"
            "mov %5, %%r8\n"
            "mov %6, %%r9\n"
            "mov %7, %%rax\n"
            "syscall\n"
            "mov %%rax, %0\n"
            : "=r"(ret)
            : "r"(arg1), "r"(arg2), "r"(arg3), "r"(arg4), "r"(arg5), "r"(arg6), "r"(number)
            : "rax", "rdi", "rsi", "rdx", "r10", "r8", "r9", "rcx", "r11", "memory"
        );
        return ret;
    }

    // i386 syscall wrappers
#elif defined(ARCHITECTURE_I386)

    // Syscall with 0 arguments
    inline SSIZE syscall0(USIZE number)
    {
        SSIZE ret;
        __asm__ volatile(
            "mov %1, %%eax\n"
            "int $0x80\n"
            "mov %%eax, %0\n"
            : "=r"(ret)
            : "r"(number)
            : "eax", "memory"
        );
        return ret;
    }

    // Syscall with 1 argument
    inline SSIZE syscall1(USIZE number, USIZE arg1)
    {
        SSIZE ret;
        __asm__ volatile(
            "xchg %%ebx, %1\n"  // Save ebx (PIC register), load arg1
            "int $0x80\n"
            "xchg %%ebx, %1\n"  // Restore ebx
            : "=a"(ret)
            : "r"(arg1), "a"(number)
            : "memory"
        );
        return ret;
    }

    // Syscall with 2 arguments
    inline SSIZE syscall2(USIZE number, USIZE arg1, USIZE arg2)
    {
        SSIZE ret;
        __asm__ volatile(
            "xchg %%ebx, %1\n"  // Save ebx, load arg1
            "int $0x80\n"
            "xchg %%ebx, %1\n"  // Restore ebx
            : "=a"(ret)
            : "r"(arg1), "c"(arg2), "a"(number)
            : "memory"
        );
        return ret;
    }

    // Syscall with 3 arguments
    inline SSIZE syscall3(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
    {
        SSIZE ret;
        __asm__ volatile(
            "xchg %%ebx, %1\n"  // Save ebx, load arg1
            "int $0x80\n"
            "xchg %%ebx, %1\n"  // Restore ebx
            : "=a"(ret)
            : "r"(arg1), "c"(arg2), "d"(arg3), "a"(number)
            : "memory"
        );
        return ret;
    }

    // Syscall with 4 arguments
    inline SSIZE syscall4(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
    {
        SSIZE ret;
        __asm__ volatile(
            "xchg %%ebx, %1\n"  // Save ebx, load arg1
            "int $0x80\n"
            "xchg %%ebx, %1\n"  // Restore ebx
            : "=a"(ret)
            : "r"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "a"(number)
            : "memory"
        );
        return ret;
    }

    // Syscall with 5 arguments
    inline SSIZE syscall5(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
    {
        SSIZE ret;
        __asm__ volatile(
            "push %%ebx\n"      // Save ebx (PIC register)
            "mov %1, %%ebx\n"   // Load arg1
            "int $0x80\n"
            "pop %%ebx\n"       // Restore ebx
            : "=a"(ret)
            : "r"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5), "a"(number)
            : "memory"
        );
        return ret;
    }

    // Syscall with 6 arguments
    inline SSIZE syscall6(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
    {
        SSIZE ret;
        __asm__ volatile(
            "push %%ebp\n"      // Save ebp
            "push %%ebx\n"      // Save ebx (PIC register)
            "mov %1, %%ebx\n"   // Load arg1
            "mov %6, %%ebp\n"   // Load arg6
            "int $0x80\n"
            "pop %%ebx\n"       // Restore ebx
            "pop %%ebp\n"       // Restore ebp
            : "=a"(ret)
            : "r"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5), "r"(arg6), "a"(number)
            : "memory"
        );
        return ret;
    }

    // AArch64 syscall wrappers
#elif defined(ARCHITECTURE_AARCH64)

    // Syscall with 0 arguments
    inline SSIZE syscall0(USIZE number)
    {
        register USIZE x0 __asm__("x0");
        register USIZE x8 __asm__("x8") = number;
        __asm__ volatile(
            "svc #0\n"
            : "=r"(x0)
            : "r"(x8)
            : "memory", "cc"
        );
        return (SSIZE)x0;
    }

    // Syscall with 1 argument
    inline SSIZE syscall1(USIZE number, USIZE arg1)
    {
        register USIZE x0 __asm__("x0") = arg1;
        register USIZE x8 __asm__("x8") = number;
        __asm__ volatile(
            "svc #0\n"
            : "+r"(x0)
            : "r"(x8)
            : "memory", "cc"
        );
        return (SSIZE)x0;
    }

    // Syscall with 2 arguments
    inline SSIZE syscall2(USIZE number, USIZE arg1, USIZE arg2)
    {
        register USIZE x0 __asm__("x0") = arg1;
        register USIZE x1 __asm__("x1") = arg2;
        register USIZE x8 __asm__("x8") = number;
        __asm__ volatile(
            "svc #0\n"
            : "+r"(x0)
            : "r"(x1), "r"(x8)
            : "memory", "cc"
        );
        return (SSIZE)x0;
    }

    // Syscall with 3 arguments
    inline SSIZE syscall3(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
    {
        register USIZE x0 __asm__("x0") = arg1;
        register USIZE x1 __asm__("x1") = arg2;
        register USIZE x2 __asm__("x2") = arg3;
        register USIZE x8 __asm__("x8") = number;
        __asm__ volatile(
            "svc #0\n"
            : "+r"(x0)
            : "r"(x1), "r"(x2), "r"(x8)
            : "memory", "cc"
        );
        return (SSIZE)x0;
    }

    // Syscall with 4 arguments
    inline SSIZE syscall4(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
    {
        register USIZE x0 __asm__("x0") = arg1;
        register USIZE x1 __asm__("x1") = arg2;
        register USIZE x2 __asm__("x2") = arg3;
        register USIZE x3 __asm__("x3") = arg4;
        register USIZE x8 __asm__("x8") = number;
        __asm__ volatile(
            "svc #0\n"
            : "+r"(x0)
            : "r"(x1), "r"(x2), "r"(x3), "r"(x8)
            : "memory", "cc"
        );
        return (SSIZE)x0;
    }

    // Syscall with 5 arguments
    inline SSIZE syscall5(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
    {
        register USIZE x0 __asm__("x0") = arg1;
        register USIZE x1 __asm__("x1") = arg2;
        register USIZE x2 __asm__("x2") = arg3;
        register USIZE x3 __asm__("x3") = arg4;
        register USIZE x4 __asm__("x4") = arg5;
        register USIZE x8 __asm__("x8") = number;
        __asm__ volatile(
            "svc #0\n"
            : "+r"(x0)
            : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x8)
            : "memory", "cc"
        );
        return (SSIZE)x0;
    }

    // Syscall with 6 arguments
    inline SSIZE syscall6(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
    {
        register USIZE x0 __asm__("x0") = arg1;
        register USIZE x1 __asm__("x1") = arg2;
        register USIZE x2 __asm__("x2") = arg3;
        register USIZE x3 __asm__("x3") = arg4;
        register USIZE x4 __asm__("x4") = arg5;
        register USIZE x5 __asm__("x5") = arg6;
        register USIZE x8 __asm__("x8") = number;
        __asm__ volatile(
            "svc #0\n"
            : "+r"(x0)
            : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8)
            : "memory", "cc"
        );
        return (SSIZE)x0;
    }

    // ARMv7 syscall wrappers
#elif defined(ARCHITECTURE_ARMV7A)

    // Syscall with 0 arguments
    inline SSIZE syscall0(USIZE number)
    {
        register USIZE r0 __asm__("r0");
        register USIZE r7 __asm__("r7") = number;
        __asm__ volatile(
            "svc #0\n"
            : "=r"(r0)
            : "r"(r7)
            : "memory", "cc"
        );
        return (SSIZE)r0;
    }

    // Syscall with 1 argument
    inline SSIZE syscall1(USIZE number, USIZE arg1)
    {
        register USIZE r0 __asm__("r0") = arg1;
        register USIZE r7 __asm__("r7") = number;
        __asm__ volatile(
            "svc #0\n"
            : "+r"(r0)
            : "r"(r7)
            : "memory", "cc"
        );
        return (SSIZE)r0;
    }

    // Syscall with 2 arguments
    inline SSIZE syscall2(USIZE number, USIZE arg1, USIZE arg2)
    {
        register USIZE r0 __asm__("r0") = arg1;
        register USIZE r1 __asm__("r1") = arg2;
        register USIZE r7 __asm__("r7") = number;
        __asm__ volatile(
            "svc #0\n"
            : "+r"(r0)
            : "r"(r1), "r"(r7)
            : "memory", "cc"
        );
        return (SSIZE)r0;
    }

    // Syscall with 3 arguments
    inline SSIZE syscall3(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
    {
        register USIZE r0 __asm__("r0") = arg1;
        register USIZE r1 __asm__("r1") = arg2;
        register USIZE r2 __asm__("r2") = arg3;
        register USIZE r7 __asm__("r7") = number;
        __asm__ volatile(
            "svc #0\n"
            : "+r"(r0)
            : "r"(r1), "r"(r2), "r"(r7)
            : "memory", "cc"
        );
        return (SSIZE)r0;
    }

    // Syscall with 4 arguments
    inline SSIZE syscall4(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
    {
        register USIZE r0 __asm__("r0") = arg1;
        register USIZE r1 __asm__("r1") = arg2;
        register USIZE r2 __asm__("r2") = arg3;
        register USIZE r3 __asm__("r3") = arg4;
        register USIZE r7 __asm__("r7") = number;
        __asm__ volatile(
            "svc #0\n"
            : "+r"(r0)
            : "r"(r1), "r"(r2), "r"(r3), "r"(r7)
            : "memory", "cc"
        );
        return (SSIZE)r0;
    }

    // Syscall with 5 arguments
    inline SSIZE syscall5(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
    {
        register USIZE r0 __asm__("r0") = arg1;
        register USIZE r1 __asm__("r1") = arg2;
        register USIZE r2 __asm__("r2") = arg3;
        register USIZE r3 __asm__("r3") = arg4;
        register USIZE r4 __asm__("r4") = arg5;
        register USIZE r7 __asm__("r7") = number;
        __asm__ volatile(
            "svc #0\n"
            : "+r"(r0)
            : "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r7)
            : "memory", "cc"
        );
        return (SSIZE)r0;
    }

    // Syscall with 6 arguments
    inline SSIZE syscall6(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
    {
        register USIZE r0 __asm__("r0") = arg1;
        register USIZE r1 __asm__("r1") = arg2;
        register USIZE r2 __asm__("r2") = arg3;
        register USIZE r3 __asm__("r3") = arg4;
        register USIZE r4 __asm__("r4") = arg5;
        register USIZE r5 __asm__("r5") = arg6;
        register USIZE r7 __asm__("r7") = number;
        __asm__ volatile(
            "svc #0\n"
            : "+r"(r0)
            : "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5), "r"(r7)
            : "memory", "cc"
        );
        return (SSIZE)r0;
    }

#endif

}
