#pragma once

#include "primitives.h"

// macOS syscall numbers
// On macOS, BSD syscalls use the syscall instruction on x86_64 and svc on ARM64
// Syscall numbers must be ORed with class constants:
//   0x2000000 - BSD syscalls (SYSCALL_CLASS_UNIX)
//   0x1000000 - Mach syscalls (SYSCALL_CLASS_MACH)
namespace Syscall
{
    // x86_64 syscall wrappers
#if defined(ARCHITECTURE_X86_64)

    // macOS BSD syscall class (must be ORed with syscall number)
    constexpr USIZE SYSCALL_CLASS_UNIX = 0x2000000;

    // Syscall with 0 arguments
    inline SSIZE syscall0(USIZE number)
    {
        register USIZE r_rax __asm__("rax") = number | SYSCALL_CLASS_UNIX;
        __asm__ volatile(
            "syscall\n"
            : "+r"(r_rax)
            :
            : "rcx", "r11", "memory"
        );
        return (SSIZE)r_rax;
    }

    // Syscall with 1 argument
    inline SSIZE syscall1(USIZE number, USIZE arg1)
    {
        register USIZE r_rdi __asm__("rdi") = arg1;
        register USIZE r_rax __asm__("rax") = number | SYSCALL_CLASS_UNIX;
        __asm__ volatile(
            "syscall\n"
            : "+r"(r_rax)
            : "r"(r_rdi)
            : "rcx", "r11", "memory"
        );
        return (SSIZE)r_rax;
    }

    // Syscall with 2 arguments
    inline SSIZE syscall2(USIZE number, USIZE arg1, USIZE arg2)
    {
        register USIZE r_rdi __asm__("rdi") = arg1;
        register USIZE r_rsi __asm__("rsi") = arg2;
        register USIZE r_rax __asm__("rax") = number | SYSCALL_CLASS_UNIX;
        __asm__ volatile(
            "syscall\n"
            : "+r"(r_rax)
            : "r"(r_rdi), "r"(r_rsi)
            : "rcx", "r11", "memory"
        );
        return (SSIZE)r_rax;
    }

    // Syscall with 3 arguments
    inline SSIZE syscall3(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
    {
        register USIZE r_rdi __asm__("rdi") = arg1;
        register USIZE r_rsi __asm__("rsi") = arg2;
        register USIZE r_rdx __asm__("rdx") = arg3;
        register USIZE r_rax __asm__("rax") = number | SYSCALL_CLASS_UNIX;
        __asm__ volatile(
            "syscall\n"
            : "+r"(r_rax)
            : "r"(r_rdi), "r"(r_rsi), "r"(r_rdx)
            : "rcx", "r11", "memory"
        );
        return (SSIZE)r_rax;
    }

    // Syscall with 4 arguments
    inline SSIZE syscall4(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
    {
        register USIZE r_rdi __asm__("rdi") = arg1;
        register USIZE r_rsi __asm__("rsi") = arg2;
        register USIZE r_rdx __asm__("rdx") = arg3;
        register USIZE r_r10 __asm__("r10") = arg4;
        register USIZE r_rax __asm__("rax") = number | SYSCALL_CLASS_UNIX;
        __asm__ volatile(
            "syscall\n"
            : "+r"(r_rax)
            : "r"(r_rdi), "r"(r_rsi), "r"(r_rdx), "r"(r_r10)
            : "rcx", "r11", "memory"
        );
        return (SSIZE)r_rax;
    }

    // Syscall with 5 arguments
    inline SSIZE syscall5(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
    {
        register USIZE r_rdi __asm__("rdi") = arg1;
        register USIZE r_rsi __asm__("rsi") = arg2;
        register USIZE r_rdx __asm__("rdx") = arg3;
        register USIZE r_r10 __asm__("r10") = arg4;
        register USIZE r_r8 __asm__("r8") = arg5;
        register USIZE r_rax __asm__("rax") = number | SYSCALL_CLASS_UNIX;
        __asm__ volatile(
            "syscall\n"
            : "+r"(r_rax)
            : "r"(r_rdi), "r"(r_rsi), "r"(r_rdx), "r"(r_r10), "r"(r_r8)
            : "rcx", "r11", "memory"
        );
        return (SSIZE)r_rax;
    }

    // Syscall with 6 arguments
    inline SSIZE syscall6(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
    {
        register USIZE r_rdi __asm__("rdi") = arg1;
        register USIZE r_rsi __asm__("rsi") = arg2;
        register USIZE r_rdx __asm__("rdx") = arg3;
        register USIZE r_r10 __asm__("r10") = arg4;
        register USIZE r_r8 __asm__("r8") = arg5;
        register USIZE r_r9 __asm__("r9") = arg6;
        register USIZE r_rax __asm__("rax") = number | SYSCALL_CLASS_UNIX;
        __asm__ volatile(
            "syscall\n"
            : "+r"(r_rax)
            : "r"(r_rdi), "r"(r_rsi), "r"(r_rdx), "r"(r_r10), "r"(r_r8), "r"(r_r9)
            : "rcx", "r11", "memory"
        );
        return (SSIZE)r_rax;
    }

    // AArch64 (Apple Silicon) syscall wrappers
#elif defined(ARCHITECTURE_AARCH64)

    // macOS BSD syscall class (ARM64 uses x16 for syscall number)
    constexpr USIZE SYSCALL_CLASS_UNIX = 0;

    // Syscall with 0 arguments
    inline SSIZE syscall0(USIZE number)
    {
        register USIZE x0 __asm__("x0");
        register USIZE x16 __asm__("x16") = number;
        __asm__ volatile(
            "svc #0x80\n"
            : "=r"(x0)
            : "r"(x16)
            : "memory", "cc"
        );
        return (SSIZE)x0;
    }

    // Syscall with 1 argument
    inline SSIZE syscall1(USIZE number, USIZE arg1)
    {
        register USIZE x0 __asm__("x0") = arg1;
        register USIZE x16 __asm__("x16") = number;
        __asm__ volatile(
            "svc #0x80\n"
            : "+r"(x0)
            : "r"(x16)
            : "memory", "cc"
        );
        return (SSIZE)x0;
    }

    // Syscall with 2 arguments
    inline SSIZE syscall2(USIZE number, USIZE arg1, USIZE arg2)
    {
        register USIZE x0 __asm__("x0") = arg1;
        register USIZE x1 __asm__("x1") = arg2;
        register USIZE x16 __asm__("x16") = number;
        __asm__ volatile(
            "svc #0x80\n"
            : "+r"(x0)
            : "r"(x1), "r"(x16)
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
        register USIZE x16 __asm__("x16") = number;
        __asm__ volatile(
            "svc #0x80\n"
            : "+r"(x0)
            : "r"(x1), "r"(x2), "r"(x16)
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
        register USIZE x16 __asm__("x16") = number;
        __asm__ volatile(
            "svc #0x80\n"
            : "+r"(x0)
            : "r"(x1), "r"(x2), "r"(x3), "r"(x16)
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
        register USIZE x16 __asm__("x16") = number;
        __asm__ volatile(
            "svc #0x80\n"
            : "+r"(x0)
            : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x16)
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
        register USIZE x16 __asm__("x16") = number;
        __asm__ volatile(
            "svc #0x80\n"
            : "+r"(x0)
            : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x16)
            : "memory", "cc"
        );
        return (SSIZE)x0;
    }

#endif

}
