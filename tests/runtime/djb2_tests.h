#pragma once

#include "ral.h"

class Djb2Tests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		LOG_INFO("Running DJB2 Hash Tests...");

		// Test 1: Basic hash consistency
		if (!TestBasicHashConsistency())
		{
			allPassed = FALSE;
			LOG_ERROR("  FAILED: Basic hash consistency");
		}
		else
		{
			LOG_INFO("  PASSED: Basic hash consistency");
		}

		// Test 2: Case insensitivity
		if (!TestCaseInsensitivity())
		{
			allPassed = FALSE;
			LOG_ERROR("  FAILED: Case insensitivity");
		}
		else
		{
			LOG_INFO("  PASSED: Case insensitivity");
		}

		// Test 3: Empty string
		if (!TestEmptyString())
		{
			allPassed = FALSE;
			LOG_ERROR("  FAILED: Empty string");
		}
		else
		{
			LOG_INFO("  PASSED: Empty string");
		}

		// Test 4: Compile-time hash matches runtime hash
		if (!TestCompileTimeMatchesRuntime())
		{
			allPassed = FALSE;
			LOG_ERROR("  FAILED: Compile-time matches runtime");
		}
		else
		{
			LOG_INFO("  PASSED: Compile-time matches runtime");
		}

		// Test 5: Different strings produce different hashes
		if (!TestDifferentStringsProduceDifferentHashes())
		{
			allPassed = FALSE;
			LOG_ERROR("  FAILED: Different strings produce different hashes");
		}
		else
		{
			LOG_INFO("  PASSED: Different strings produce different hashes");
		}

		// Test 6: Wide character support
		if (!TestWideCharSupport())
		{
			allPassed = FALSE;
			LOG_ERROR("  FAILED: Wide character support");
		}
		else
		{
			LOG_INFO("  PASSED: Wide character support");
		}

		if (allPassed)
		{
			LOG_INFO("All DJB2 tests passed!");
		}
		else
		{
			LOG_ERROR("Some DJB2 tests failed!");
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
