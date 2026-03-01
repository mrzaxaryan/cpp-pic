#pragma once

#include "core/types/primitives.h"

// Solaris/illumos syscall wrappers
// x86_64: syscall instruction, same register convention as Linux, but carry flag
//         indicates error (RAX = positive errno). We negate on error to match
//         the Linux convention (negative return = error).
// i386:   int $0x91 (trap gate), syscall number in EAX, arguments on the stack
//         (kernel expects dummy return address at ESP, args at ESP+4, ESP+8, ...).
//         Carry flag indicates error (EAX = positive errno). We negate on error.
// aarch64: svc #0, syscall number in X8, arguments in X0-X5, return in X0.
//          Carry flag (C bit in NZCV) indicates error. We negate X0 on error.
class System
{
public:

#if defined(ARCHITECTURE_X86_64)

	// Syscall with 0 arguments
	// Note: Solaris kernel may write rval[1] to RDX on return, so RDX must be clobbered.
	static inline SSIZE Call(USIZE number)
	{
		register USIZE r_rax __asm__("rax") = number;
		__asm__ volatile(
			"syscall\n"
			"jnc 1f\n"
			"negq %%rax\n"
			"1:\n"
			: "+r"(r_rax)
			:
			: "rcx", "rdx", "r11", "memory", "cc"
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
			"jnc 1f\n"
			"negq %%rax\n"
			"1:\n"
			: "+r"(r_rax)
			: "r"(r_rdi)
			: "rcx", "rdx", "r11", "memory", "cc"
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
			"jnc 1f\n"
			"negq %%rax\n"
			"1:\n"
			: "+r"(r_rax)
			: "r"(r_rdi), "r"(r_rsi)
			: "rcx", "rdx", "r11", "memory", "cc"
		);
		return (SSIZE)r_rax;
	}

	// Syscall with 3 arguments
	// RDX is used as input and clobbered by kernel (rval[1]), so mark as "+r".
	static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
	{
		register USIZE r_rdi __asm__("rdi") = arg1;
		register USIZE r_rsi __asm__("rsi") = arg2;
		register USIZE r_rdx __asm__("rdx") = arg3;
		register USIZE r_rax __asm__("rax") = number;
		__asm__ volatile(
			"syscall\n"
			"jnc 1f\n"
			"negq %%rax\n"
			"1:\n"
			: "+r"(r_rax), "+r"(r_rdx)
			: "r"(r_rdi), "r"(r_rsi)
			: "rcx", "r11", "memory", "cc"
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
			"jnc 1f\n"
			"negq %%rax\n"
			"1:\n"
			: "+r"(r_rax), "+r"(r_rdx)
			: "r"(r_rdi), "r"(r_rsi), "r"(r_r10)
			: "rcx", "r11", "memory", "cc"
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
			"jnc 1f\n"
			"negq %%rax\n"
			"1:\n"
			: "+r"(r_rax), "+r"(r_rdx)
			: "r"(r_rdi), "r"(r_rsi), "r"(r_r10), "r"(r_r8)
			: "rcx", "r11", "memory", "cc"
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
			"jnc 1f\n"
			"negq %%rax\n"
			"1:\n"
			: "+r"(r_rax), "+r"(r_rdx)
			: "r"(r_rdi), "r"(r_rsi), "r"(r_r10), "r"(r_r8), "r"(r_r9)
			: "rcx", "r11", "memory", "cc"
		);
		return (SSIZE)r_rax;
	}

	// i386 syscall wrappers
	// Solaris i386 uses int $0x91 with arguments on the stack.
	// The kernel expects: [ESP] = dummy return address, [ESP+4] = arg1, etc.
	// All arguments are loaded into registers first (via "r" constraints) to
	// avoid ESP-relative operand issues when the stack pointer is modified.
#elif defined(ARCHITECTURE_I386)

	// Syscall with 0 arguments
	static inline SSIZE Call(USIZE number)
	{
		SSIZE ret;
		__asm__ volatile(
			"pushl $0\n"
			"int $0x91\n"
			"jnc 1f\n"
			"negl %%eax\n"
			"1:\n"
			"addl $4, %%esp\n"
			: "=a"(ret)
			: "a"(number)
			: "memory", "cc"
		);
		return ret;
	}

	// Syscall with 1 argument
	static inline SSIZE Call(USIZE number, USIZE arg1)
	{
		SSIZE ret;
		register USIZE r_ebx __asm__("ebx") = arg1;
		__asm__ volatile(
			"pushl %%ebx\n"
			"pushl $0\n"
			"int $0x91\n"
			"jnc 1f\n"
			"negl %%eax\n"
			"1:\n"
			"addl $8, %%esp\n"
			: "=a"(ret)
			: "a"(number), "r"(r_ebx)
			: "memory", "cc"
		);
		return ret;
	}

	// Syscall with 2 arguments
	static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
	{
		SSIZE ret;
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_ecx __asm__("ecx") = arg2;
		__asm__ volatile(
			"pushl %%ecx\n"
			"pushl %%ebx\n"
			"pushl $0\n"
			"int $0x91\n"
			"jnc 1f\n"
			"negl %%eax\n"
			"1:\n"
			"addl $12, %%esp\n"
			: "=a"(ret)
			: "a"(number), "r"(r_ebx), "r"(r_ecx)
			: "memory", "cc"
		);
		return ret;
	}

