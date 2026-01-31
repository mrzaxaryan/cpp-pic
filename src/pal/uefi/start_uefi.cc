/**
 * start_uefi.cc - UEFI Entry Point
 *
 * This file provides the UEFI application entry point (efi_main)
 * which stores the EFI context and runs the test suite.
 */

#include "ral.h"
#include "efi_context.h"
#include "tests.h"

// =============================================================================
// Global EFI Context
// =============================================================================

// Static context storage - acceptable for UEFI as we control the environment
static EFI_CONTEXT g_EfiContext;

/**
 * GetEfiContext - Return pointer to the global EFI context
 *
 * This is called by all PAL functions to access Boot/Runtime services.
 */
EFI_CONTEXT *GetEfiContext()
{
	return &g_EfiContext;
}

// =============================================================================
// UEFI Entry Point
// =============================================================================

/**
 * efi_main - UEFI application entry point
 *
 * Called by the UEFI firmware when the application is loaded.
 *
 * @param ImageHandle - Handle to the loaded image
 * @param SystemTable - Pointer to the EFI System Table
 * @return EFI_SUCCESS on success, error code otherwise
 */
extern "C" EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	// Store context for PAL layer access
	g_EfiContext.ImageHandle = ImageHandle;
	g_EfiContext.SystemTable = SystemTable;

	// Disable watchdog timer (default is 5 minutes)
	SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);

	// Run the test suite
	BOOL allPassed = TRUE;

	Logger::Info<WCHAR>(L"=== CPP-PIC Test Suite (UEFI) ==="_embed);
	Logger::Info<WCHAR>(L""_embed);

	// Run all test suites (Embedded/Primitives -> BAL -> PAL -> RAL)

	// BAL - Embedded Types and Numeric Primitives (bal/types/)
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

	// PAL (Platform Abstraction Layer) - Memory and System
	if (!MemoryTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!RandomTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	// RAL (Runtime Abstraction Layer) - Cryptography
	if (!ShaTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!EccTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	// Skip network tests - UEFI socket is not implemented
	Logger::Info<WCHAR>(L"[SKIP] Socket tests (UEFI networking not implemented)"_embed);
	Logger::Info<WCHAR>(L""_embed);

	Logger::Info<WCHAR>(L"[SKIP] TLS tests (requires socket)"_embed);
	Logger::Info<WCHAR>(L""_embed);

	Logger::Info<WCHAR>(L"[SKIP] DNS tests (requires socket)"_embed);
	Logger::Info<WCHAR>(L""_embed);

	Logger::Info<WCHAR>(L"[SKIP] WebSocket tests (requires socket)"_embed);

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

	// Exit cleanly
	ExitProcess(allPassed ? 0 : 1);

	// Never reached
	return EFI_SUCCESS;
}
