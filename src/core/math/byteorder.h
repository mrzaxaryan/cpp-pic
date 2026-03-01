/**
 * @file byteorder.h
 * @brief Byte Order Utilities
 *
 * @details Platform-independent byte order swapping operations using compiler intrinsics.
 * Essential for:
 * - Network protocols (converting between host and network byte order)
 * - Binary file formats
 * - Cross-platform data exchange
 *
 * Internet protocols use big-endian (network) byte order as defined in RFC 1700.
 * These macros convert between host byte order and network byte order.
 *
 * All operations are constant-time and typically compile to single CPU instructions:
 * - x86/x64: BSWAP instruction
 * - ARM: REV instruction
 * - RISC-V: Sequence of shifts and ORs
 *
 * @see RFC 1700 — Assigned Numbers (defines network byte order as big-endian)
 *      https://datatracker.ietf.org/doc/html/rfc1700
 *
 * @ingroup core
 *
 * @defgroup byteorder Byte Order Operations
 * @ingroup core
 * @{
 */

#pragma once

// =============================================================================
// BYTE SWAPPING OPERATIONS
// =============================================================================

/**
 * @def UINT16SwapByteOrder
 * @brief Swaps the byte order of a 16-bit unsigned integer
 * @param value The 16-bit value to swap
 * @return The byte-swapped value
 *
 * @details Converts between little-endian and big-endian format.
 * Example: 0x1234 becomes 0x3412
 *
 * @par Example:
 * @code
 * UINT16 networkPort = UINT16SwapByteOrder(8080);  // Convert to network byte order
 * @endcode
 */
#define UINT16SwapByteOrder(value) __builtin_bswap16(value)

/**
 * @def UINT32SwapByteOrder
 * @brief Swaps the byte order of a 32-bit unsigned integer
 * @param value The 32-bit value to swap
 * @return The byte-swapped value
 *
 * @details Converts between little-endian and big-endian format.
 * Example: 0x12345678 becomes 0x78563412
 *
 * @par Example:
 * @code
 * UINT32 networkAddr = UINT32SwapByteOrder(ipAddress);  // Convert to network byte order
 * @endcode
 */
#define UINT32SwapByteOrder(value) __builtin_bswap32(value)

/**
 * @def UINT64SwapByteOrder
 * @brief Swaps the byte order of a 64-bit unsigned integer
 * @param value The 64-bit value to swap
 * @return The byte-swapped value
 *
 * @details Converts between little-endian and big-endian format.
 * Example: 0x0123456789ABCDEF becomes 0xEFCDAB8967452301
 */
#define UINT64SwapByteOrder(value) __builtin_bswap64(value)

/** @} */ // end of byteorder group
