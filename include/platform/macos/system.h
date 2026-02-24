#pragma once

#include "primitives.h"

// macOS BSD syscall wrappers
// x86_64: syscall instruction, same register convention as Linux, but carry flag
//         indicates error (RAX = positive errno). We negate on error to match
//         the Linux convention (negative return = error).
// aarch64: svc #0x80 (not svc #0), syscall number in X16 (not X8), carry flag
//          in NZCV for error. We negate X0 on error.
class System
{
public:

#if defined(ARCHITECTURE_X86_64)

	// Syscall with 0 arguments
	// Note: macOS kernel writes rval[1] to RDX on return, so RDX must be clobbered.
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

#elif defined(ARCHITECTURE_AARCH64)

	// Syscall with 0 arguments
	// Note: macOS kernel writes rval[1] to X1 on return, so X1 must be clobbered.
	static inline SSIZE Call(USIZE number)
	{
		register USIZE x0 __asm__("x0");
		register USIZE x16 __asm__("x16") = number;
		__asm__ volatile(
			"svc #0x80\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "=r"(x0)
			: "r"(x16)
			: "x1", "memory", "cc"
		);
		return (SSIZE)x0;
	}

	// Syscall with 1 argument
	static inline SSIZE Call(USIZE number, USIZE arg1)
	{
		register USIZE x0 __asm__("x0") = arg1;
		register USIZE x16 __asm__("x16") = number;
		__asm__ volatile(
			"svc #0x80\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0)
			: "r"(x16)
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
		register USIZE x16 __asm__("x16") = number;
		__asm__ volatile(
			"svc #0x80\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0), "+r"(x1)
			: "r"(x16)
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
		register USIZE x16 __asm__("x16") = number;
		__asm__ volatile(
			"svc #0x80\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0), "+r"(x1)
			: "r"(x2), "r"(x16)
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
		register USIZE x16 __asm__("x16") = number;
		__asm__ volatile(
			"svc #0x80\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0), "+r"(x1)
			: "r"(x2), "r"(x3), "r"(x16)
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
		register USIZE x16 __asm__("x16") = number;
		__asm__ volatile(
			"svc #0x80\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0), "+r"(x1)
			: "r"(x2), "r"(x3), "r"(x4), "r"(x16)
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
		register USIZE x16 __asm__("x16") = number;
		__asm__ volatile(
			"svc #0x80\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0), "+r"(x1)
			: "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x16)
			: "memory", "cc"
		);
		return (SSIZE)x0;
	}

#endif

};  // class System
