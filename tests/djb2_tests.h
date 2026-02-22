#pragma once

#include "runtime.h"
#include "tests.h"

class Djb2Tests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		LOG_INFO("Running DJB2 Hash Tests...");

		RunTest(allPassed, EMBED_FUNC(TestBasicHashConsistency), L"Basic hash consistency"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCaseInsensitivity), L"Case insensitivity"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEmptyString), L"Empty string"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCompileTimeMatchesRuntime), L"Compile-time matches runtime"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDifferentStringsProduceDifferentHashes), L"Different strings produce different hashes"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWideCharSupport), L"Wide character support"_embed);

		if (allPassed)
			LOG_INFO("All DJB2 tests passed!");
		else
			LOG_ERROR("Some DJB2 tests failed!");

		return allPassed;
	}

private:
	static BOOL TestBasicHashConsistency()
	{
		// Same input should always produce same hash
		auto testStr = "hello"_embed;
		UINT64 hash1 = Djb2::Hash((const CHAR*)testStr);
		UINT64 hash2 = Djb2::Hash((const CHAR*)testStr);
		return hash1 == hash2;
	}

	static BOOL TestCaseInsensitivity()
	{
		auto lower = "hello"_embed;
		auto upper = "HELLO"_embed;
		auto mixed = "HeLLo"_embed;

		// Hash should be case-insensitive
		UINT64 hashLower = Djb2::Hash((const CHAR*)lower);
		UINT64 hashUpper = Djb2::Hash((const CHAR*)upper);
		UINT64 hashMixed = Djb2::Hash((const CHAR*)mixed);

		return (hashLower == hashUpper) && (hashLower == hashMixed);
	}

	static BOOL TestEmptyString()
	{
		auto empty = ""_embed;

		// Empty string should return the seed value
		UINT64 hash = Djb2::Hash((const CHAR*)empty);
		// Hash of empty string should be non-zero (it's the seed)
		return hash != 0;
	}

	static BOOL TestCompileTimeMatchesRuntime()
	{
		// Compile-time hash should match runtime hash for same input
		constexpr UINT64 compileTimeHash = Djb2::HashCompileTime("test");
		auto runtimeStr = "test"_embed;
		UINT64 runtimeHash = Djb2::Hash((const CHAR*)runtimeStr);
		return compileTimeHash == runtimeHash;
	}

	static BOOL TestDifferentStringsProduceDifferentHashes()
	{
		auto str1 = "hello"_embed;
		auto str2 = "world"_embed;
		auto str3 = "test"_embed;

		// Different strings should (almost always) produce different hashes
		UINT64 hash1 = Djb2::Hash((const CHAR*)str1);
		UINT64 hash2 = Djb2::Hash((const CHAR*)str2);
		UINT64 hash3 = Djb2::Hash((const CHAR*)str3);

		return (hash1 != hash2) && (hash2 != hash3) && (hash1 != hash3);
	}

	static BOOL TestWideCharSupport()
	{
		auto wideStr = L"hello"_embed;

		// Wide character strings should hash correctly
		UINT64 hash1 = Djb2::Hash((const WCHAR*)wideStr);
		UINT64 hash2 = Djb2::Hash((const WCHAR*)wideStr);

		// Consistency check
		if (hash1 != hash2)
			return FALSE;

		auto wideLower = L"hello"_embed;
		auto wideUpper = L"HELLO"_embed;

		// Case insensitivity for wide chars
		UINT64 hashLower = Djb2::Hash((const WCHAR*)wideLower);
		UINT64 hashUpper = Djb2::Hash((const WCHAR*)wideUpper);

		return hashLower == hashUpper;
	}
};
