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

// Environment data structure for PIC-style rebasing (used on Windows i386)
typedef struct _ENVIRONMENT_DATA
{
    INT32 RandomSeed;
} ENVIRONMENT_DATA, *PENVIRONMENT_DATA;

#define GetEnvironmentData() ((PENVIRONMENT_DATA)(GetCurrentPEB()->SubSystemData))

NOINLINE VOID InitializeRuntime(PENVIRONMENT_DATA envData);

// PerformRelocation has been removed - use EMBEDDED_FUNCTION_POINTER instead
// See: include/bal/primitives/embedded_function_pointer.h

// Entry point macro
#define ENTRYPOINT extern "C" __attribute__((noreturn))

// Cross-platform exit process function
NO_RETURN VOID ExitProcess(USIZE code);

// =============================================================================
// Platform Services
// =============================================================================
#include "allocator.h"
#include "console.h"
#include "date_time.h"
#include "file_system.h"
#include "logger.h"
#include "network.h"
#include "random.h"
#include "socket.h"
