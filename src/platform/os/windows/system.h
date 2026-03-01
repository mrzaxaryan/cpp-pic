#pragma once

#include "platform.h"
#include "peb.h"

#define SYSCALL_SSN_INVALID ((INT32) - 1)

typedef struct SYSCALL_ENTRY
{
    INT32 ssn;
    PVOID syscallAddress;
} SYSCALL_ENTRY;

class System
{
public:
    static SYSCALL_ENTRY ResolveSyscallEntry(UINT64 functionNameHash);

#if defined(ARCHITECTURE_X86_64)

    // Indirect syscall with 0 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry)
    {
        register USIZE r_rax __asm__("rax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "call *%[gadget]\n"
            : "+r"(r_rax)
            : [gadget] "r"(entry.syscallAddress)
            : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
        return (NTSTATUS)r_rax;
    }

    // Indirect syscall with 1 argument
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1)
    {
        register USIZE r_r10 __asm__("r10") = a1;
        register USIZE r_rax __asm__("rax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "call *%[gadget]\n"
            : "+r"(r_rax), "+r"(r_r10)
            : [gadget] "r"(entry.syscallAddress)
            : "rcx", "rdx", "r8", "r9", "r11", "memory");
        return (NTSTATUS)r_rax;
    }

    // Indirect syscall with 2 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2)
    {
        register USIZE r_r10 __asm__("r10") = a1;
        register USIZE r_rdx __asm__("rdx") = a2;
        register USIZE r_rax __asm__("rax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "call *%[gadget]\n"
            : "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx)
            : [gadget] "r"(entry.syscallAddress)
            : "rcx", "r8", "r9", "r11", "memory");
        return (NTSTATUS)r_rax;
    }

    // Indirect syscall with 3 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3)
    {
        register USIZE r_r10 __asm__("r10") = a1;
        register USIZE r_rdx __asm__("rdx") = a2;
        register USIZE r_r8 __asm__("r8") = a3;
        register USIZE r_rax __asm__("rax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "call *%[gadget]\n"
            : "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8)
            : [gadget] "r"(entry.syscallAddress)
            : "rcx", "r9", "r11", "memory");
        return (NTSTATUS)r_rax;
    }

    // Indirect syscall with 4 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4)
    {
        register USIZE r_r10 __asm__("r10") = a1;
        register USIZE r_rdx __asm__("rdx") = a2;
        register USIZE r_r8 __asm__("r8") = a3;
        register USIZE r_r9 __asm__("r9") = a4;
        register USIZE r_rax __asm__("rax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "call *%[gadget]\n"
            : "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
            : [gadget] "r"(entry.syscallAddress)
            : "rcx", "r11", "memory");
        return (NTSTATUS)r_rax;
    }

    // Indirect syscall with 5 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5)
    {
        register USIZE r_r10 __asm__("r10") = a1;
        register USIZE r_rdx __asm__("rdx") = a2;
        register USIZE r_r8 __asm__("r8") = a3;
        register USIZE r_r9 __asm__("r9") = a4;
        register USIZE r_rax __asm__("rax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "sub $0x28, %%rsp\n"
            "mov %[a5], 0x20(%%rsp)\n"
            "call *%[gadget]\n"
            "add $0x28, %%rsp\n"
            : "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
            : [a5] "r"(a5), [gadget] "r"(entry.syscallAddress)
            : "rcx", "r11", "memory");
        return (NTSTATUS)r_rax;
    }

    // Indirect syscall with 6 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6)
    {
        register USIZE r_r10 __asm__("r10") = a1;
        register USIZE r_rdx __asm__("rdx") = a2;
        register USIZE r_r8 __asm__("r8") = a3;
        register USIZE r_r9 __asm__("r9") = a4;
        register USIZE r_rax __asm__("rax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "sub $0x30, %%rsp\n"
            "mov %[a5], 0x20(%%rsp)\n"
            "mov %[a6], 0x28(%%rsp)\n"
            "call *%[gadget]\n"
            "add $0x30, %%rsp\n"
            : "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
            : [a5] "r"(a5), [a6] "r"(a6), [gadget] "r"(entry.syscallAddress)
            : "rcx", "r11", "memory");
        return (NTSTATUS)r_rax;
    }

    // Indirect syscall with 7 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7)
    {
        register USIZE r_r10 __asm__("r10") = a1;
        register USIZE r_rdx __asm__("rdx") = a2;
        register USIZE r_r8 __asm__("r8") = a3;
        register USIZE r_r9 __asm__("r9") = a4;
        register USIZE r_rax __asm__("rax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "sub $0x38, %%rsp\n"
            "mov %[a5], 0x20(%%rsp)\n"
            "mov %[a6], 0x28(%%rsp)\n"
            "mov %[a7], 0x30(%%rsp)\n"
            "call *%[gadget]\n"
            "add $0x38, %%rsp\n"
            : "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
            : [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7),
              [gadget] "r"(entry.syscallAddress)
            : "rcx", "r11", "memory");
        return (NTSTATUS)r_rax;
    }

