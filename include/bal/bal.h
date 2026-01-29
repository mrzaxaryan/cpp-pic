/**
 * bal.h - Base Abstraction Layer
 *
 * Platform-independent types and utilities.
 * No dependencies on PAL or RAL.
 */

#pragma once

// Primitives and types
#include "primitives.h"
#include "uint64.h"
#include "int64.h"
#include "double.h"
#include "embedded_double.h"
#include "embedded_string.h"
#include "embedded_function_pointer.h"
#include "embedded_array.h"

// Base utilities
#include "string.h"
#include "string_formatter.h"
#include "djb2.h"
#include "memory.h"
#include "math.h"
#include "ip_address.h"
