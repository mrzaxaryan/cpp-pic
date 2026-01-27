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

	// Run all test suites
	if (!Djb2Tests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!MemoryTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!StringTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!Uint64Tests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!Int64Tests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!DoubleTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);
	if (!ArrayStorageTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);
	if (!StringFormatterTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!RandomTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!ShaTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	if (!EccTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	// Run socket tests (requires network connectivity)
	if (!SocketTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);
	// Run TLS tests (requires network connectivity)
	if (!TlsTests::RunAll())
		allPassed = FALSE;
	Logger::Info<WCHAR>(L""_embed);

	// Run DNS tests (requires network connectivity)
	if (!DnsTests::RunAll())
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