    // Indirect syscall with 8 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8)
    {
        register USIZE r_r10 __asm__("r10") = a1;
        register USIZE r_rdx __asm__("rdx") = a2;
        register USIZE r_r8 __asm__("r8") = a3;
        register USIZE r_r9 __asm__("r9") = a4;
        register USIZE r_rax __asm__("rax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "sub $0x40, %%rsp\n"
            "mov %[a5], 0x20(%%rsp)\n"
            "mov %[a6], 0x28(%%rsp)\n"
            "mov %[a7], 0x30(%%rsp)\n"
            "mov %[a8], 0x38(%%rsp)\n"
            "call *%[gadget]\n"
            "add $0x40, %%rsp\n"
            : "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
            : [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7), [a8] "r"(a8),
              [gadget] "r"(entry.syscallAddress)
            : "rcx", "r11", "memory");
        return (NTSTATUS)r_rax;
    }

    // Indirect syscall with 9 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9)
    {
        register USIZE r_r10 __asm__("r10") = a1;
        register USIZE r_rdx __asm__("rdx") = a2;
        register USIZE r_r8 __asm__("r8") = a3;
        register USIZE r_r9 __asm__("r9") = a4;
        register USIZE r_rax __asm__("rax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "sub $0x48, %%rsp\n"
            "mov %[a5], 0x20(%%rsp)\n"
            "mov %[a6], 0x28(%%rsp)\n"
            "mov %[a7], 0x30(%%rsp)\n"
            "mov %[a8], 0x38(%%rsp)\n"
            "mov %[a9], 0x40(%%rsp)\n"
            "call *%[gadget]\n"
            "add $0x48, %%rsp\n"
            : "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
            : [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7), [a8] "r"(a8),
              [a9] "r"(a9), [gadget] "r"(entry.syscallAddress)
            : "rcx", "r11", "memory");
        return (NTSTATUS)r_rax;
    }

    // Indirect syscall with 10 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10)
    {
        register USIZE r_r10 __asm__("r10") = a1;
        register USIZE r_rdx __asm__("rdx") = a2;
        register USIZE r_r8 __asm__("r8") = a3;
        register USIZE r_r9 __asm__("r9") = a4;
        register USIZE r_rax __asm__("rax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "sub $0x50, %%rsp\n"
            "mov %[a5],  0x20(%%rsp)\n"
            "mov %[a6],  0x28(%%rsp)\n"
            "mov %[a7],  0x30(%%rsp)\n"
            "mov %[a8],  0x38(%%rsp)\n"
            "mov %[a9],  0x40(%%rsp)\n"
            "mov %[a10], 0x48(%%rsp)\n"
            "call *%[gadget]\n"
            "add $0x50, %%rsp\n"
            : "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
            : [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7), [a8] "r"(a8),
              [a9] "r"(a9), [a10] "r"(a10), [gadget] "r"(entry.syscallAddress)
            : "rcx", "r11", "memory");
        return (NTSTATUS)r_rax;
    }

    // Indirect syscall with 11 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11)
    {
        register USIZE r_r10 __asm__("r10") = a1;
        register USIZE r_rdx __asm__("rdx") = a2;
        register USIZE r_r8 __asm__("r8") = a3;
        register USIZE r_r9 __asm__("r9") = a4;
        register USIZE r_rax __asm__("rax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "mov %[a11], %%rcx\n"
            "sub $0x58, %%rsp\n"
            "mov %[a5],  0x20(%%rsp)\n"
            "mov %[a6],  0x28(%%rsp)\n"
            "mov %[a7],  0x30(%%rsp)\n"
            "mov %[a8],  0x38(%%rsp)\n"
            "mov %[a9],  0x40(%%rsp)\n"
            "mov %[a10], 0x48(%%rsp)\n"
            "mov %%rcx,  0x50(%%rsp)\n"
            "call *%[gadget]\n"
            "add $0x58, %%rsp\n"
            : "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
            : [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7), [a8] "r"(a8),
              [a9] "r"(a9), [a10] "r"(a10), [a11] "m"(a11), [gadget] "r"(entry.syscallAddress)
            : "rcx", "r11", "memory");
        return (NTSTATUS)r_rax;
    }

    // Indirect syscall with 12 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12)
    {
        register USIZE r_r10 __asm__("r10") = a1;
        register USIZE r_rdx __asm__("rdx") = a2;
        register USIZE r_r8 __asm__("r8") = a3;
        register USIZE r_r9 __asm__("r9") = a4;
        register USIZE r_rax __asm__("rax") = (USIZE)entry.ssn;
        USIZE args[] = {a5, a6, a7, a8, a9, a10, a11, a12};
        __asm__ volatile(
            "sub $0x60, %%rsp\n"
            "mov (%[args]), %%rcx\n"
            "mov %%rcx, 0x20(%%rsp)\n"
            "mov 0x8(%[args]), %%rcx\n"
            "mov %%rcx, 0x28(%%rsp)\n"
            "mov 0x10(%[args]), %%rcx\n"
            "mov %%rcx, 0x30(%%rsp)\n"
            "mov 0x18(%[args]), %%rcx\n"
            "mov %%rcx, 0x38(%%rsp)\n"
            "mov 0x20(%[args]), %%rcx\n"
            "mov %%rcx, 0x40(%%rsp)\n"
            "mov 0x28(%[args]), %%rcx\n"
            "mov %%rcx, 0x48(%%rsp)\n"
            "mov 0x30(%[args]), %%rcx\n"
            "mov %%rcx, 0x50(%%rsp)\n"
            "mov 0x38(%[args]), %%rcx\n"
            "mov %%rcx, 0x58(%%rsp)\n"
            "call *%[gadget]\n"
            "add $0x60, %%rsp\n"
            : "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
            : [args] "r"(args), [gadget] "r"(entry.syscallAddress)
            : "rcx", "r11", "memory");
        return (NTSTATUS)r_rax;
    }

