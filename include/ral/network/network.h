#pragma once

#include "primitives.h"

#define INVALID_IPV4 0xFFFFFFFF /* 255.255.255.255 */

#define UINT16SwapByteOrder(value) __builtin_bswap16(value)
#define UINT32SwapByteOrder(value) __builtin_bswap32(value)
#define UINT64SwapByteOrder(value) __builtin_bswap64(value)

typedef UINT32 IPv4;

UINT32 ConvertIP(PCCHAR pIP);