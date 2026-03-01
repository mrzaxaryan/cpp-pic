#include "allocator.h"
#include "syscall.h"
#include "system.h"

// Memory allocator using mmap/munmap (BSD syscalls)
// Same pattern as Linux but macOS uses MAP_ANONYMOUS = 0x1000

PVOID Allocator::AllocateMemory(USIZE size)
{
	if (size == 0)
		return nullptr;

	// Align size to page boundary (4096 bytes on macOS)
	size = (size + 4095) & ~4095ULL;

	PVOID addr = nullptr;
	INT32 prot = PROT_READ | PROT_WRITE;
	INT32 flags = MAP_PRIVATE | MAP_ANONYMOUS;

	SSIZE result = System::Call(SYS_MMAP, (USIZE)addr, size, prot, flags, -1, (USIZE)0);

	if (result == -1 || result < 0)
		return nullptr;

	return (PVOID)result;
}

VOID Allocator::ReleaseMemory(PVOID address, USIZE size)
{
	if (address == nullptr || size == 0)
		return;

	size = (size + 4095) & ~4095ULL;
	System::Call(SYS_MUNMAP, (USIZE)address, size);
}