    // Indirect syscall with 13 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12, USIZE a13)
    {
        register USIZE r_r10 __asm__("r10") = a1;
        register USIZE r_rdx __asm__("rdx") = a2;
        register USIZE r_r8 __asm__("r8") = a3;
        register USIZE r_r9 __asm__("r9") = a4;
        register USIZE r_rax __asm__("rax") = (USIZE)entry.ssn;
        USIZE args[] = {a5, a6, a7, a8, a9, a10, a11, a12, a13};
        __asm__ volatile(
            "sub $0x68, %%rsp\n"
            "mov (%[args]), %%rcx\n"
            "mov %%rcx, 0x20(%%rsp)\n"
            "mov 0x8(%[args]), %%rcx\n"
            "mov %%rcx, 0x28(%%rsp)\n"
            "mov 0x10(%[args]), %%rcx\n"
            "mov %%rcx, 0x30(%%rsp)\n"
            "mov 0x18(%[args]), %%rcx\n"
            "mov %%rcx, 0x38(%%rsp)\n"
            "mov 0x20(%[args]), %%rcx\n"
            "mov %%rcx, 0x40(%%rsp)\n"
            "mov 0x28(%[args]), %%rcx\n"
            "mov %%rcx, 0x48(%%rsp)\n"
            "mov 0x30(%[args]), %%rcx\n"
            "mov %%rcx, 0x50(%%rsp)\n"
            "mov 0x38(%[args]), %%rcx\n"
            "mov %%rcx, 0x58(%%rsp)\n"
            "mov 0x40(%[args]), %%rcx\n"
            "mov %%rcx, 0x60(%%rsp)\n"
            "call *%[gadget]\n"
            "add $0x68, %%rsp\n"
            : "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
            : [args] "r"(args), [gadget] "r"(entry.syscallAddress)
            : "rcx", "r11", "memory");
        return (NTSTATUS)r_rax;
    }

    // Indirect syscall with 14 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12, USIZE a13, USIZE a14)
    {
        register USIZE r_r10 __asm__("r10") = a1;
        register USIZE r_rdx __asm__("rdx") = a2;
        register USIZE r_r8 __asm__("r8") = a3;
        register USIZE r_r9 __asm__("r9") = a4;
        register USIZE r_rax __asm__("rax") = (USIZE)entry.ssn;
        USIZE args[] = {a5, a6, a7, a8, a9, a10, a11, a12, a13, a14};
        __asm__ volatile(
            "sub $0x70, %%rsp\n"
            "mov (%[args]), %%rcx\n"
            "mov %%rcx, 0x20(%%rsp)\n"
            "mov 0x8(%[args]), %%rcx\n"
            "mov %%rcx, 0x28(%%rsp)\n"
            "mov 0x10(%[args]), %%rcx\n"
            "mov %%rcx, 0x30(%%rsp)\n"
            "mov 0x18(%[args]), %%rcx\n"
            "mov %%rcx, 0x38(%%rsp)\n"
            "mov 0x20(%[args]), %%rcx\n"
            "mov %%rcx, 0x40(%%rsp)\n"
            "mov 0x28(%[args]), %%rcx\n"
            "mov %%rcx, 0x48(%%rsp)\n"
            "mov 0x30(%[args]), %%rcx\n"
            "mov %%rcx, 0x50(%%rsp)\n"
            "mov 0x38(%[args]), %%rcx\n"
            "mov %%rcx, 0x58(%%rsp)\n"
            "mov 0x40(%[args]), %%rcx\n"
            "mov %%rcx, 0x60(%%rsp)\n"
            "mov 0x48(%[args]), %%rcx\n"
            "mov %%rcx, 0x68(%%rsp)\n"
            "call *%[gadget]\n"
            "add $0x70, %%rsp\n"
            : "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
            : [args] "r"(args), [gadget] "r"(entry.syscallAddress)
            : "rcx", "r11", "memory");
        return (NTSTATUS)r_rax;
    }

#elif defined(ARCHITECTURE_I386)

