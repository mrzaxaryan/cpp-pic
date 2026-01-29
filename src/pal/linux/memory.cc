#include "allocator.h"
#include "syscall.h"

// Linux syscall numbers for memory management
#if defined(ARCHITECTURE_X86_64)
constexpr USIZE SYS_BRK = 12;
#elif defined(ARCHITECTURE_I386)
constexpr USIZE SYS_BRK = 45;
#elif defined(ARCHITECTURE_AARCH64)
constexpr USIZE SYS_BRK = 214;
#elif defined(ARCHITECTURE_ARMV7A)
constexpr USIZE SYS_BRK = 45;
#endif

// Simple heap allocator using brk syscall
// This is a very basic bump allocator - not suitable for production
// but sufficient for basic needs
namespace {
    PVOID heapStart = NULL;
    PVOID heapEnd = NULL;
    PVOID heapCurrent = NULL;
}

PVOID Allocator::AllocateMemory(USIZE size)
{
    if (size == 0)
        return NULL;

    // Align size to 16 bytes
    size = (size + 15) & ~15ULL;

    // Initialize heap if needed
    if (heapStart == NULL)
    {
        // Get current brk
        SSIZE result = Syscall::syscall1(SYS_BRK, 0);
        if (result < 0)
            return NULL;

        heapStart = (PVOID)result;
        heapEnd = heapStart;
        heapCurrent = heapStart;
    }

    // Check if we have enough space
    USIZE available = (USIZE)heapEnd - (USIZE)heapCurrent;
    if (available < size)
    {
        // Expand heap
        USIZE newEnd = (USIZE)heapEnd + size + 4096; // Add some extra
        SSIZE result = Syscall::syscall1(SYS_BRK, newEnd);
        if (result < 0 || (USIZE)result < newEnd)
        {
            // Try exact size
            newEnd = (USIZE)heapEnd + size;
            result = Syscall::syscall1(SYS_BRK, newEnd);
            if (result < 0 || (USIZE)result < newEnd)
                return NULL;
        }
        heapEnd = (PVOID)result;
    }

    // Allocate from current position
    PVOID allocated = heapCurrent;
    heapCurrent = (PVOID)((USIZE)heapCurrent + size);

    return allocated;
}

VOID Allocator::ReleaseMemory(PVOID address, USIZE size)
{
    // Simple bump allocator doesn't support freeing individual allocations
    // In a real implementation, you'd use a proper heap allocator
    (VOID)address;
    (VOID)size;
}
