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

PVOID GetInstructionAddress(VOID);
PCHAR ReversePatternSearch(PCHAR ip, const CHAR *pattern, UINT32 len);

// Function to get export address from PEB modules
PVOID ResolveExportAddressFromPebModule(USIZE moduleNameHash, USIZE functionNameHash);

#define GetEnvironmentBaseAddress() (USIZE)(GetCurrentPEB()->SubSystemData)
#define SetEnvironmentBaseAddress(v) (GetCurrentPEB()->SubSystemData = (PVOID)(v))

// Environment data structure for PIC-style rebasing (used on Windows i386)
typedef struct _ENVIRONMENT_DATA
{
    PVOID BaseAddress;
    BOOL ShouldRelocate;
} ENVIRONMENT_DATA, *PENVIRONMENT_DATA;

#if defined(PLATFORM_WINDOWS_I386)
#define IMAGE_LINK_BASE ((USIZE)0x401000)
#define GetEnvironmentData() ((PENVIRONMENT_DATA)(GetCurrentPEB()->SubSystemData))

NOINLINE VOID InitializeRuntime(PENVIRONMENT_DATA envData);
PVOID PerformRelocation(PVOID p);

#else
#define PerformRelocation(p) (p)
#define InitializeRuntime(envData) ((VOID)envData)
#endif

// Entry point macro
#define ENTRYPOINT extern "C" __attribute__((noreturn))

// Cross-platform exit process function
NO_RETURN VOID ExitProcess(USIZE code);

// =============================================================================
// Platform Services
// =============================================================================
#include "allocator.h"
#include "console.h"
#include "DateTime.h"