    // Windows i386 syscalls are STACK-BASED: all arguments are pushed onto the stack,
    // EAX holds the syscall number, and the gadget handles the kernel transition.
    //
    // Native i386: gadget = KiFastSystemCall (mov edx,esp; sysenter)
    // WoW64:       gadget = trampoline (jmp to wow64cpu 32-to-64 transition)
    //
    // We push a dummy DWORD to fill the "original caller return address" slot,
    // so the kernel/handler finds arguments at the expected stack offset:
    //   [ESP+0x00] = return address (from our call instruction)
    //   [ESP+0x04] = dummy (fills expected second return address slot)
    //   [ESP+0x08] = arg1
    //   [ESP+0x0C] = arg2 ...
    //
    // EAX is pre-loaded with the SSN by the compiler via register variable,
    // and the gadget address is passed as a separate register operand to
    // prevent register aliasing (the compiler must not assign [gadget] to EAX).

    // Indirect syscall with 0 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry)
    {
        register USIZE r_eax __asm__("eax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "pushl $0\n"
            "movl %[gadget], %%edx\n"
            "call *%%edx\n"
            "addl $4, %%esp\n"
            : "+a"(r_eax)
            : [gadget] "r"(entry.syscallAddress)
            : "ecx", "edx", "memory");
        return (NTSTATUS)r_eax;
    }

