/**
 * allocator.linux.cc - Linux Memory Allocator Implementation
 *
 * Implements the Allocator class for Linux using mmap/munmap syscalls.
 *   - Zero libc dependencies
 *   - Simple page-based allocation
 *   - Header stores allocation size for deallocation
 *
 * Memory layout:
 *   +----------------+------------------+
 *   | ALLOC_HDR      | User Data        |
 *   | (Size field)   | (returned ptr)   |
 *   +----------------+------------------+
 *   ^                ^
 *   mmap result      AllocateMemory returns this
 */

#if defined(PLATFORM_LINUX)

#include "allocator.h"
#include "linux/syscall.h"

/* Allocation header - stores user-requested size */
typedef struct _ALLOC_HDR
{
    USIZE Size;
} ALLOC_HDR;

/* Round up x to next multiple of a (a must be power of 2) */
static inline USIZE __align_up(USIZE x, USIZE a)
{
    return (x + (a - 1)) & ~(a - 1);
}

/**
 * Allocator::AllocateMemory - Allocate memory pages
 *
 * Allocates (header + len) rounded up to page size via mmap.
 */
PVOID Allocator::AllocateMemory(USIZE len)
{
    const USIZE page = 4096;
    const USIZE total = __align_up((USIZE)sizeof(ALLOC_HDR) + len, page);

    PVOID base = Syscall::Mmap(
        NULL,
        total,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0);

    if (base == MAP_FAILED)
        return (PVOID)0;

    ALLOC_HDR *h = (ALLOC_HDR *)base;
    h->Size = len;

    return (PVOID)(h + 1);
}

/**
 * Allocator::ReleaseMemory - Free allocated memory
 * 
 * Retrieves size from header and calls munmap.
 */
VOID Allocator::ReleaseMemory(PVOID ptr, USIZE sizeHint)
{
    if (!ptr)
        return;

    ALLOC_HDR *h = ((ALLOC_HDR *)ptr) - 1;
    const USIZE realSize = h->Size;

#if defined(DEBUG)
    if (sizeHint != 0 && sizeHint != realSize)
        __builtin_trap();
#else
    (VOID)sizeHint;
#endif

    const USIZE page = 4096;
    const USIZE total = __align_up((USIZE)sizeof(ALLOC_HDR) + realSize, page);

    Syscall::Munmap(h, total);
}

#endif /* PLATFORM_LINUX */
