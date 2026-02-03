#include "allocator.h"
#include "syscall.h"
#include "system.h"

// Memory allocator using mmap/munmap
// Each allocation is a separate mmap, which is simple but not efficient for
// many small allocations. Suitable for basic needs.

PVOID Allocator::AllocateMemory(USIZE size)
{
    if (size == 0)
        return NULL;

    // Align size to page boundary (4096 bytes)
    // mmap allocates in pages, so we round up
    size = (size + 4095) & ~4095ULL;

    // Use mmap to allocate memory
    // fd = -1 for anonymous mapping (no file backing)
    PVOID addr = NULL;
    INT32 prot = PROT_READ | PROT_WRITE;
    INT32 flags = MAP_PRIVATE | MAP_ANONYMOUS;

#if defined(ARCHITECTURE_I386) || defined(ARCHITECTURE_ARMV7A)
    // 32-bit architectures use mmap2 with page-shifted offset
    USIZE offset = 0; // No offset for anonymous mapping
    SSIZE result = System::Call(SYS_MMAP2, (USIZE)addr, size, prot, flags, -1, offset);
#else
    // 64-bit architectures use mmap
    USIZE offset = 0;
    SSIZE result = System::Call(SYS_MMAP, (USIZE)addr, size, prot, flags, -1, offset);
#endif

    // mmap returns -1 on error (well, actually MAP_FAILED which is (void*)-1)
    if (result == -1 || result < 0)
        return NULL;

    return (PVOID)result;
}

VOID Allocator::ReleaseMemory(PVOID address, USIZE size)
{
    if (address == NULL || size == 0)
        return;

    // Align size to page boundary (must match allocation alignment)
    size = (size + 4095) & ~4095ULL;

    // Use munmap to release memory
    System::Call(SYS_MUNMAP, (USIZE)address, size);
}
