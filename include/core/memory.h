/**
 * @file memory.h
 * @brief Platform-Independent Memory Operations
 *
 * @details Provides CRT-free memory manipulation functions (copy, set, compare, zero).
 * These are pure byte-by-byte operations with no platform dependencies, implemented
 * without relying on the C runtime library.
 *
 * The Memory class provides a clean C++ interface to low-level memory operations
 * that are essential for shellcode and position-independent code.
 *
 * @note All functions are implemented in memory.cc without CRT dependencies.
 *
 * @ingroup core
 *
 * @defgroup memory Memory Operations
 * @ingroup core
 * @{
 */

#pragma once

#include "primitives.h"

// =============================================================================
// C LIBRARY MEMORY FUNCTIONS
// =============================================================================

/**
 * @brief Sets a block of memory to a specified value
 * @param dest Pointer to the destination memory block
 * @param ch Value to set (converted to unsigned char)
 * @param count Number of bytes to set
 * @return Pointer to the destination memory block
 * @note Implemented in memory.cc without CRT dependencies
 */
extern "C" PVOID memset(PVOID dest, INT32 ch, USIZE count);

/**
 * @brief Copies a block of memory from source to destination
 * @param dest Pointer to the destination memory block
 * @param src Pointer to the source memory block
 * @param count Number of bytes to copy
 * @return Pointer to the destination memory block
 * @warning Memory regions must not overlap; use memmove for overlapping regions
 * @note Implemented in memory.cc without CRT dependencies
 */
extern "C" PVOID memcpy(PVOID dest, const VOID *src, USIZE count);

/**
 * @brief Compares two blocks of memory
 * @param ptr1 Pointer to the first memory block
 * @param ptr2 Pointer to the second memory block
 * @param num Number of bytes to compare
 * @return 0 if equal, negative if ptr1 < ptr2, positive if ptr1 > ptr2
 * @note Implemented in memory.cc without CRT dependencies
 */
extern "C" INT32 memcmp(const VOID *ptr1, const VOID *ptr2, USIZE num);

// =============================================================================
// MEMORY CLASS
// =============================================================================

/**
 * @class Memory
 * @brief Position-independent memory operations wrapper class
 *
 * @details Provides a clean C++ interface for memory operations without
 * CRT dependencies. All methods are force-inlined for maximum performance.
 *
 * @par Example Usage:
 * @code
 * CHAR buffer[256];
 * Memory::Zero(buffer, sizeof(buffer));           // Zero the buffer
 * Memory::Copy(buffer, "Hello", 5);               // Copy data
 * Memory::Set(buffer + 5, 'X', 10);               // Fill with 'X'
 * INT32 cmp = Memory::Compare(a, b, sizeof(a));   // Compare memory
 * @endcode
 */
class Memory
{
public:
	/**
	 * @brief Copies memory from source to destination
	 * @param dest Destination buffer
	 * @param src Source buffer
	 * @param count Number of bytes to copy
	 * @return Pointer to destination buffer
	 */
	FORCE_INLINE static PVOID Copy(PVOID dest, PCVOID src, USIZE count)
	{
		return memcpy(dest, src, count);
	}

	/**
	 * @brief Zeros a memory region
	 * @param dest Destination buffer to zero
	 * @param count Number of bytes to zero
	 * @return Pointer to destination buffer
	 */
	FORCE_INLINE static PVOID Zero(PVOID dest, USIZE count)
	{
		return memset(dest, 0, count);
	}

	/**
	 * @brief Sets memory to a specified byte value
	 * @param dest Destination buffer
	 * @param ch Byte value to set
	 * @param count Number of bytes to set
	 * @return Pointer to destination buffer
	 */
	FORCE_INLINE static PVOID Set(PVOID dest, INT32 ch, USIZE count)
	{
		return memset(dest, ch, count);
	}

	/**
	 * @brief Compares two memory regions
	 * @param ptr1 First memory region
	 * @param ptr2 Second memory region
	 * @param num Number of bytes to compare
	 * @return 0 if equal, negative if ptr1 < ptr2, positive if ptr1 > ptr2
	 */
	FORCE_INLINE static INT32 Compare(PCVOID ptr1, PCVOID ptr2, USIZE num)
	{
		return memcmp(ptr1, ptr2, num);
	}
};

/** @} */ // end of memory group
