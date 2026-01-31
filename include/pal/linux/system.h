#pragma once

#include "primitives.h"

// Linux syscall wrappers
class System
{
public:
    // x86_64 syscall wrappers
#if defined(ARCHITECTURE_X86_64)

    // Syscall with 0 arguments
    static inline SSIZE Call(USIZE number)
    {
        register USIZE r_rax __asm__("rax") = number;
        __asm__ volatile(
            "syscall\n"
            : "+r"(r_rax)
            :
            : "rcx", "r11", "memory"
        );
        return (SSIZE)r_rax;
    }

    // Syscall with 1 argument
    static inline SSIZE Call(USIZE number, USIZE arg1)
    {
        register USIZE r_rdi __asm__("rdi") = arg1;
        register USIZE r_rax __asm__("rax") = number;
        __asm__ volatile(
            "syscall\n"
            : "+r"(r_rax)
            : "r"(r_rdi)
            : "rcx", "r11", "memory"
        );
        return (SSIZE)r_rax;
    }

    // Syscall with 2 arguments
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
    {
        register USIZE r_rdi __asm__("rdi") = arg1;
        register USIZE r_rsi __asm__("rsi") = arg2;
        register USIZE r_rax __asm__("rax") = number;
        __asm__ volatile(
            "syscall\n"
            : "+r"(r_rax)
            : "r"(r_rdi), "r"(r_rsi)
            : "rcx", "r11", "memory"
        );
        return (SSIZE)r_rax;
    }

    // Syscall with 3 arguments
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
    {
        register USIZE r_rdi __asm__("rdi") = arg1;
        register USIZE r_rsi __asm__("rsi") = arg2;
        register USIZE r_rdx __asm__("rdx") = arg3;
        register USIZE r_rax __asm__("rax") = number;
        __asm__ volatile(
            "syscall\n"
            : "+r"(r_rax)
            : "r"(r_rdi), "r"(r_rsi), "r"(r_rdx)
            : "rcx", "r11", "memory"
        );
        return (SSIZE)r_rax;
    }

    // Syscall with 4 arguments
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
    {
        register USIZE r_rdi __asm__("rdi") = arg1;
        register USIZE r_rsi __asm__("rsi") = arg2;
        register USIZE r_rdx __asm__("rdx") = arg3;
        register USIZE r_r10 __asm__("r10") = arg4;
        register USIZE r_rax __asm__("rax") = number;
        __asm__ volatile(
            "syscall\n"
            : "+r"(r_rax)
            : "r"(r_rdi), "r"(r_rsi), "r"(r_rdx), "r"(r_r10)
            : "rcx", "r11", "memory"
        );
        return (SSIZE)r_rax;
    }

    // Syscall with 5 arguments
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
    {
        register USIZE r_rdi __asm__("rdi") = arg1;
        register USIZE r_rsi __asm__("rsi") = arg2;
        register USIZE r_rdx __asm__("rdx") = arg3;
        register USIZE r_r10 __asm__("r10") = arg4;
        register USIZE r_r8 __asm__("r8") = arg5;
        register USIZE r_rax __asm__("rax") = number;
        __asm__ volatile(
            "syscall\n"
            : "+r"(r_rax)
            : "r"(r_rdi), "r"(r_rsi), "r"(r_rdx), "r"(r_r10), "r"(r_r8)
            : "rcx", "r11", "memory"
        );
        return (SSIZE)r_rax;
    }

    // Syscall with 6 arguments
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
    {
        register USIZE r_rdi __asm__("rdi") = arg1;
        register USIZE r_rsi __asm__("rsi") = arg2;
        register USIZE r_rdx __asm__("rdx") = arg3;
        register USIZE r_r10 __asm__("r10") = arg4;
        register USIZE r_r8 __asm__("r8") = arg5;
        register USIZE r_r9 __asm__("r9") = arg6;
        register USIZE r_rax __asm__("rax") = number;
        __asm__ volatile(
            "syscall\n"
            : "+r"(r_rax)
            : "r"(r_rdi), "r"(r_rsi), "r"(r_rdx), "r"(r_r10), "r"(r_r8), "r"(r_r9)
            : "rcx", "r11", "memory"
        );
        return (SSIZE)r_rax;
    }

    // i386 syscall wrappers
#elif defined(ARCHITECTURE_I386)

