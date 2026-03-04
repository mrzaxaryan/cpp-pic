/**
 * @file system.riscv64.h
 * @brief RISC-V 64-bit FreeBSD syscall implementation via inline assembly.
 *
 * @details Provides System::Call overloads (0-6 arguments) that invoke FreeBSD
 * syscalls using the ecall instruction with the syscall number in a7 (x17).
 * Arguments are passed in a0-a5 (x10-x15). On FreeBSD RISC-V, errors are
 * indicated via the t0 (x5) register: t0=0 on success, t0=1 on error
 * (a0 contains the positive errno). The inline assembly negates a0 on
 * error to normalize to the negative-return convention.
 */
#pragma once

#include "core/types/primitives.h"

class System
{
public:

	// Syscall with 0 arguments
	static inline SSIZE Call(USIZE number)
	{
		register USIZE a0 __asm__("a0");
		register USIZE a7 __asm__("a7") = number;
		register USIZE t0 __asm__("t0");
		__asm__ volatile(
			"ecall\n"
			"beqz t0, 1f\n"
			"neg a0, a0\n"
			"1:\n"
			: "=r"(a0), "=r"(t0)
			: "r"(a7)
			: "memory"
		);
		return (SSIZE)a0;
	}

	// Syscall with 1 argument
	static inline SSIZE Call(USIZE number, USIZE arg1)
	{
		register USIZE a0 __asm__("a0") = arg1;
		register USIZE a7 __asm__("a7") = number;
		register USIZE t0 __asm__("t0");
		__asm__ volatile(
			"ecall\n"
			"beqz t0, 1f\n"
			"neg a0, a0\n"
			"1:\n"
			: "+r"(a0), "=r"(t0)
			: "r"(a7)
			: "memory"
		);
		return (SSIZE)a0;
	}

	// Syscall with 2 arguments
	static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
	{
		register USIZE a0 __asm__("a0") = arg1;
		register USIZE a1 __asm__("a1") = arg2;
		register USIZE a7 __asm__("a7") = number;
		register USIZE t0 __asm__("t0");
		__asm__ volatile(
			"ecall\n"
			"beqz t0, 1f\n"
			"neg a0, a0\n"
			"1:\n"
			: "+r"(a0), "=r"(t0)
			: "r"(a1), "r"(a7)
			: "memory"
		);
		return (SSIZE)a0;
	}

	// Syscall with 3 arguments
	static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
	{
		register USIZE a0 __asm__("a0") = arg1;
		register USIZE a1 __asm__("a1") = arg2;
		register USIZE a2 __asm__("a2") = arg3;
		register USIZE a7 __asm__("a7") = number;
		register USIZE t0 __asm__("t0");
		__asm__ volatile(
			"ecall\n"
			"beqz t0, 1f\n"
			"neg a0, a0\n"
			"1:\n"
			: "+r"(a0), "=r"(t0)
			: "r"(a1), "r"(a2), "r"(a7)
			: "memory"
		);
		return (SSIZE)a0;
	}

	// Syscall with 4 arguments
	static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
	{
		register USIZE a0 __asm__("a0") = arg1;
		register USIZE a1 __asm__("a1") = arg2;
		register USIZE a2 __asm__("a2") = arg3;
		register USIZE a3 __asm__("a3") = arg4;
		register USIZE a7 __asm__("a7") = number;
		register USIZE t0 __asm__("t0");
		__asm__ volatile(
			"ecall\n"
			"beqz t0, 1f\n"
			"neg a0, a0\n"
			"1:\n"
			: "+r"(a0), "=r"(t0)
			: "r"(a1), "r"(a2), "r"(a3), "r"(a7)
			: "memory"
		);
		return (SSIZE)a0;
	}

	// Syscall with 5 arguments
	static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
	{
		register USIZE a0 __asm__("a0") = arg1;
		register USIZE a1 __asm__("a1") = arg2;
		register USIZE a2 __asm__("a2") = arg3;
		register USIZE a3 __asm__("a3") = arg4;
		register USIZE a4 __asm__("a4") = arg5;
		register USIZE a7 __asm__("a7") = number;
		register USIZE t0 __asm__("t0");
		__asm__ volatile(
			"ecall\n"
			"beqz t0, 1f\n"
			"neg a0, a0\n"
			"1:\n"
			: "+r"(a0), "=r"(t0)
			: "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a7)
			: "memory"
		);
		return (SSIZE)a0;
	}

	// Syscall with 6 arguments
	static inline SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
	{
		register USIZE a0 __asm__("a0") = arg1;
		register USIZE a1 __asm__("a1") = arg2;
		register USIZE a2 __asm__("a2") = arg3;
		register USIZE a3 __asm__("a3") = arg4;
		register USIZE a4 __asm__("a4") = arg5;
		register USIZE a5 __asm__("a5") = arg6;
		register USIZE a7 __asm__("a7") = number;
		register USIZE t0 __asm__("t0");
		__asm__ volatile(
			"ecall\n"
			"beqz t0, 1f\n"
			"neg a0, a0\n"
			"1:\n"
			: "+r"(a0), "=r"(t0)
			: "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a7)
			: "memory"
		);
		return (SSIZE)a0;
	}

};  // class System
