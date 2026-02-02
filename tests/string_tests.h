#pragma once

#include "ral.h"

class StringTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		LOG_INFO("Running String Tests...");

		// Test 1: Length of narrow string
		if (!TestLengthNarrow())
		{
			allPassed = FALSE;
			LOG_ERROR("  FAILED: Narrow string length");
		}
		else
		{
			LOG_INFO("  PASSED: Narrow string length");
		}

		// Test 2: Length of wide string
		if (!TestLengthWide())
		{
			allPassed = FALSE;
			LOG_ERROR("  FAILED: Wide string length");
		}
		else
		{
			LOG_INFO("  PASSED: Wide string length");
		}

		// Test 3: Empty string length
		if (!TestLengthEmpty())
		{
			allPassed = FALSE;
			LOG_ERROR("  FAILED: Empty string length");
		}
		else
		{
			LOG_INFO("  PASSED: Empty string length");
		}

		// Test 4: ToLowerCase for ASCII
		if (!TestToLowerCaseAscii())
		{
			allPassed = FALSE;
			LOG_ERROR("  FAILED: ToLowerCase ASCII");
		}
		else
		{
			LOG_INFO("  PASSED: ToLowerCase ASCII");
		}

		// Test 5: ToLowerCase preserves non-uppercase
		if (!TestToLowerCasePreserves())
		{
			allPassed = FALSE;
			LOG_ERROR("  FAILED: ToLowerCase preserves non-uppercase");
		}
		else
		{
			LOG_INFO("  PASSED: ToLowerCase preserves non-uppercase");
		}

		// Test 6: WideToUtf8 basic ASCII
		if (!TestWideToUtf8BasicAscii())
		{
			allPassed = FALSE;
			LOG_ERROR("  FAILED: WideToUtf8 basic ASCII");
		}
		else
		{
			LOG_INFO("  PASSED: WideToUtf8 basic ASCII");
		}

		// Test 7: WideToUtf8 empty string
		if (!TestWideToUtf8Empty())
		{
			allPassed = FALSE;
			LOG_ERROR("  FAILED: WideToUtf8 empty string");
		}
		else
		{
			LOG_INFO("  PASSED: WideToUtf8 empty string");
		}

		// Test 8: WideToUtf8 null handling
		if (!TestWideToUtf8NullHandling())
		{
			allPassed = FALSE;
			LOG_ERROR("  FAILED: WideToUtf8 null handling");
		}
		else
		{
			LOG_INFO("  PASSED: WideToUtf8 null handling");
		}

		if (allPassed)
		{
			LOG_INFO("All String tests passed!");
		}
		else
		{
			LOG_ERROR("Some String tests failed!");
		}

		return allPassed;
	}

private:
	static BOOL TestLengthNarrow()
	{
		auto str1 = "Hello"_embed;
		auto str2 = "Hello, World!"_embed;
		auto str3 = "A"_embed;

		if (String::Length((const CHAR*)str1) != 5)
			return FALSE;
		if (String::Length((const CHAR*)str2) != 13)
			return FALSE;
		if (String::Length((const CHAR*)str3) != 1)
			return FALSE;

		return TRUE;
	}

	static BOOL TestLengthWide()
	{
		auto str1 = L"Hello"_embed;
		auto str2 = L"Hello, World!"_embed;
		auto str3 = L"A"_embed;

		if (String::Length((const WCHAR*)str1) != 5)
			return FALSE;
		if (String::Length((const WCHAR*)str2) != 13)
			return FALSE;
		if (String::Length((const WCHAR*)str3) != 1)
			return FALSE;

		return TRUE;
	}

	static BOOL TestLengthEmpty()
	{
		auto emptyNarrow = ""_embed;
		auto emptyWide = L""_embed;

		if (String::Length((const CHAR*)emptyNarrow) != 0)
			return FALSE;
		if (String::Length((const WCHAR*)emptyWide) != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestToLowerCaseAscii()
	{
		// Test uppercase letters A-Z
		if (String::ToLowerCase<CHAR>('A') != 'a')
			return FALSE;
		if (String::ToLowerCase<CHAR>('M') != 'm')
			return FALSE;
		if (String::ToLowerCase<CHAR>('Z') != 'z')
			return FALSE;

		// Test wide char version
		if (String::ToLowerCase<WCHAR>(L'A') != L'a')
			return FALSE;
		if (String::ToLowerCase<WCHAR>(L'Z') != L'z')
			return FALSE;

		return TRUE;
	}

	static BOOL TestToLowerCasePreserves()
	{
		// Already lowercase should stay lowercase
		if (String::ToLowerCase<CHAR>('a') != 'a')
			return FALSE;
		if (String::ToLowerCase<CHAR>('z') != 'z')
			return FALSE;

		// Numbers should be unchanged
		if (String::ToLowerCase<CHAR>('0') != '0')
			return FALSE;
		if (String::ToLowerCase<CHAR>('9') != '9')
			return FALSE;

		// Special characters should be unchanged
		if (String::ToLowerCase<CHAR>('!') != '!')
			return FALSE;
		if (String::ToLowerCase<CHAR>('@') != '@')
			return FALSE;
		if (String::ToLowerCase<CHAR>(' ') != ' ')
			return FALSE;

		return TRUE;
	}

	static BOOL TestWideToUtf8BasicAscii()
	{
		auto wide = L"Hello"_embed;
		CHAR utf8[16];

		USIZE len = String::WideToUtf8((const WCHAR*)wide, utf8, sizeof(utf8));

		// Should be 5 characters
		if (len != 5)
			return FALSE;

		// Verify content
		auto expected_hello = "Hello"_embed;
		if (Memory::Compare(utf8, (const CHAR*)expected_hello, 6) != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestWideToUtf8Empty()
	{
		auto wide = L""_embed;
		CHAR utf8[16];

		USIZE len = String::WideToUtf8((const WCHAR*)wide, utf8, sizeof(utf8));

		// Should be 0 characters
		if (len != 0)
			return FALSE;

		// Should be null terminated
		auto expected_empty = ""_embed;
		if (Memory::Compare(utf8, (const CHAR*)expected_empty, 1) != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestWideToUtf8NullHandling()
	{
		CHAR utf8[16];
		auto wide = L"Test"_embed;

		// Null wide string should return 0
		if (String::WideToUtf8(NULL, utf8, sizeof(utf8)) != 0)
			return FALSE;

		// Null utf8 buffer should return 0
		if (String::WideToUtf8((const WCHAR*)wide, NULL, sizeof(utf8)) != 0)
			return FALSE;

		// Zero buffer size should return 0
		if (String::WideToUtf8((const WCHAR*)wide, utf8, 0) != 0)
			return FALSE;

		return TRUE;
	}
};
