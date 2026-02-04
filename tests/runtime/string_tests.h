#pragma once

#include "runtime.h"
#include "tests.h"

class StringTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		LOG_INFO("Running String Tests...");

		RunTest(allPassed, EMBED_FUNC(TestLengthNarrow), L"Narrow string length"_embed);
		RunTest(allPassed, EMBED_FUNC(TestLengthWide), L"Wide string length"_embed);
		RunTest(allPassed, EMBED_FUNC(TestLengthEmpty), L"Empty string length"_embed);
		RunTest(allPassed, EMBED_FUNC(TestToLowerCaseAscii), L"ToLowerCase ASCII"_embed);
		RunTest(allPassed, EMBED_FUNC(TestToLowerCasePreserves), L"ToLowerCase preserves non-uppercase"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWideToUtf8BasicAscii), L"WideToUtf8 basic ASCII"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWideToUtf8Empty), L"WideToUtf8 empty string"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWideToUtf8NullHandling), L"WideToUtf8 null handling"_embed);

		if (allPassed)
			LOG_INFO("All String tests passed!");
		else
			LOG_ERROR("Some String tests failed!");

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
