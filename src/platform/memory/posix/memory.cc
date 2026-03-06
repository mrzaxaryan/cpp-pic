/**
 * @file memory.cc
 * @brief POSIX memory allocation implementation.
 * @details Provides Allocator::AllocateMemory and ReleaseMemory using
 * mmap/munmap syscalls. A USIZE header is prepended to every allocation
 * to store the total mapped size for munmap.
 */

#include "platform/memory/allocator.h"
#if defined(PLATFORM_LINUX)
#include "platform/common/linux/syscall.h"
#include "platform/common/linux/system.h"
#elif defined(PLATFORM_MACOS)
#include "platform/common/macos/syscall.h"
#include "platform/common/macos/system.h"
#elif defined(PLATFORM_SOLARIS)
#include "platform/common/solaris/syscall.h"
#include "platform/common/solaris/system.h"
#elif defined(PLATFORM_FREEBSD)
#include "platform/common/freebsd/syscall.h"
#include "platform/common/freebsd/system.h"
#elif defined(PLATFORM_OPENBSD)
#include "platform/common/openbsd/syscall.h"
#include "platform/common/openbsd/system.h"
#endif

// Memory allocator using mmap/munmap
// Each allocation is a separate mmap, which is simple but not efficient for
// many small allocations. Suitable for basic needs.
//
// A USIZE header is prepended to every allocation to store the total mmap'd
// size. This allows ReleaseMemory to call munmap without requiring the caller
// to supply the size (unsized operator delete passes size = 0).

PVOID Allocator::AllocateMemory(USIZE size)
{
	if (size == 0)
		return nullptr;

	// Total allocation = header + requested size, aligned to page boundary
	USIZE totalSize = (size + sizeof(USIZE) + 4095) & ~(USIZE)4095;

	// Use mmap to allocate memory
	// fd = -1 for anonymous mapping (no file backing)
	PVOID addr = nullptr;
	INT32 prot = PROT_READ | PROT_WRITE;
	INT32 flags = MAP_PRIVATE | MAP_ANONYMOUS;

#if defined(PLATFORM_LINUX) && (defined(ARCHITECTURE_I386) || defined(ARCHITECTURE_ARMV7A) || defined(ARCHITECTURE_RISCV32))
	// 32-bit Linux architectures use mmap2 with page-shifted offset
	USIZE offset = 0;
	SSIZE result = System::Call(SYS_MMAP2, (USIZE)addr, totalSize, prot, flags, -1, offset);
#elif (defined(PLATFORM_FREEBSD) || defined(PLATFORM_OPENBSD)) && defined(ARCHITECTURE_I386)
	// FreeBSD/OpenBSD i386: SYS_MMAP takes off_t pos (64-bit), which occupies
	// two 32-bit stack slots. The 6-arg System::Call only pushes one slot for
	// the offset, leaving garbage in pos_hi. FreeBSD rejects MAP_ANONYMOUS
	// with non-zero offset (EINVAL), causing allocation failure and segfault.
	// Use direct assembly to push all 7 argument slots + dummy = 32 bytes.
	SSIZE result;
	register USIZE r1 __asm__("ebx") = (USIZE)addr;
	register USIZE r2 __asm__("ecx") = totalSize;
	register USIZE r3 __asm__("edx") = (USIZE)prot;
	register USIZE r4 __asm__("esi") = (USIZE)flags;
	register USIZE r5 __asm__("edi") = (USIZE)(SSIZE)-1; // fd
	__asm__ volatile(
		"pushl $0\n"          // off_t pos high 32 bits = 0
		"pushl $0\n"          // off_t pos low 32 bits = 0
		"pushl %%edi\n"       // fd = -1
		"pushl %%esi\n"       // flags
		"pushl %%edx\n"       // prot
		"pushl %%ecx\n"       // len
		"pushl %%ebx\n"       // addr
		"pushl $0\n"          // dummy return address
		"int $0x80\n"
		"jnc 1f\n"
		"negl %%eax\n"
		"1:\n"
		"addl $32, %%esp\n"
		: "=a"(result)
		: "a"((USIZE)SYS_MMAP), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5)
		: "memory", "cc"
	);
