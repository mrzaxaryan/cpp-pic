#pragma once

#include "runtime.h"

class ArrayStorageTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		Logger::Info<WCHAR>(L"Running ArrayStorage Tests..."_embed);

		// Test 1: Basic char array storage
		if (!TestCharArrayStorage())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Char array storage"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Char array storage"_embed);
		}

		// Test 2: Wide char array storage
		if (!TestWideCharArrayStorage())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Wide char array storage"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Wide char array storage"_embed);
		}

		// Test 3: UINT32 array storage
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
	static BOOL TestCharArrayStorage()
	{
static constexpr UINT64 sha512_k[80] =
    {0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL,
     0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
     0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
     0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
     0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
     0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
     0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL,
     0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
     0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
     0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
     0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL,
     0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
     0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL,
     0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
     0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
     0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
     0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL,
     0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
     0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL,
     0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
     0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
     0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
     0xd192e819d6ef5218ULL, 0xd69906245565a910ULL,
     0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
     0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
     0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
     0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
     0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
     0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL,
     0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
     0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL,
     0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
     0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
     0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
     0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
     0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
     0x28db77f523047d84ULL, 0x32caab7b40c72493ULL,
     0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
     0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
     0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL};

		static constexpr auto storage1 = MakeEmbedArray(sha512_k);

		// Verify size
		if (storage1.Count != 80)
			return FALSE;

		// Verify data integrity
		for (USIZE i = 0; i < 80; i++)
		{
			LOG_WARNING("    sha512_k[%d] = 0x%ull", i, storage1[i]);
		}

		return TRUE;
	}

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
