/**
 * memory.h - Platform-Independent Memory Operations
 *
 * Provides CRT-free memory manipulation functions (copy, set, compare, zero).
 * These are pure byte-by-byte operations with no platform dependencies.
 *
 * Part of CORE (Core Abstraction Layer) - platform-independent.
 */

#pragma once

#include "primitives.h"

// C library memory functions implemented in memory.cc
extern "C" PVOID memset(PVOID dest, INT32 ch, USIZE count);
extern "C" PVOID memcpy(PVOID dest, const VOID *src, USIZE count);
extern "C" INT32 memcmp(const VOID *ptr1, const VOID *ptr2, USIZE num);

/**
 * Memory - Position-independent memory operations
 *
 * USAGE:
 *   Memory::Copy(dest, src, size);   // memcpy equivalent
 *   Memory::Zero(buffer, size);      // memset(buffer, 0, size)
 *   Memory::Set(buffer, 'A', size);  // memset(buffer, 'A', size)
 *   Memory::Compare(a, b, size);     // memcmp equivalent
 */
class Memory
{
public:
	FORCE_INLINE static PVOID Copy(PVOID dest, PCVOID src, USIZE count)
	{
		return memcpy(dest, src, count);
	}

	FORCE_INLINE static PVOID Zero(PVOID dest, USIZE count)
	{
		return memset(dest, 0, count);
	}

	FORCE_INLINE static PVOID Set(PVOID dest, INT32 ch, USIZE count)
	{
		return memset(dest, ch, count);
	}

	FORCE_INLINE static INT32 Compare(PCVOID ptr1, PCVOID ptr2, USIZE num)
	{
		return memcmp(ptr1, ptr2, num);
	}
};
