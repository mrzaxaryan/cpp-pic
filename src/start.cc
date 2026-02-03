/**
 * start.cc - CPP-PIC Runtime Entry Point
 *
 * Unified _start() entry point for all platforms.
 */

#include "runtime.h"
#include "language/pil_tests.h"
#include "runtime/pir_tests.h"

/**
 * _start - Entry point for all platforms
 */
#if defined(PLATFORM_UEFI)
ENTRYPOINT EFI_STATUS EFIAPI _start(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
#else
ENTRYPOINT INT32 _start(VOID)
#endif
{
#if defined(PLATFORM_UEFI)
	// Allocate context on stack and store pointer in CPU register (GS/TPIDR_EL0)
	// This eliminates the need for a global variable in .data section
	EFI_CONTEXT efiContext = {};
	efiContext.ImageHandle = ImageHandle;
	efiContext.SystemTable = SystemTable;
	SetEfiContextRegister(&efiContext);
	// Disable watchdog timer (default is 5 minutes)
	SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
#endif

	// Run runtime and unit tests
	BOOL allPassed = RunPIRTests() && RunPILTests();
	ExitProcess(allPassed ? 0 : 1);
}
