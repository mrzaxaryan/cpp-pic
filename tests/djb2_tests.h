#pragma once

#include "runtime/runtime.h"
#include "tests.h"

class Djb2Tests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running DJB2 Hash Tests...");

		RunTest(allPassed, &TestBasicHashConsistency, "Basic hash consistency");
		RunTest(allPassed, &TestCaseInsensitivity, "Case insensitivity");
		RunTest(allPassed, &TestEmptyString, "Empty string");
		RunTest(allPassed, &TestCompileTimeMatchesRuntime, "Compile-time matches runtime");
		RunTest(allPassed, &TestDifferentStringsProduceDifferentHashes, "Different strings produce different hashes");
		RunTest(allPassed, &TestWideCharSupport, "Wide character support");

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
		auto testStr = "hello";
		UINT64 hash1 = Djb2::Hash((const CHAR *)testStr);
		UINT64 hash2 = Djb2::Hash((const CHAR *)testStr);
		if (hash1 != hash2)
		{
			LOG_ERROR("Hash consistency failed: same input produced different hashes");
			return false;
		}
		return true;
	}

	static BOOL TestCaseInsensitivity()
	{
		auto lower = "hello";
		auto upper = "HELLO";
		auto mixed = "HeLLo";

		// Hash should be case-insensitive
		UINT64 hashLower = Djb2::Hash((const CHAR *)lower);
		UINT64 hashUpper = Djb2::Hash((const CHAR *)upper);
		UINT64 hashMixed = Djb2::Hash((const CHAR *)mixed);

		if (hashLower != hashUpper)
		{
			LOG_ERROR("Case insensitivity failed: lower != upper");
			return false;
		}
		if (hashLower != hashMixed)
		{
			LOG_ERROR("Case insensitivity failed: lower != mixed");
			return false;
		}
		return true;
	}

	static BOOL TestEmptyString()
	{
		auto empty = "";

		// Empty string should return the seed value
		UINT64 hash = Djb2::Hash((const CHAR *)empty);
		// Hash of empty string should be non-zero (it's the seed)
		if (hash == 0)
		{
			LOG_ERROR("Empty string hash is zero, expected non-zero seed");
			return false;
		}
		return true;
	}

	static BOOL TestCompileTimeMatchesRuntime()
	{
		// Compile-time hash should match runtime hash for same input
		constexpr UINT64 compileTimeHash = Djb2::HashCompileTime("test");
		auto runtimeStr = "test";
		UINT64 runtimeHash = Djb2::Hash((const CHAR *)runtimeStr);
		if (compileTimeHash != runtimeHash)
		{
			LOG_ERROR("Compile-time hash does not match runtime hash");
			return false;
		}
		return true;
	}

	static BOOL TestDifferentStringsProduceDifferentHashes()
	{
		auto str1 = "hello";
		auto str2 = "world";
		auto str3 = "test";

		// Different strings should (almost always) produce different hashes
		UINT64 hash1 = Djb2::Hash((const CHAR *)str1);
		UINT64 hash2 = Djb2::Hash((const CHAR *)str2);
		UINT64 hash3 = Djb2::Hash((const CHAR *)str3);

		if (hash1 == hash2)
		{
			LOG_ERROR("Hash collision: 'hello' == 'world'");
			return false;
		}
		if (hash2 == hash3)
		{
			LOG_ERROR("Hash collision: 'world' == 'test'");
			return false;
		}
		if (hash1 == hash3)
		{
			LOG_ERROR("Hash collision: 'hello' == 'test'");
			return false;
		}
		return true;
	}

	static BOOL TestWideCharSupport()
	{
		auto wideStr = L"hello";

		// Wide character strings should hash correctly
		UINT64 hash1 = Djb2::Hash((const WCHAR *)wideStr);
		UINT64 hash2 = Djb2::Hash((const WCHAR *)wideStr);

		// Consistency check
		if (hash1 != hash2)
		{
			LOG_ERROR("Wide char hash consistency failed");
			return false;
		}

		auto wideLower = L"hello";
		auto wideUpper = L"HELLO";

		// Case insensitivity for wide chars
		UINT64 hashLower = Djb2::Hash((const WCHAR *)wideLower);
		UINT64 hashUpper = Djb2::Hash((const WCHAR *)wideUpper);

		if (hashLower != hashUpper)
		{
			LOG_ERROR("Wide char case insensitivity failed: lower != upper");
			return false;
		}
		return true;
	}
};
