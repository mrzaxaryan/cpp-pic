#pragma once

#include "runtime.h"
#include "tests.h"

class ArrayStorageTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		LOG_INFO("Running ArrayStorage Tests...");

		RunTest(allPassed, EMBED_FUNC(TestWideCharArrayStorage), L"Wide char array storage"_embed);
		RunTest(allPassed, EMBED_FUNC(TestUInt32ArrayStorage), L"UINT32 array storage"_embed);
		RunTest(allPassed, EMBED_FUNC(TestUInt64ArrayStorage), L"UINT64 array storage"_embed);
		RunTest(allPassed, EMBED_FUNC(TestArrayIndexing), L"Array indexing"_embed);
		RunTest(allPassed, EMBED_FUNC(TestPointerConversionAndCopy), L"Pointer conversion and copy"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCompileTimeConstants), L"Compile-time constants"_embed);

		if (allPassed)
			LOG_INFO("All ArrayStorage tests passed!");
		else
			LOG_ERROR("Some ArrayStorage tests failed!");

		return allPassed;
	}

private:
	static BOOL TestWideCharArrayStorage()
	{
		// Use string literal directly - no named array
		auto storage = MakeEmbedArray(L"Test");

		// Verify size
		if (storage.Count != 5) // "Test" + null terminator
			return FALSE;

		// Verify data integrity using literals
		if (storage[0] != L'T' || storage[1] != L'e' || storage[2] != L's' ||
		    storage[3] != L't' || storage[4] != L'\0')
			return FALSE;

		return TRUE;
	}

	static BOOL TestUInt32ArrayStorage()
	{
		// Use compound literal directly - no named array
		auto storage = MakeEmbedArray((const UINT32[]){1, 2, 3, 4});

		// Verify size
		if (storage.Count != 4)
			return FALSE;

		// Print values to console
		LOG_INFO("    UINT32 values:");
		for (USIZE i = 0; i < 4; i++)
		{
			LOG_INFO("      %u", storage[i]);
		}

		// Verify data integrity using literals
		if (storage[0] != 1 || storage[1] != 2 || storage[2] != 3 || storage[3] != 4)
			return FALSE;

		return TRUE;
	}

	static BOOL TestUInt64ArrayStorage()
	{
		// Use compound literal directly - no named array
		auto storage = MakeEmbedArray((const UINT64[]){
			UINT64(0x123456789ABCDEF0ULL),
			UINT64(0xFEDCBA9876543210ULL),
			UINT64(0x0011223344556677ULL)});

		// Verify size
		if (storage.Count != 3)
			return FALSE;

		// Verify data integrity using literals
		if (storage[0] != UINT64(0x123456789ABCDEF0ULL) ||
		    storage[1] != UINT64(0xFEDCBA9876543210ULL) ||
		    storage[2] != UINT64(0x0011223344556677ULL))
			return FALSE;

		return TRUE;
	}

	static BOOL TestArrayIndexing()
	{
		// Use compound literal directly - no named array
		auto storage = MakeEmbedArray((const UINT32[]){100, 200, 300, 400, 500});

		// Test indexing operator
		if (storage[0] != 100 || storage[1] != 200 || storage[2] != 300 ||
		    storage[3] != 400 || storage[4] != 500)
			return FALSE;

		return TRUE;
	}

	static BOOL TestPointerConversionAndCopy()
	{
		// Use compound literal directly - no named array
		auto storage = MakeEmbedArray((const UINT32[]){0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC});

		// Test pointer conversion and Memory::Copy
		UINT32 dest[3];
		Memory::Copy(dest, storage, 3 * sizeof(UINT32));

		if (dest[0] != 0xAAAAAAAA || dest[1] != 0xBBBBBBBB || dest[2] != 0xCCCCCCCC)
			return FALSE;

		return TRUE;
	}

	static BOOL TestCompileTimeConstants()
	{
		// Use string literal directly - no named array, no constexpr on storage
		auto storage = MakeEmbedArray("CompileTime");

		// Verify compile-time properties via type traits
		using StorageType = decltype(MakeEmbedArray("CompileTime"));
		static_assert(StorageType::Count == 12, "Count should be 12");
		static_assert(StorageType::SizeBytes == 12, "SizeBytes should be 12");

		// Verify runtime behavior matches compile-time expectations
		if (storage.Count != 12)
			return FALSE;
		if (storage.SizeBytes != 12)
			return FALSE;

		return TRUE;
	}
};
