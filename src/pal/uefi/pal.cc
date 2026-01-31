/**
 * pal.cc - UEFI Platform Abstraction Layer Core
 *
 * Provides ExitProcess implementation for UEFI.
 */

#include "pal.h"
#include "efi_context.h"

/**
 * ExitProcess - Terminate the UEFI application
 *
 * Uses EFI Boot Services Exit() to cleanly terminate the application.
 *
 * @param code - Exit code (0 = success, non-zero = error)
 */
NO_RETURN VOID ExitProcess(USIZE code)
{
	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	// Call Exit boot service
	// Parameters: ImageHandle, ExitStatus, ExitDataSize, ExitData
	bs->Exit(ctx->ImageHandle, (EFI_STATUS)code, 0, NULL);

	// Should never reach here
	__builtin_unreachable();
}
