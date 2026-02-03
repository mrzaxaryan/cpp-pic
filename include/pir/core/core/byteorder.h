/**
 * byteorder.h - Byte Order Utilities
 *
 * Platform-independent byte order swapping operations.
 * Essential for network protocols, binary file formats, and cross-platform data exchange.
 *
 * Part of CORE (Core Abstraction Layer) - platform-independent.
 */

#pragma once

// =============================================================================
// BYTE SWAPPING OPERATIONS
// =============================================================================

/**
 * Byte order swapping macros using compiler intrinsics
 *
 * These operations are constant-time and typically compile to single CPU instructions:
 *   - x86/x64: BSWAP instruction
 *   - ARM: REV instruction
 *   - RISC-V: Sequence of shifts and ORs
 */

#define UINT16SwapByteOrder(value) __builtin_bswap16(value)
#define UINT32SwapByteOrder(value) __builtin_bswap32(value)
#define UINT64SwapByteOrder(value) __builtin_bswap64(value)