#elif defined(PLATFORM_OPENBSD) && defined(ARCHITECTURE_X86_64)
	// OpenBSD sys_mmap(addr, len, prot, flags, fd, pad, pos) has 7 parameters.
	// x86_64 syscall ABI passes 6 in registers (rdi, rsi, rdx, r10, r8, r9);
	// the 7th (pos) must be on the user stack before the syscall instruction.
	SSIZE result;
	register USIZE r_rdi __asm__("rdi") = (USIZE)addr;
	register USIZE r_rsi __asm__("rsi") = totalSize;
	register USIZE r_rdx __asm__("rdx") = (USIZE)prot;
	register USIZE r_r10 __asm__("r10") = (USIZE)flags;
	register USIZE r_r8  __asm__("r8")  = (USIZE)(SSIZE)-1; // fd
	register USIZE r_r9  __asm__("r9")  = 0;                 // pad
	register USIZE r_rax __asm__("rax") = SYS_MMAP;
	__asm__ volatile(
		"pushq $0\n"          // pos = 0 (7th arg on stack)
		"syscall\n"
		"addq $8, %%rsp\n"    // clean up stack
		"jnc 1f\n"
		"negq %%rax\n"
		"1:\n"
		: "+r"(r_rax), "+r"(r_rdx)
		: "r"(r_rdi), "r"(r_rsi), "r"(r_r10), "r"(r_r8), "r"(r_r9)
		: "rcx", "r11", "memory", "cc"
	);
	result = (SSIZE)r_rax;
#elif defined(PLATFORM_OPENBSD) && defined(ARCHITECTURE_AARCH64)
	// OpenBSD sys_mmap has 7 parameters. AArch64 kernel reads x0-x7 from the
	// trapframe, so all 7 args fit in registers (x0-x6) with x8 as the syscall
	// number. No stack arguments needed.
	SSIZE result;
	register USIZE x0 __asm__("x0") = (USIZE)addr;
	register USIZE x1 __asm__("x1") = totalSize;
	register USIZE x2 __asm__("x2") = (USIZE)prot;
	register USIZE x3 __asm__("x3") = (USIZE)flags;
	register USIZE x4 __asm__("x4") = (USIZE)(SSIZE)-1; // fd
	register USIZE x5 __asm__("x5") = 0;                 // pad
	register USIZE x6 __asm__("x6") = 0;                 // pos
	register USIZE x8 __asm__("x8") = SYS_MMAP;
	__asm__ volatile(
		"svc #0\n"
		"b.cc 1f\n"
		"neg x0, x0\n"
		"1:\n"
		: "+r"(x0), "+r"(x1)
		: "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x6), "r"(x8)
		: "memory", "cc"
	);
	result = (SSIZE)x0;
#elif defined(PLATFORM_OPENBSD) && defined(ARCHITECTURE_RISCV64)
	// OpenBSD sys_mmap has 7 parameters. RISC-V 64 passes 6 in a0-a5; the 7th
	// (pos) goes on the user stack. Maintain 16-byte stack alignment.
	SSIZE result;
	register USIZE a0 __asm__("a0") = (USIZE)addr;
	register USIZE a1 __asm__("a1") = totalSize;
	register USIZE a2 __asm__("a2") = (USIZE)prot;
	register USIZE a3 __asm__("a3") = (USIZE)flags;
	register USIZE a4 __asm__("a4") = (USIZE)(SSIZE)-1; // fd
	register USIZE a5 __asm__("a5") = 0;                 // pad
	__asm__ volatile(
		"addi sp, sp, -16\n"  // 16-byte aligned
		"sd zero, 0(sp)\n"    // pos = 0
		"mv t0, %6\n"
		"ecall\n"
		"addi sp, sp, 16\n"   // clean up stack
		"beqz t0, 1f\n"
		"neg a0, a0\n"
		"1:\n"
		: "+&r"(a0), "+&r"(a1), "+&r"(a2), "+&r"(a3), "+&r"(a4), "+&r"(a5)
		: "r"((USIZE)SYS_MMAP)
		: "t0", "memory"
	);
	result = (SSIZE)a0;
#else
	USIZE offset = 0;
	SSIZE result = System::Call(SYS_MMAP, (USIZE)addr, totalSize, prot, flags, -1, offset);
#endif

	// mmap returns -1 on error (well, actually MAP_FAILED which is (void*)-1)
	if (result < 0 && result >= -4095)
		return nullptr;

	// Store total mmap'd size in the header
	PCHAR base = (PCHAR)result;
	*(USIZE*)base = totalSize;

	// Return pointer past the header
	return (PVOID)(base + sizeof(USIZE));
}

VOID Allocator::ReleaseMemory(PVOID address, USIZE)
{
	if (address == nullptr)
		return;

	// Recover base pointer and total size from the prepended header
	PCHAR base = (PCHAR)address - sizeof(USIZE);
	USIZE totalSize = *(USIZE*)base;

	// Use munmap to release the entire mapping
	System::Call(SYS_MUNMAP, (USIZE)base, totalSize);
}
