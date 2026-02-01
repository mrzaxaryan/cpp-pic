/**
 * start.cc - CPP-PIC Runtime Entry Point
 *
 * Unified entry point for all platforms:
 * - Windows/Linux: _start()
 * - UEFI: efi_main()
 */

#include "ral.h"
#include "tests.h"

// =============================================================================
// UEFI-Specific Setup
// =============================================================================

#if defined(PLATFORM_UEFI)
#include "efi_context.h"
#endif

// =============================================================================
// Common Test Runner
// =============================================================================

static BOOL RunAllTests()
{
	BOOL allPassed = TRUE;

	Logger::Info<WCHAR>(L"=== CPP-PIC Test Suite ==="_embed);

	Logger::Info<WCHAR>(L""_embed);

	// BAL - Embedded Types and Numeric Primitives
	if (!DoubleTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!StringTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	// BAL - Data Structures, String Utilities, and Algorithms
	if (!ArrayStorageTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!StringFormatterTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!Djb2Tests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!Base64Tests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	// PAL - Memory and System
	if (!MemoryTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!RandomTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	// RAL - Cryptography
	if (!ShaTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!EccTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	// RAL - Network
	if (!SocketTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!TlsTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!DnsTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!WebSocketTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	// Final summary
	Logger::Info<WCHAR>(L"=== Test Suite Complete ==="_embed);
	if (allPassed)
	{
		Logger::Info<WCHAR>(L"ALL TESTS PASSED!"_embed);
	}
	else
	{
		Logger::Error<WCHAR>(L"SOME TESTS FAILED!"_embed);
	}

	return allPassed;
}

// =============================================================================
// Platform Entry Points
// =============================================================================

#if defined(PLATFORM_UEFI)

/**
 * efi_main - UEFI application entry point
 */
extern "C" EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	// Allocate context on stack and store pointer in CPU register (GS/TPIDR_EL0)
	// This eliminates the need for a global variable in .data section
	EFI_CONTEXT efiContext = {};
	efiContext.ImageHandle = ImageHandle;
	efiContext.SystemTable = SystemTable;
	SetEfiContextRegister(&efiContext);

	// Disable watchdog timer (default is 5 minutes)
	SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);

	// Run tests and exit
	BOOL allPassed = RunAllTests();
	ExitProcess(allPassed ? 0 : 1);

	return EFI_SUCCESS;
}

#else

/**
 * _start - Windows/Linux entry point
 */
ENTRYPOINT INT32 _start(VOID)
{
	BOOL allPassed = RunAllTests();
	ExitProcess(allPassed ? 0 : 1);
}

#endif
