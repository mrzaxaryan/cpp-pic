/**
 * pal.cc - UEFI Platform Abstraction Layer Core
 *
 * Provides ExitProcess implementation for UEFI.
 */

#include "pal.h"
#include "efi_context.h"

/**
 * ExitProcess - Shutdown the system
 *
 * Uses EFI Runtime Services ResetSystem() to power off the machine.
 *
 * @param code - Exit code (0 = success, non-zero = error)
 */
NO_RETURN VOID ExitProcess(USIZE code)
{
	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_RUNTIME_SERVICES *rs = ctx->SystemTable->RuntimeServices;

	// Shutdown the system
	rs->ResetSystem(EfiResetShutdown, (EFI_STATUS)code, 0, NULL);

	// Should never reach here
	__builtin_unreachable();
}
