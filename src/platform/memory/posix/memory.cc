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
