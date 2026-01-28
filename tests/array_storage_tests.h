#pragma once

#include "ral.h"

class ArrayStorageTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		Logger::Info<WCHAR>(L"Running ArrayStorage Tests..."_embed);

		// Test 1: Wide char array storage
		if (!TestWideCharArrayStorage())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Wide char array storage"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Wide char array storage"_embed);
		}

		// Test 2: UINT32 array storage
		if (!TestUInt32ArrayStorage())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: UINT32 array storage"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: UINT32 array storage"_embed);
		}

		// Test 4: UINT64 array storage
		if (!TestUInt64ArrayStorage())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: UINT64 array storage"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: UINT64 array storage"_embed);
		}

		// Test 5: Array indexing operator
		if (!TestArrayIndexing())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Array indexing"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Array indexing"_embed);
		}

		// Test 6: Pointer conversion and memory copy
		if (!TestPointerConversionAndCopy())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Pointer conversion and copy"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Pointer conversion and copy"_embed);
		}

		// Test 7: Compile-time constants
		if (!TestCompileTimeConstants())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Compile-time constants"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Compile-time constants"_embed);
		}

		if (allPassed)
		{
			Logger::Info<WCHAR>(L"All ArrayStorage tests passed!"_embed);
		}
		else
		{
			Logger::Error<WCHAR>(L"Some ArrayStorage tests failed!"_embed);
		}

		return allPassed;
	}

private:
	static BOOL TestWideCharArrayStorage()
	{
		static constexpr const WCHAR testData[] = L"Test";
		static constexpr auto storage = MakeEmbedArray(testData);

		// Verify size
		if (storage.Count != 5) // "Test" + null terminator
			return FALSE;

		// Verify data integrity
		for (USIZE i = 0; i < 5; i++)
		{
			if (storage[i] != testData[i])
				return FALSE;
		}

		return TRUE;
	}

	static BOOL TestUInt32ArrayStorage()
	{
		static constexpr const UINT32 testData[] = {1, 2, 3, 4};
		static constexpr auto storage = MakeEmbedArray(testData);

		// Verify size
		if (storage.Count != 4)
			return FALSE;

		// Print values to console
		Logger::Info<WCHAR>(L"    UINT32 values:"_embed);
		for (USIZE i = 0; i < 4; i++)
		{
			Logger::Info<WCHAR>(L"      %u"_embed, storage[i]);
		}

		// Verify data integrity
		for (USIZE i = 0; i < 4; i++)
		{
			if (storage[i] != testData[i])
				return FALSE;
		}

		return TRUE;
	}

	static BOOL TestUInt64ArrayStorage()
	{
		static constexpr const UINT64 testData[] = {
			0x123456789ABCDEF0ULL,
			0xFEDCBA9876543210ULL,
			0x0011223344556677ULL};
		static constexpr auto storage = MakeEmbedArray(testData);

		// Verify size
		if (storage.Count != 3)
			return FALSE;

		// Verify data integrity
		for (USIZE i = 0; i < 3; i++)
		{
			if (storage[i] != testData[i])
				return FALSE;
		}

		return TRUE;
	}

	static BOOL TestArrayIndexing()
	{
		static constexpr UINT32 testData[] = {100, 200, 300, 400, 500};
		auto storage = MakeEmbedArray(testData);

		// Test indexing operator
		for (USIZE i = 0; i < 5; i++)
		{
			if (storage[i] != testData[i])
				return FALSE;
		}

		return TRUE;
	}

	static BOOL TestPointerConversionAndCopy()
	{
		static constexpr const UINT32 testData[] = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC};
		static constexpr auto storage = MakeEmbedArray(testData);

		// Test pointer conversion and Memory::Copy
		UINT32 dest[3];
		Memory::Copy(dest, storage, sizeof(testData));

		for (USIZE i = 0; i < 3; i++)
		{
			if (dest[i] != testData[i])
				return FALSE;
		}

		return TRUE;
	}

	static BOOL TestCompileTimeConstants()
	{
		static constexpr const CHAR testData[] = "CompileTime";
		static constexpr auto storage = MakeEmbedArray(testData);

		// Verify compile-time properties
		static_assert(storage.Count == 12, "Count should be 12");
		static_assert(storage.SizeBytes == 12, "SizeBytes should be 12");

		// Verify runtime behavior matches compile-time expectations
		if (storage.Count != 12)
			return FALSE;
		if (storage.SizeBytes != 12)
			return FALSE;

		return TRUE;
	}
};