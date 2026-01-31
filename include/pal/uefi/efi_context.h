/**
 * efi_context.h - EFI Runtime Context
 *
 * Provides storage and access to EFI runtime context.
 * The ImageHandle and SystemTable are stored here after efi_main
 * receives them and can be accessed by all PAL functions.
 */

#pragma once

#include "efi_system_table.h"

// =============================================================================
// EFI Context Structure
// =============================================================================

struct EFI_CONTEXT
{
	EFI_HANDLE ImageHandle;
	EFI_SYSTEM_TABLE *SystemTable;
};

// =============================================================================
// Context Accessor
// =============================================================================

/**
 * GetEfiContext - Get the global EFI context
 *
 * Returns pointer to the EFI context containing ImageHandle
 * and SystemTable. This is set by efi_main at startup.
 *
 * Note: For UEFI, we use a static global because:
 * 1. UEFI applications control their entire address space
 * 2. The .data section is allowed in UEFI PE files
 * 3. There is only ever one instance running
 * 4. Passing context through all call chains would require
 *    changing all PAL function signatures
 *
 * @return Pointer to the EFI context
 */
EFI_CONTEXT *GetEfiContext();
