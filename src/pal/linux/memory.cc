#include "allocator.h"
#include "syscall.h"

// Linux syscall numbers for memory management
#if defined(ARCHITECTURE_X86_64)
constexpr USIZE SYS_MMAP = 9;
constexpr USIZE SYS_MUNMAP = 11;
#elif defined(ARCHITECTURE_I386)
constexpr USIZE SYS_MMAP2 = 192;
constexpr USIZE SYS_MUNMAP = 91;
#elif defined(ARCHITECTURE_AARCH64)
constexpr USIZE SYS_MMAP = 222;
constexpr USIZE SYS_MUNMAP = 215;
#elif defined(ARCHITECTURE_ARMV7A)
constexpr USIZE SYS_MMAP2 = 192;
constexpr USIZE SYS_MUNMAP = 91;
#endif

// mmap flags and protection
constexpr int PROT_READ = 0x1;
constexpr int PROT_WRITE = 0x2;
constexpr int MAP_PRIVATE = 0x2;
constexpr int MAP_ANONYMOUS = 0x20;

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
    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;

#if defined(ARCHITECTURE_I386) || defined(ARCHITECTURE_ARMV7A)
    // 32-bit architectures use mmap2 with page-shifted offset
    USIZE offset = 0; // No offset for anonymous mapping
    SSIZE result = Syscall::syscall6(SYS_MMAP2, (USIZE)addr, size, prot, flags, -1, offset);
#else
    // 64-bit architectures use mmap
    USIZE offset = 0;
    SSIZE result = Syscall::syscall6(SYS_MMAP, (USIZE)addr, size, prot, flags, -1, offset);
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
    Syscall::syscall2(SYS_MUNMAP, (USIZE)address, size);
}
