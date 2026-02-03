/**
 * core.h - Core Abstraction Layer
 *
 * Platform-independent types and utilities.
 */

#pragma once

// Core utilities
#include "core/compiler.h"
#include "core/memory.h"
#include "core/math.h"
#include "core/byteorder.h"

// Base types
#include "types/primitives.h"

// Numeric types
#include "types/numeric/double.h"

// Embedded types (EMBEDDED_DOUBLE is now in double.h)
#include "types/embedded/embedded_string.h"
#include "types/embedded/embedded_function_pointer.h"
#include "types/embedded/embedded_array.h"

// String utilities
#include "string/string.h"
#include "string/string_formatter.h"

// Algorithms
#include "algorithms/djb2.h"
#include "algorithms/base64.h"

// Network types
#include "types/network/ip_address.h"