    // Indirect syscall with 1 argument
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1)
    {
        register USIZE r_eax __asm__("eax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "pushl %[a1]\n"
            "pushl $0\n"
            "movl %[gadget], %%edx\n"
            "call *%%edx\n"
            "addl $8, %%esp\n"
            : "+a"(r_eax)
            : [gadget] "r"(entry.syscallAddress), [a1] "r"(a1)
            : "ecx", "edx", "memory");
        return (NTSTATUS)r_eax;
    }

    // Indirect syscall with 2 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2)
    {
        USIZE args[] = {a1, a2};
        register USIZE r_eax __asm__("eax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "pushl 4(%[args])\n"
            "pushl (%[args])\n"
            "pushl $0\n"
            "movl %[gadget], %%edx\n"
            "call *%%edx\n"
            "addl $12, %%esp\n"
            : "+a"(r_eax)
            : [gadget] "r"(entry.syscallAddress), [args] "r"(args)
            : "ecx", "edx", "memory");
        return (NTSTATUS)r_eax;
    }

    // Indirect syscall with 3 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3)
    {
        USIZE args[] = {a1, a2, a3};
        register USIZE r_eax __asm__("eax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "pushl 8(%[args])\n"
            "pushl 4(%[args])\n"
            "pushl (%[args])\n"
            "pushl $0\n"
            "movl %[gadget], %%edx\n"
            "call *%%edx\n"
            "addl $16, %%esp\n"
            : "+a"(r_eax)
            : [gadget] "r"(entry.syscallAddress), [args] "r"(args)
            : "ecx", "edx", "memory");
        return (NTSTATUS)r_eax;
    }

    // Indirect syscall with 4 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4)
    {
        USIZE args[] = {a1, a2, a3, a4};
        register USIZE r_eax __asm__("eax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "pushl 12(%[args])\n"
            "pushl 8(%[args])\n"
            "pushl 4(%[args])\n"
            "pushl (%[args])\n"
            "pushl $0\n"
            "movl %[gadget], %%edx\n"
            "call *%%edx\n"
            "addl $20, %%esp\n"
            : "+a"(r_eax)
            : [gadget] "r"(entry.syscallAddress), [args] "r"(args)
            : "ecx", "edx", "memory");
        return (NTSTATUS)r_eax;
    }

    // Indirect syscall with 5 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5)
    {
        USIZE args[] = {a1, a2, a3, a4, a5};
        register USIZE r_eax __asm__("eax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "pushl 16(%[args])\n"
            "pushl 12(%[args])\n"
            "pushl 8(%[args])\n"
            "pushl 4(%[args])\n"
            "pushl (%[args])\n"
            "pushl $0\n"
            "movl %[gadget], %%edx\n"
            "call *%%edx\n"
            "addl $24, %%esp\n"
            : "+a"(r_eax)
            : [gadget] "r"(entry.syscallAddress), [args] "r"(args)
            : "ecx", "edx", "memory");
        return (NTSTATUS)r_eax;
    }

    // Indirect syscall with 6 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6)
    {
        USIZE args[] = {a1, a2, a3, a4, a5, a6};
        register USIZE r_eax __asm__("eax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "pushl 20(%[args])\n"
            "pushl 16(%[args])\n"
            "pushl 12(%[args])\n"
            "pushl 8(%[args])\n"
            "pushl 4(%[args])\n"
            "pushl (%[args])\n"
            "pushl $0\n"
            "movl %[gadget], %%edx\n"
            "call *%%edx\n"
            "addl $28, %%esp\n"
            : "+a"(r_eax)
            : [gadget] "r"(entry.syscallAddress), [args] "r"(args)
            : "ecx", "edx", "memory");
        return (NTSTATUS)r_eax;
    }

    // Indirect syscall with 7 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7)
    {
        USIZE args[] = {a1, a2, a3, a4, a5, a6, a7};
        register USIZE r_eax __asm__("eax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "pushl 24(%[args])\n"
            "pushl 20(%[args])\n"
            "pushl 16(%[args])\n"
            "pushl 12(%[args])\n"
            "pushl 8(%[args])\n"
            "pushl 4(%[args])\n"
            "pushl (%[args])\n"
            "pushl $0\n"
            "movl %[gadget], %%edx\n"
            "call *%%edx\n"
            "addl $32, %%esp\n"
            : "+a"(r_eax)
            : [gadget] "r"(entry.syscallAddress), [args] "r"(args)
            : "ecx", "edx", "memory");
        return (NTSTATUS)r_eax;
    }

    // Indirect syscall with 8 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8)
    {
        USIZE args[] = {a1, a2, a3, a4, a5, a6, a7, a8};
        register USIZE r_eax __asm__("eax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "pushl 28(%[args])\n"
            "pushl 24(%[args])\n"
            "pushl 20(%[args])\n"
            "pushl 16(%[args])\n"
            "pushl 12(%[args])\n"
            "pushl 8(%[args])\n"
            "pushl 4(%[args])\n"
            "pushl (%[args])\n"
            "pushl $0\n"
            "movl %[gadget], %%edx\n"
            "call *%%edx\n"
            "addl $36, %%esp\n"
            : "+a"(r_eax)
            : [gadget] "r"(entry.syscallAddress), [args] "r"(args)
            : "ecx", "edx", "memory");
        return (NTSTATUS)r_eax;
    }

    // Indirect syscall with 9 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9)
    {
        USIZE args[] = {a1, a2, a3, a4, a5, a6, a7, a8, a9};
        register USIZE r_eax __asm__("eax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "pushl 32(%[args])\n"
            "pushl 28(%[args])\n"
            "pushl 24(%[args])\n"
            "pushl 20(%[args])\n"
            "pushl 16(%[args])\n"
            "pushl 12(%[args])\n"
            "pushl 8(%[args])\n"
            "pushl 4(%[args])\n"
            "pushl (%[args])\n"
            "pushl $0\n"
            "movl %[gadget], %%edx\n"
            "call *%%edx\n"
            "addl $40, %%esp\n"
            : "+a"(r_eax)
            : [gadget] "r"(entry.syscallAddress), [args] "r"(args)
            : "ecx", "edx", "memory");
        return (NTSTATUS)r_eax;
    }

    // Indirect syscall with 10 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10)
    {
        USIZE args[] = {a1, a2, a3, a4, a5, a6, a7, a8, a9, a10};
        register USIZE r_eax __asm__("eax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "pushl 36(%[args])\n"
            "pushl 32(%[args])\n"
            "pushl 28(%[args])\n"
            "pushl 24(%[args])\n"
            "pushl 20(%[args])\n"
            "pushl 16(%[args])\n"
            "pushl 12(%[args])\n"
            "pushl 8(%[args])\n"
            "pushl 4(%[args])\n"
            "pushl (%[args])\n"
            "pushl $0\n"
            "movl %[gadget], %%edx\n"
            "call *%%edx\n"
            "addl $44, %%esp\n"
            : "+a"(r_eax)
            : [gadget] "r"(entry.syscallAddress), [args] "r"(args)
            : "ecx", "edx", "memory");
        return (NTSTATUS)r_eax;
    }

    // Indirect syscall with 11 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11)
    {
        USIZE args[] = {a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11};
        register USIZE r_eax __asm__("eax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "pushl 40(%[args])\n"
            "pushl 36(%[args])\n"
            "pushl 32(%[args])\n"
            "pushl 28(%[args])\n"
            "pushl 24(%[args])\n"
            "pushl 20(%[args])\n"
            "pushl 16(%[args])\n"
            "pushl 12(%[args])\n"
            "pushl 8(%[args])\n"
            "pushl 4(%[args])\n"
            "pushl (%[args])\n"
            "pushl $0\n"
            "movl %[gadget], %%edx\n"
            "call *%%edx\n"
            "addl $48, %%esp\n"
            : "+a"(r_eax)
            : [gadget] "r"(entry.syscallAddress), [args] "r"(args)
            : "ecx", "edx", "memory");
        return (NTSTATUS)r_eax;
    }

    // Indirect syscall with 12 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12)
    {
        USIZE args[] = {a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12};
        register USIZE r_eax __asm__("eax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "pushl 44(%[args])\n"
            "pushl 40(%[args])\n"
            "pushl 36(%[args])\n"
            "pushl 32(%[args])\n"
            "pushl 28(%[args])\n"
            "pushl 24(%[args])\n"
            "pushl 20(%[args])\n"
            "pushl 16(%[args])\n"
            "pushl 12(%[args])\n"
            "pushl 8(%[args])\n"
            "pushl 4(%[args])\n"
            "pushl (%[args])\n"
            "pushl $0\n"
            "movl %[gadget], %%edx\n"
            "call *%%edx\n"
            "addl $52, %%esp\n"
            : "+a"(r_eax)
            : [gadget] "r"(entry.syscallAddress), [args] "r"(args)
            : "ecx", "edx", "memory");
        return (NTSTATUS)r_eax;
    }

    // Indirect syscall with 13 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12, USIZE a13)
    {
        USIZE args[] = {a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13};
        register USIZE r_eax __asm__("eax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "pushl 48(%[args])\n"
            "pushl 44(%[args])\n"
            "pushl 40(%[args])\n"
            "pushl 36(%[args])\n"
            "pushl 32(%[args])\n"
            "pushl 28(%[args])\n"
            "pushl 24(%[args])\n"
            "pushl 20(%[args])\n"
            "pushl 16(%[args])\n"
            "pushl 12(%[args])\n"
            "pushl 8(%[args])\n"
            "pushl 4(%[args])\n"
            "pushl (%[args])\n"
            "pushl $0\n"
            "movl %[gadget], %%edx\n"
            "call *%%edx\n"
            "addl $56, %%esp\n"
            : "+a"(r_eax)
            : [gadget] "r"(entry.syscallAddress), [args] "r"(args)
            : "ecx", "edx", "memory");
        return (NTSTATUS)r_eax;
    }

    // Indirect syscall with 14 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12, USIZE a13, USIZE a14)
    {
        USIZE args[] = {a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14};
        register USIZE r_eax __asm__("eax") = (USIZE)entry.ssn;
        __asm__ volatile(
            "pushl 52(%[args])\n"
            "pushl 48(%[args])\n"
            "pushl 44(%[args])\n"
            "pushl 40(%[args])\n"
            "pushl 36(%[args])\n"
            "pushl 32(%[args])\n"
            "pushl 28(%[args])\n"
            "pushl 24(%[args])\n"
            "pushl 20(%[args])\n"
            "pushl 16(%[args])\n"
            "pushl 12(%[args])\n"
            "pushl 8(%[args])\n"
            "pushl 4(%[args])\n"
            "pushl (%[args])\n"
            "pushl $0\n"
            "movl %[gadget], %%edx\n"
            "call *%%edx\n"
            "addl $60, %%esp\n"
            : "+a"(r_eax)
            : [gadget] "r"(entry.syscallAddress), [args] "r"(args)
            : "ecx", "edx", "memory");
        return (NTSTATUS)r_eax;
    }

