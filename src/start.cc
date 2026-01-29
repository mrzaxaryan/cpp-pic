/**
 * start.cc - CPP-PIC Runtime Entry Point
 */

#include "ral.h"
#include "tests.h"

ENTRYPOINT INT32 _start(VOID)
{
	ENVIRONMENT_DATA envData{};
	InitializeRuntime(&envData);

	BOOL allPassed = TRUE;

	Logger::Info<WCHAR>(L"=== CPP-PIC Test Suite ==="_embed);
	Logger::Info<WCHAR>(L""_embed);

	// Run all test suites (Embedded/Primitives -> BAL -> PAL -> RAL)

	// BAL - Embedded Types and Numeric Primitives (bal/types/)
	if (!Uint64Tests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!Int64Tests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!DoubleTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!StringTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	// BAL - Data Structures, String Utilities, and Algorithms (bal/core, bal/string, bal/algorithms)
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

	// PAL (Platform Abstraction Layer) - Memory and System (pal/)
	if (!MemoryTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!RandomTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	// RAL (Runtime Abstraction Layer) - Cryptography and Network
	if (!ShaTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!EccTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

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

	ExitProcess(allPassed ? 0 : 1);
}
