/**
 * pal.h - Platform Abstraction Layer
 *
 * OS/hardware abstraction.
 * Depends on BAL.
 */

#pragma once

#include "bal.h"

// =============================================================================
// Platform Core
// =============================================================================

// Function to get export address from PEB modules
PVOID ResolveExportAddressFromPebModule(USIZE moduleNameHash, USIZE functionNameHash);

// Cross-platform exit process function
NO_RETURN VOID ExitProcess(USIZE code);

// =============================================================================
// Platform Services
// =============================================================================

// Memory management
#include "memory/allocator.h"

// System utilities (must come before logger.h since it uses DateTime)
#include "system/date_time.h"
#include "algorithms/random.h"

// I/O services
#include "io/console.h"
#include "io/file_system.h"
#include "io/logger.h"

// Network services
#include "network/socket.h"
