#pragma once

#include "primitives.h"

#define UINT16SwapByteOrder(value) __builtin_bswap16(value)
#define UINT32SwapByteOrder(value) __builtin_bswap32(value)
#define UINT64SwapByteOrder(value) __builtin_bswap64(value)