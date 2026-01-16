#pragma once

#include "runtime.h"

class Djb2Tests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		Logger::Info<WCHAR>(L"Running DJB2 Hash Tests..."_embed);

		// Test 1: Basic hash consistency
		if (!TestBasicHashConsistency())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Basic hash consistency"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Basic hash consistency"_embed);
		}

		// Test 2: Case insensitivity
		if (!TestCaseInsensitivity())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Case insensitivity"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Case insensitivity"_embed);
		}

		// Test 3: Empty string
		if (!TestEmptyString())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Empty string"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Empty string"_embed);
		}

		// Test 4: Compile-time hash matches runtime hash
		if (!TestCompileTimeMatchesRuntime())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Compile-time matches runtime"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Compile-time matches runtime"_embed);
		}

		// Test 5: Different strings produce different hashes
		if (!TestDifferentStringsProduceDifferentHashes())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Different strings produce different hashes"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Different strings produce different hashes"_embed);
		}

		// Test 6: Wide character support
		if (!TestWideCharSupport())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Wide character support"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Wide character support"_embed);
		}

		if (allPassed)
		{
			Logger::Info<WCHAR>(L"All DJB2 tests passed!"_embed);
		}
		else
		{
			Logger::Error<WCHAR>(L"Some DJB2 tests failed!"_embed);
		}

		return allPassed;
	}

private:
	static BOOL TestBasicHashConsistency()
	{
		// Same input should always produce same hash
		auto testStr = "hello"_embed;
		USIZE hash1 = Djb2::Hash((const CHAR*)testStr);
		USIZE hash2 = Djb2::Hash((const CHAR*)testStr);
		return hash1 == hash2;
	}

	static BOOL TestCaseInsensitivity()
	{
		auto lower = "hello"_embed;
		auto upper = "HELLO"_embed;
		auto mixed = "HeLLo"_embed;

		// Hash should be case-insensitive
		USIZE hashLower = Djb2::Hash((const CHAR*)lower);
		USIZE hashUpper = Djb2::Hash((const CHAR*)upper);
		USIZE hashMixed = Djb2::Hash((const CHAR*)mixed);

		return (hashLower == hashUpper) && (hashLower == hashMixed);
	}

	static BOOL TestEmptyString()
	{
		auto empty = ""_embed;

		// Empty string should return the seed value
		USIZE hash = Djb2::Hash((const CHAR*)empty);
		// Hash of empty string should be non-zero (it's the seed)
		return hash != 0;
	}

	static BOOL TestCompileTimeMatchesRuntime()
	{
		// Compile-time hash should match runtime hash for same input
		constexpr USIZE compileTimeHash = Djb2::HashCompileTime("test");
		auto runtimeStr = "test"_embed;
		USIZE runtimeHash = Djb2::Hash((const CHAR*)runtimeStr);
		return compileTimeHash == runtimeHash;
	}

	static BOOL TestDifferentStringsProduceDifferentHashes()
	{
		auto str1 = "hello"_embed;
		auto str2 = "world"_embed;
		auto str3 = "test"_embed;

		// Different strings should (almost always) produce different hashes
		USIZE hash1 = Djb2::Hash((const CHAR*)str1);
		USIZE hash2 = Djb2::Hash((const CHAR*)str2);
		USIZE hash3 = Djb2::Hash((const CHAR*)str3);

		return (hash1 != hash2) && (hash2 != hash3) && (hash1 != hash3);
	}

	static BOOL TestWideCharSupport()
	{
		auto wideStr = L"hello"_embed;

		// Wide character strings should hash correctly
		USIZE hash1 = Djb2::Hash((const WCHAR*)wideStr);
		USIZE hash2 = Djb2::Hash((const WCHAR*)wideStr);

		// Consistency check
		if (hash1 != hash2)
			return FALSE;

		auto wideLower = L"hello"_embed;
		auto wideUpper = L"HELLO"_embed;

		// Case insensitivity for wide chars
		USIZE hashLower = Djb2::Hash((const WCHAR*)wideLower);
		USIZE hashUpper = Djb2::Hash((const WCHAR*)wideUpper);

		return hashLower == hashUpper;
	}
};
