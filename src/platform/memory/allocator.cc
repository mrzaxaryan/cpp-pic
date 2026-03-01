#include "allocator.h"

PVOID operator new(USIZE size)
{
    return Allocator::AllocateMemory(size);
}

PVOID operator new[](USIZE size)
{
    return Allocator::AllocateMemory(size);
}

VOID operator delete(PVOID p) noexcept
{
    Allocator::ReleaseMemory(p, 0);
}

VOID operator delete[](PVOID p) noexcept
{
    Allocator::ReleaseMemory(p, 0);
}

VOID operator delete(PVOID p, USIZE s) noexcept
{
    Allocator::ReleaseMemory(p, s);
}

VOID operator delete[](PVOID p, USIZE s) noexcept
{
    Allocator::ReleaseMemory(p, s);
}