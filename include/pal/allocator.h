/**
 * allocator.h - Platform-Specific Memory Allocation
 *
 * Provides heap allocation/deallocation via platform syscalls.
 * Part of PAL (Platform Abstraction Layer).
 *
 * For memory operations (copy, set, compare), use bal/memory.h instead.
 */

#pragma once

#include "primitives.h"

class Allocator
{
public:
    // Platform-specific allocation (implemented in pal/windows/allocator.windows.cc)
    static PVOID AllocateMemory(USIZE size);
    static VOID ReleaseMemory(PVOID ptr, USIZE size);
};