    // Syscall with 0 arguments
    static inline SSIZE Call(USIZE number)
    {
        register USIZE r_eax __asm__("eax") = number;
        __asm__ volatile(
            "int $0x80\n"
            : "+r"(r_eax)
            :
            : "memory"
        );
        return (SSIZE)r_eax;
    }

    // Syscall with 1 argument
    static inline SSIZE Call(USIZE number, USIZE arg1)
    {
        register USIZE r_ebx __asm__("ebx") = arg1;
        register USIZE r_eax __asm__("eax") = number;
        __asm__ volatile(
            "int $0x80\n"
            : "+r"(r_eax)
            : "r"(r_ebx)
            : "memory"
        );
        return (SSIZE)r_eax;
    }

    // Syscall with 2 arguments
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
    {
        register USIZE r_ebx __asm__("ebx") = arg1;
        register USIZE r_ecx __asm__("ecx") = arg2;
        register USIZE r_eax __asm__("eax") = number;
        __asm__ volatile(
            "int $0x80\n"
            : "+r"(r_eax)
            : "r"(r_ebx), "r"(r_ecx)
            : "memory"
        );
        return (SSIZE)r_eax;
    }

    // Syscall with 3 arguments
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
    {
        register USIZE r_ebx __asm__("ebx") = arg1;
        register USIZE r_ecx __asm__("ecx") = arg2;
        register USIZE r_edx __asm__("edx") = arg3;
        register USIZE r_eax __asm__("eax") = number;
        __asm__ volatile(
            "int $0x80\n"
            : "+r"(r_eax)
            : "r"(r_ebx), "r"(r_ecx), "r"(r_edx)
            : "memory"
        );
        return (SSIZE)r_eax;
    }

    // Syscall with 4 arguments
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
    {
        register USIZE r_ebx __asm__("ebx") = arg1;
        register USIZE r_ecx __asm__("ecx") = arg2;
        register USIZE r_edx __asm__("edx") = arg3;
        register USIZE r_esi __asm__("esi") = arg4;
        register USIZE r_eax __asm__("eax") = number;
        __asm__ volatile(
            "int $0x80\n"
            : "+r"(r_eax)
            : "r"(r_ebx), "r"(r_ecx), "r"(r_edx), "r"(r_esi)
            : "memory"
        );
        return (SSIZE)r_eax;
    }

    // Syscall with 5 arguments
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
    {
        register USIZE r_ebx __asm__("ebx") = arg1;
        register USIZE r_ecx __asm__("ecx") = arg2;
        register USIZE r_edx __asm__("edx") = arg3;
        register USIZE r_esi __asm__("esi") = arg4;
        register USIZE r_edi __asm__("edi") = arg5;
        register USIZE r_eax __asm__("eax") = number;
        __asm__ volatile(
            "int $0x80\n"
            : "+r"(r_eax)
            : "r"(r_ebx), "r"(r_ecx), "r"(r_edx), "r"(r_esi), "r"(r_edi)
            : "memory"
        );
        return (SSIZE)r_eax;
    }

    // Syscall with 6 arguments
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
    {
        register USIZE r_ebx __asm__("ebx") = arg1;
        register USIZE r_ecx __asm__("ecx") = arg2;
        register USIZE r_edx __asm__("edx") = arg3;
        register USIZE r_esi __asm__("esi") = arg4;
        register USIZE r_edi __asm__("edi") = arg5;
        register USIZE r_ebp __asm__("ebp") = arg6;
        register USIZE r_eax __asm__("eax") = number;
        __asm__ volatile(
            "int $0x80\n"
            : "+r"(r_eax)
            : "r"(r_ebx), "r"(r_ecx), "r"(r_edx), "r"(r_esi), "r"(r_edi), "r"(r_ebp)
            : "memory"
        );
        return (SSIZE)r_eax;
    }

    // AArch64 syscall wrappers
#elif defined(ARCHITECTURE_AARCH64)

    // Syscall with 0 arguments
    static inline SSIZE Call(USIZE number)
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
    static inline SSIZE Call(USIZE number, USIZE arg1)
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
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
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
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
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
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
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
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
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
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
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
    static inline SSIZE Call(USIZE number)
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
    static inline SSIZE Call(USIZE number, USIZE arg1)
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
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
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
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
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
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
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
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
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
    static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
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

};  // class System