	// Syscall with 3 arguments
	static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
	{
		SSIZE ret;
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_ecx __asm__("ecx") = arg2;
		register USIZE r_edx __asm__("edx") = arg3;
		__asm__ volatile(
			"pushl %%edx\n"
			"pushl %%ecx\n"
			"pushl %%ebx\n"
			"pushl $0\n"
			"int $0x91\n"
			"jnc 1f\n"
			"negl %%eax\n"
			"1:\n"
			"addl $16, %%esp\n"
			: "=a"(ret)
			: "a"(number), "r"(r_ebx), "r"(r_ecx), "r"(r_edx)
			: "memory", "cc"
		);
		return ret;
	}

	// Syscall with 4 arguments
	static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
	{
		SSIZE ret;
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_ecx __asm__("ecx") = arg2;
		register USIZE r_edx __asm__("edx") = arg3;
		register USIZE r_esi __asm__("esi") = arg4;
		__asm__ volatile(
			"pushl %%esi\n"
			"pushl %%edx\n"
			"pushl %%ecx\n"
			"pushl %%ebx\n"
			"pushl $0\n"
			"int $0x91\n"
			"jnc 1f\n"
			"negl %%eax\n"
			"1:\n"
			"addl $20, %%esp\n"
			: "=a"(ret)
			: "a"(number), "r"(r_ebx), "r"(r_ecx), "r"(r_edx), "r"(r_esi)
			: "memory", "cc"
		);
		return ret;
	}

	// Syscall with 5 arguments
	static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
	{
		SSIZE ret;
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_ecx __asm__("ecx") = arg2;
		register USIZE r_edx __asm__("edx") = arg3;
		register USIZE r_esi __asm__("esi") = arg4;
		register USIZE r_edi __asm__("edi") = arg5;
		__asm__ volatile(
			"pushl %%edi\n"
			"pushl %%esi\n"
			"pushl %%edx\n"
			"pushl %%ecx\n"
			"pushl %%ebx\n"
			"pushl $0\n"
			"int $0x91\n"
			"jnc 1f\n"
			"negl %%eax\n"
			"1:\n"
			"addl $24, %%esp\n"
			: "=a"(ret)
			: "a"(number), "r"(r_ebx), "r"(r_ecx), "r"(r_edx), "r"(r_esi), "r"(r_edi)
			: "memory", "cc"
		);
		return ret;
	}

	// Syscall with 6 arguments
	// Cannot use register __asm__("ebp") for arg6 — it conflicts with the
	// frame pointer at -O1+ under LTO. Push arg6 from "g" constraint first
	// (ESP is still valid at that point), then push remaining from registers.
	static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
	{
		SSIZE ret;
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_ecx __asm__("ecx") = arg2;
		register USIZE r_edx __asm__("edx") = arg3;
		register USIZE r_esi __asm__("esi") = arg4;
		register USIZE r_edi __asm__("edi") = arg5;
		__asm__ volatile(
			"pushl %[a6]\n"
			"pushl %%edi\n"
			"pushl %%esi\n"
			"pushl %%edx\n"
			"pushl %%ecx\n"
			"pushl %%ebx\n"
			"pushl $0\n"
			"int $0x91\n"
			"jnc 1f\n"
			"negl %%eax\n"
			"1:\n"
			"addl $28, %%esp\n"
			: "=a"(ret)
			: "a"(number), "r"(r_ebx), "r"(r_ecx), "r"(r_edx), "r"(r_esi), "r"(r_edi), [a6] "g"(arg6)
			: "memory", "cc"
		);
		return ret;
	}

	// AArch64 syscall wrappers
	// Solaris aarch64 uses svc #0 with X8 as the syscall number (standard ARM64
	// convention, NOT svc #0x80 / X16 like macOS). Carry flag (C bit in NZCV)
	// indicates error. We negate X0 on error to match the Linux convention.
#elif defined(ARCHITECTURE_AARCH64)

	// Syscall with 0 arguments
	// Note: Solaris kernel may write rval[1] to X1 on return, so X1 must be clobbered.
	static inline SSIZE Call(USIZE number)
	{
		register USIZE x0 __asm__("x0");
		register USIZE x8 __asm__("x8") = number;
		__asm__ volatile(
			"svc #0\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "=r"(x0)
			: "r"(x8)
			: "x1", "memory", "cc"
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
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0)
			: "r"(x8)
			: "x1", "memory", "cc"
		);
		return (SSIZE)x0;
	}

	// Syscall with 2 arguments
	// X1 is used as input and clobbered by kernel (rval[1]), so mark as "+r".
	static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
	{
		register USIZE x0 __asm__("x0") = arg1;
		register USIZE x1 __asm__("x1") = arg2;
		register USIZE x8 __asm__("x8") = number;
		__asm__ volatile(
			"svc #0\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0), "+r"(x1)
			: "r"(x8)
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
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0), "+r"(x1)
			: "r"(x2), "r"(x8)
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
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0), "+r"(x1)
			: "r"(x2), "r"(x3), "r"(x8)
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
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0), "+r"(x1)
			: "r"(x2), "r"(x3), "r"(x4), "r"(x8)
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
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0), "+r"(x1)
			: "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8)
			: "memory", "cc"
		);
		return (SSIZE)x0;
	}

#endif

};  // class System
