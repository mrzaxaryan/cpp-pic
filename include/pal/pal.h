/**
 * pal.h - Platform Abstraction Layer
 *
 * OS/hardware abstraction.
 * Depends on BAL.
 */

#pragma once

#include "bal.h"

// =============================================================================
// Platform-Specific Headers
// =============================================================================

#if defined(PLATFORM_UEFI)
// UEFI platform - include EFI types and system table
#include "efi_types.h"
#include "efi_system_table.h"
#include "efi_context.h"
#endif

// =============================================================================
// Platform Core
// =============================================================================

// Cross-platform exit process function
NO_RETURN VOID ExitProcess(USIZE code);

// =============================================================================
// Platform Services
// =============================================================================

// Memory management
#include "memory/allocator.h"

// System utilities (must come before logger.h since it uses DateTime)
#include "system/date_time.h"
#include "system/random.h"

// I/O services
#include "io/console.h"
#include "io/file_system.h"
#include "io/logger.h"

// Network services
#include "network/socket.h"
