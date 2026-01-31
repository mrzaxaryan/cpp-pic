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

// Static context storage for UEFI
static EFI_CONTEXT g_EfiContext;

/**
 * GetEfiContext - Return pointer to the global EFI context
 */
EFI_CONTEXT *GetEfiContext()
{
	return &g_EfiContext;
}
#endif

// =============================================================================
// Common Test Runner
// =============================================================================

static BOOL RunAllTests()
{
	BOOL allPassed = TRUE;

#if defined(PLATFORM_UEFI)
	Logger::Info<WCHAR>(L"=== CPP-PIC Test Suite (UEFI) ==="_embed);
#else
	Logger::Info<WCHAR>(L"=== CPP-PIC Test Suite ==="_embed);
#endif
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

#if defined(PLATFORM_UEFI)
	// Skip network tests - UEFI socket is not implemented
	Logger::Info<WCHAR>(L"[SKIP] Socket tests (UEFI networking not implemented)"_embed);
	Logger::Info<WCHAR>(L""_embed);
	Logger::Info<WCHAR>(L"[SKIP] TLS tests (requires socket)"_embed);
	Logger::Info<WCHAR>(L""_embed);
	Logger::Info<WCHAR>(L"[SKIP] DNS tests (requires socket)"_embed);
	Logger::Info<WCHAR>(L""_embed);
	Logger::Info<WCHAR>(L"[SKIP] WebSocket tests (requires socket)"_embed);
#else
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
#endif

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
	// Store context for PAL layer access
	g_EfiContext.ImageHandle = ImageHandle;
	g_EfiContext.SystemTable = SystemTable;

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
