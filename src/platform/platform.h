/**
 * platform.h - Platform Abstraction Layer
 *
 * OS/hardware abstraction.
 * Depends on CORE.
 */

#pragma once

#include "core.h"

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
#include "platform/memory/allocator.h"

// System utilities (must come before logger.h since it uses DateTime)
#include "platform/system/date_time.h"
#include "platform/system/random.h"

// I/O services
#include "platform/io/console.h"
#include "platform/io/file_system.h"
#include "platform/io/logger.h"

// Network services
#include "platform/network/socket.h"
#include "platform/io/path.h"

// Process management
#include "platform/system/process.h"
