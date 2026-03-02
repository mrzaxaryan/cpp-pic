/**
 * @file platform.h
 * @brief Platform Abstraction Layer aggregate header
 *
 * @details Includes all platform-layer headers for OS and hardware abstraction.
 * This is the single entry point for consumers of the PLATFORM layer, pulling in
 * memory allocation, system utilities, I/O services, networking, and process
 * management. Platform-specific headers (UEFI, Windows, Linux, macOS, Solaris)
 * are conditionally included based on the target platform. Depends on CORE.
 */

#pragma once

#include "core/core.h"

// =============================================================================
// Platform-Specific Headers
// =============================================================================

#if defined(PLATFORM_UEFI)
// UEFI platform - include EFI types and system table
#include "platform/common/uefi/efi_types.h"
#include "platform/common/uefi/efi_system_table.h"
#include "platform/common/uefi/efi_context.h"
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
#include "platform/fs/offset_origin.h"
#include "platform/fs/directory_entry.h"
#include "platform/fs/file.h"
#include "platform/fs/directory.h"
#include "platform/fs/directory_iterator.h"
#include "platform/fs/path.h"
#include "platform/io/logger.h"

// Network services
#include "platform/network/socket.h"

// Process management
#include "platform/system/process.h"