#elif defined(ARCHITECTURE_AARCH64)

    // Windows ARM64 indirect syscall via BLR to ntdll stub.
    //
    // On Windows ARM64, the syscall number is encoded in the SVC instruction's
    // immediate value (e.g., SVC #7 for syscall 7). The kernel extracts it from
    // ESR_EL1's ISS field. This is fundamentally different from Linux (x8 register)
    // and x64 (EAX register).
    //
    // ntdll stubs are just: SVC #N; RET
    // We set up args in x0-x7 (+ stack for 9+), then BLR to the stub.
    // The stub's SVC carries the correct syscall number in its immediate.
    //
    // x16 holds the stub address (intra-procedure-call scratch register).
    // x18 (TEB) is never touched. Callee-saved regs (x19-x28, x29) are safe
    // since the stub is only SVC+RET.

    // Indirect syscall with 0 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry)
    {
        register USIZE x0 __asm__("x0");
        register USIZE stub __asm__("x16") = (USIZE)entry.syscallAddress;
        __asm__ volatile(
            "blr %[stub]\n"
            : "=r"(x0), [stub] "+r"(stub)
            :
            : "x1", "x2", "x3", "x4", "x5", "x6", "x7",
              "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
              "x17", "x30", "memory");
        return (NTSTATUS)x0;
    }

    // Indirect syscall with 1 argument
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1)
    {
        register USIZE x0 __asm__("x0") = a1;
        register USIZE stub __asm__("x16") = (USIZE)entry.syscallAddress;
        __asm__ volatile(
            "blr %[stub]\n"
            : "+r"(x0), [stub] "+r"(stub)
            :
            : "x1", "x2", "x3", "x4", "x5", "x6", "x7",
              "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
              "x17", "x30", "memory");
        return (NTSTATUS)x0;
    }

    // Indirect syscall with 2 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2)
    {
        register USIZE x0 __asm__("x0") = a1;
        register USIZE x1 __asm__("x1") = a2;
        register USIZE stub __asm__("x16") = (USIZE)entry.syscallAddress;
        __asm__ volatile(
            "blr %[stub]\n"
            : "+r"(x0), "+r"(x1), [stub] "+r"(stub)
            :
            : "x2", "x3", "x4", "x5", "x6", "x7",
              "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
              "x17", "x30", "memory");
        return (NTSTATUS)x0;
    }

    // Indirect syscall with 3 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3)
    {
        register USIZE x0 __asm__("x0") = a1;
        register USIZE x1 __asm__("x1") = a2;
        register USIZE x2 __asm__("x2") = a3;
        register USIZE stub __asm__("x16") = (USIZE)entry.syscallAddress;
        __asm__ volatile(
            "blr %[stub]\n"
            : "+r"(x0), "+r"(x1), "+r"(x2), [stub] "+r"(stub)
            :
            : "x3", "x4", "x5", "x6", "x7",
              "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
              "x17", "x30", "memory");
        return (NTSTATUS)x0;
    }

    // Indirect syscall with 4 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4)
    {
        register USIZE x0 __asm__("x0") = a1;
        register USIZE x1 __asm__("x1") = a2;
        register USIZE x2 __asm__("x2") = a3;
        register USIZE x3 __asm__("x3") = a4;
        register USIZE stub __asm__("x16") = (USIZE)entry.syscallAddress;
        __asm__ volatile(
            "blr %[stub]\n"
            : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), [stub] "+r"(stub)
            :
            : "x4", "x5", "x6", "x7",
              "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
              "x17", "x30", "memory");
        return (NTSTATUS)x0;
    }

    // Indirect syscall with 5 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5)
    {
        register USIZE x0 __asm__("x0") = a1;
        register USIZE x1 __asm__("x1") = a2;
        register USIZE x2 __asm__("x2") = a3;
        register USIZE x3 __asm__("x3") = a4;
        register USIZE x4 __asm__("x4") = a5;
        register USIZE stub __asm__("x16") = (USIZE)entry.syscallAddress;
        __asm__ volatile(
            "blr %[stub]\n"
            : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), [stub] "+r"(stub)
            :
            : "x5", "x6", "x7",
              "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
              "x17", "x30", "memory");
        return (NTSTATUS)x0;
    }

    // Indirect syscall with 6 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6)
    {
        register USIZE x0 __asm__("x0") = a1;
        register USIZE x1 __asm__("x1") = a2;
        register USIZE x2 __asm__("x2") = a3;
        register USIZE x3 __asm__("x3") = a4;
        register USIZE x4 __asm__("x4") = a5;
        register USIZE x5 __asm__("x5") = a6;
        register USIZE stub __asm__("x16") = (USIZE)entry.syscallAddress;
        __asm__ volatile(
            "blr %[stub]\n"
            : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), [stub] "+r"(stub)
            :
            : "x6", "x7",
              "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
              "x17", "x30", "memory");
        return (NTSTATUS)x0;
    }

    // Indirect syscall with 7 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7)
    {
        register USIZE x0 __asm__("x0") = a1;
        register USIZE x1 __asm__("x1") = a2;
        register USIZE x2 __asm__("x2") = a3;
        register USIZE x3 __asm__("x3") = a4;
        register USIZE x4 __asm__("x4") = a5;
        register USIZE x5 __asm__("x5") = a6;
        register USIZE x6 __asm__("x6") = a7;
        register USIZE stub __asm__("x16") = (USIZE)entry.syscallAddress;
        __asm__ volatile(
            "blr %[stub]\n"
            : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), [stub] "+r"(stub)
            :
            : "x7",
              "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
              "x17", "x30", "memory");
        return (NTSTATUS)x0;
    }

    // Indirect syscall with 8 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8)
    {
        register USIZE x0 __asm__("x0") = a1;
        register USIZE x1 __asm__("x1") = a2;
        register USIZE x2 __asm__("x2") = a3;
        register USIZE x3 __asm__("x3") = a4;
        register USIZE x4 __asm__("x4") = a5;
        register USIZE x5 __asm__("x5") = a6;
        register USIZE x6 __asm__("x6") = a7;
        register USIZE x7 __asm__("x7") = a8;
        register USIZE stub __asm__("x16") = (USIZE)entry.syscallAddress;
        __asm__ volatile(
            "blr %[stub]\n"
            : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7), [stub] "+r"(stub)
            :
            : "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
              "x17", "x30", "memory");
        return (NTSTATUS)x0;
    }

    // Indirect syscall with 9 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9)
    {
        register USIZE x0 __asm__("x0") = a1;
        register USIZE x1 __asm__("x1") = a2;
        register USIZE x2 __asm__("x2") = a3;
        register USIZE x3 __asm__("x3") = a4;
        register USIZE x4 __asm__("x4") = a5;
        register USIZE x5 __asm__("x5") = a6;
        register USIZE x6 __asm__("x6") = a7;
        register USIZE x7 __asm__("x7") = a8;
        register USIZE stub __asm__("x16") = (USIZE)entry.syscallAddress;
        __asm__ volatile(
            "sub sp, sp, #16\n"
            "str %[a9], [sp]\n"
            "blr %[stub]\n"
            "add sp, sp, #16\n"
            : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7), [stub] "+r"(stub)
            : [a9] "r"(a9)
            : "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
              "x17", "x30", "memory");
        return (NTSTATUS)x0;
    }

    // Indirect syscall with 10 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10)
    {
        register USIZE x0 __asm__("x0") = a1;
        register USIZE x1 __asm__("x1") = a2;
        register USIZE x2 __asm__("x2") = a3;
        register USIZE x3 __asm__("x3") = a4;
        register USIZE x4 __asm__("x4") = a5;
        register USIZE x5 __asm__("x5") = a6;
        register USIZE x6 __asm__("x6") = a7;
        register USIZE x7 __asm__("x7") = a8;
        register USIZE stub __asm__("x16") = (USIZE)entry.syscallAddress;
        __asm__ volatile(
            "sub sp, sp, #16\n"
            "str %[a9], [sp]\n"
            "str %[a10], [sp, #8]\n"
            "blr %[stub]\n"
            "add sp, sp, #16\n"
            : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7), [stub] "+r"(stub)
            : [a9] "r"(a9), [a10] "r"(a10)
            : "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
              "x17", "x30", "memory");
        return (NTSTATUS)x0;
    }

    // Indirect syscall with 11 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11)
    {
        register USIZE x0 __asm__("x0") = a1;
        register USIZE x1 __asm__("x1") = a2;
        register USIZE x2 __asm__("x2") = a3;
        register USIZE x3 __asm__("x3") = a4;
        register USIZE x4 __asm__("x4") = a5;
        register USIZE x5 __asm__("x5") = a6;
        register USIZE x6 __asm__("x6") = a7;
        register USIZE x7 __asm__("x7") = a8;
        register USIZE stub __asm__("x16") = (USIZE)entry.syscallAddress;
        __asm__ volatile(
            "sub sp, sp, #32\n"
            "str %[a9], [sp]\n"
            "str %[a10], [sp, #8]\n"
            "str %[a11], [sp, #16]\n"
            "blr %[stub]\n"
            "add sp, sp, #32\n"
            : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7), [stub] "+r"(stub)
            : [a9] "r"(a9), [a10] "r"(a10), [a11] "r"(a11)
            : "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
              "x17", "x30", "memory");
        return (NTSTATUS)x0;
    }

    // Indirect syscall with 12 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12)
    {
        register USIZE x0 __asm__("x0") = a1;
        register USIZE x1 __asm__("x1") = a2;
        register USIZE x2 __asm__("x2") = a3;
        register USIZE x3 __asm__("x3") = a4;
        register USIZE x4 __asm__("x4") = a5;
        register USIZE x5 __asm__("x5") = a6;
        register USIZE x6 __asm__("x6") = a7;
        register USIZE x7 __asm__("x7") = a8;
        register USIZE stub __asm__("x16") = (USIZE)entry.syscallAddress;
        __asm__ volatile(
            "sub sp, sp, #32\n"
            "str %[a9], [sp]\n"
            "str %[a10], [sp, #8]\n"
            "str %[a11], [sp, #16]\n"
            "str %[a12], [sp, #24]\n"
            "blr %[stub]\n"
            "add sp, sp, #32\n"
            : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7), [stub] "+r"(stub)
            : [a9] "r"(a9), [a10] "r"(a10), [a11] "r"(a11), [a12] "r"(a12)
            : "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
              "x17", "x30", "memory");
        return (NTSTATUS)x0;
    }

    // Indirect syscall with 13 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12, USIZE a13)
    {
        register USIZE x0 __asm__("x0") = a1;
        register USIZE x1 __asm__("x1") = a2;
        register USIZE x2 __asm__("x2") = a3;
        register USIZE x3 __asm__("x3") = a4;
        register USIZE x4 __asm__("x4") = a5;
        register USIZE x5 __asm__("x5") = a6;
        register USIZE x6 __asm__("x6") = a7;
        register USIZE x7 __asm__("x7") = a8;
        register USIZE stub __asm__("x16") = (USIZE)entry.syscallAddress;
        __asm__ volatile(
            "sub sp, sp, #48\n"
            "str %[a9], [sp]\n"
            "str %[a10], [sp, #8]\n"
            "str %[a11], [sp, #16]\n"
            "str %[a12], [sp, #24]\n"
            "str %[a13], [sp, #32]\n"
            "blr %[stub]\n"
            "add sp, sp, #48\n"
            : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7), [stub] "+r"(stub)
            : [a9] "r"(a9), [a10] "r"(a10), [a11] "r"(a11), [a12] "r"(a12), [a13] "r"(a13)
            : "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
              "x17", "x30", "memory");
        return (NTSTATUS)x0;
    }

    // Indirect syscall with 14 arguments
    static inline NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12, USIZE a13, USIZE a14)
    {
        register USIZE x0 __asm__("x0") = a1;
        register USIZE x1 __asm__("x1") = a2;
        register USIZE x2 __asm__("x2") = a3;
        register USIZE x3 __asm__("x3") = a4;
        register USIZE x4 __asm__("x4") = a5;
        register USIZE x5 __asm__("x5") = a6;
        register USIZE x6 __asm__("x6") = a7;
        register USIZE x7 __asm__("x7") = a8;
        register USIZE stub __asm__("x16") = (USIZE)entry.syscallAddress;
        __asm__ volatile(
            "sub sp, sp, #48\n"
            "str %[a9], [sp]\n"
            "str %[a10], [sp, #8]\n"
            "str %[a11], [sp, #16]\n"
            "str %[a12], [sp, #24]\n"
            "str %[a13], [sp, #32]\n"
            "str %[a14], [sp, #40]\n"
            "blr %[stub]\n"
            "add sp, sp, #48\n"
            : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7), [stub] "+r"(stub)
            : [a9] "r"(a9), [a10] "r"(a10), [a11] "r"(a11), [a12] "r"(a12), [a13] "r"(a13), [a14] "r"(a14)
            : "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
              "x17", "x30", "memory");
        return (NTSTATUS)x0;
    }

#else
#error "Unsupported architecture"
#endif
};

#define ResolveSyscall(functionName) System::ResolveSyscallEntry(Djb2::HashCompileTime(functionName))
