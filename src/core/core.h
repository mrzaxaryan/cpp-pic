/**
 * @file core.h
 * @brief Core Abstraction Layer - Main include header
 *
 * @details This is the main header file for the Position-Independent Runtime (PIR)
 * Core Abstraction Layer. It provides platform-independent types, utilities, and
 * abstractions that form the foundation of PIR.
 *
 * The Core layer includes:
 * - Compiler-specific definitions and optimization attributes
 * - Memory operations (copy, set, compare, zero)
 * - Mathematical utilities (min, max, abs, clamp)
 * - Byte order manipulation
 * - Bit manipulation operations
 * - Primitive type definitions
 * - Numeric types with PIC guarantees (DOUBLE)
 * - Embedded types for position-independent string/array/function pointer storage
 * - String utilities and formatting
 * - Hash algorithms (DJB2, Base64)
 * - Encoding utilities (UTF-16)
 * - Network types (IP address)
 *
 * @note All components in the Core layer are designed to be position-independent
 * and do not generate .rdata section dependencies.
 *
 * @see compiler/compiler.h For compiler-specific macros
 * @see memory/memory.h For memory operations
 * @see types/primitives.h For base type definitions
 *
 * @defgroup core Core Abstraction Layer
 * @{
 */

#pragma once

/// @name Core Utilities
/// @{
#include "core/compiler/compiler.h"
#include "core/memory/memory.h"
#include "core/math/math.h"
#include "core/math/byteorder.h"
#include "core/math/bitops.h"
/// @}

/// @name Base Types
/// @{
#include "types/primitives.h"
#include "types/span.h"
/// @}
/// @{
#include "core/types/error.h"
/// @}

/// @name Result Type
/// @{
#include "types/result.h"
/// @}

/// @name Numeric Types
/// @{
#include "types/double.h"
/// @}

/// @name Embedded Types
/// @brief Types for position-independent storage (strings, arrays, function pointers)
/// @{
#include "types/embedded/embedded_string.h"
#include "types/embedded/embedded_function_pointer.h"
#include "types/embedded/embedded_array.h"
/// @}

/// @name String Utilities
/// @{
#include "string/string.h"
#include "string/string_formatter.h"
/// @}

/// @name Algorithms
/// @{
#include "algorithms/djb2.h"
#include "algorithms/base64.h"
/// @}

/// @name Encoding Utilities
/// @{
#include "encoding/utf16.h"
/// @}

/// @name Network Types
/// @{
#include "types/ip_address.h"
/// @}

/// @name Binary I/O
/// @{
#include "core/io/binary_reader.h"
#include "core/io/binary_writer.h"
/// @}

/** @} */ // end of core group
