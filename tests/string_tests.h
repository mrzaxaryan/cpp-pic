#pragma once

#include "runtime/runtime.h"
#include "tests.h"

class StringTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running String Tests...");

		RunTest(allPassed, EMBED_FUNC(TestLengthNarrow), "Narrow string length"_embed);
		RunTest(allPassed, EMBED_FUNC(TestLengthWide), "Wide string length"_embed);
		RunTest(allPassed, EMBED_FUNC(TestLengthEmpty), "Empty string length"_embed);
		RunTest(allPassed, EMBED_FUNC(TestToLowerCaseAscii), "ToLowerCase ASCII"_embed);
		RunTest(allPassed, EMBED_FUNC(TestToLowerCasePreserves), "ToLowerCase preserves non-uppercase"_embed);
		RunTest(allPassed, EMBED_FUNC(TestUTF16ToUTF8BasicAscii), "UTF16::ToUTF8 basic ASCII"_embed);
		RunTest(allPassed, EMBED_FUNC(TestUTF16ToUTF8Empty), "UTF16::ToUTF8 empty string"_embed);

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

		if (StringUtils::Length((const CHAR *)str1) != 5)
		{
			LOG_ERROR("Length('Hello') != 5");
			return false;
		}
		if (StringUtils::Length((const CHAR *)str2) != 13)
		{
			LOG_ERROR("Length('Hello, World!') != 13");
			return false;
		}
		if (StringUtils::Length((const CHAR *)str3) != 1)
		{
			LOG_ERROR("Length('A') != 1");
			return false;
		}

		return true;
	}

	static BOOL TestLengthWide()
	{
		auto str1 = L"Hello"_embed;
		auto str2 = L"Hello, World!"_embed;
		auto str3 = L"A"_embed;

		if (StringUtils::Length((const WCHAR *)str1) != 5)
		{
			LOG_ERROR("Wide Length('Hello') != 5");
			return false;
		}
		if (StringUtils::Length((const WCHAR *)str2) != 13)
		{
			LOG_ERROR("Wide Length('Hello, World!') != 13");
			return false;
		}
		if (StringUtils::Length((const WCHAR *)str3) != 1)
		{
			LOG_ERROR("Wide Length('A') != 1");
			return false;
		}

		return true;
	}

	static BOOL TestLengthEmpty()
	{
		auto emptyNarrow = ""_embed;
		auto emptyWide = L""_embed;

		if (StringUtils::Length((const CHAR *)emptyNarrow) != 0)
		{
			LOG_ERROR("Narrow empty string length != 0");
			return false;
		}
		if (StringUtils::Length((const WCHAR *)emptyWide) != 0)
		{
			LOG_ERROR("Wide empty string length != 0");
			return false;
		}

		return true;
	}

	static BOOL TestToLowerCaseAscii()
	{
		// Test uppercase letters A-Z
		if (StringUtils::ToLowerCase<CHAR>('A') != 'a')
		{
			LOG_ERROR("ToLowerCase('A') != 'a'");
			return false;
		}
		if (StringUtils::ToLowerCase<CHAR>('M') != 'm')
		{
			LOG_ERROR("ToLowerCase('M') != 'm'");
			return false;
		}
		if (StringUtils::ToLowerCase<CHAR>('Z') != 'z')
		{
			LOG_ERROR("ToLowerCase('Z') != 'z'");
			return false;
		}

		// Test wide char version
		if (StringUtils::ToLowerCase<WCHAR>(L'A') != L'a')
		{
			LOG_ERROR("Wide ToLowerCase('A') != 'a'");
			return false;
		}
		if (StringUtils::ToLowerCase<WCHAR>(L'Z') != L'z')
		{
			LOG_ERROR("Wide ToLowerCase('Z') != 'z'");
			return false;
		}

		return true;
	}

	static BOOL TestToLowerCasePreserves()
	{
		// Already lowercase should stay lowercase
		if (StringUtils::ToLowerCase<CHAR>('a') != 'a')
		{
			LOG_ERROR("ToLowerCase('a') != 'a'");
			return false;
		}
		if (StringUtils::ToLowerCase<CHAR>('z') != 'z')
		{
			LOG_ERROR("ToLowerCase('z') != 'z'");
			return false;
		}

		// Numbers should be unchanged
		if (StringUtils::ToLowerCase<CHAR>('0') != '0')
		{
			LOG_ERROR("ToLowerCase('0') != '0'");
			return false;
		}
		if (StringUtils::ToLowerCase<CHAR>('9') != '9')
		{
			LOG_ERROR("ToLowerCase('9') != '9'");
			return false;
		}

		// Special characters should be unchanged
		if (StringUtils::ToLowerCase<CHAR>('!') != '!')
		{
			LOG_ERROR("ToLowerCase('!') != '!'");
			return false;
		}
		if (StringUtils::ToLowerCase<CHAR>('@') != '@')
		{
			LOG_ERROR("ToLowerCase('@') != '@'");
			return false;
		}
		if (StringUtils::ToLowerCase<CHAR>(' ') != ' ')
		{
			LOG_ERROR("ToLowerCase(' ') != ' '");
			return false;
		}

		return true;
	}

	static BOOL TestUTF16ToUTF8BasicAscii()
	{
		auto wide = L"Hello"_embed;
		CHAR utf8[16];

		USIZE wideLen = StringUtils::Length((const WCHAR *)wide);
		USIZE len = UTF16::ToUTF8(Span<const WCHAR>((const WCHAR *)wide, wideLen), Span<CHAR>(utf8, sizeof(utf8) - 1));
		utf8[len] = '\0';

		// Should be 5 characters
		if (len != 5)
		{
			LOG_ERROR("UTF16->UTF8 length: expected 5, got %u", (UINT32)len);
			return false;
		}

		// Verify content
		auto expected_hello = "Hello"_embed;
		if (Memory::Compare(utf8, (const CHAR *)expected_hello, 6) != 0)
		{
			LOG_ERROR("UTF16->UTF8 content mismatch");
			return false;
		}

		return true;
	}

	static BOOL TestUTF16ToUTF8Empty()
	{
		auto wide = L""_embed;
		CHAR utf8[16];

		USIZE wideLen = StringUtils::Length((const WCHAR *)wide);
		USIZE len = UTF16::ToUTF8(Span<const WCHAR>((const WCHAR *)wide, wideLen), Span<CHAR>(utf8, sizeof(utf8) - 1));
		utf8[len] = '\0';

		// Should be 0 characters
		if (len != 0)
		{
			LOG_ERROR("UTF16->UTF8 empty length: expected 0, got %u", (UINT32)len);
			return false;
		}

		// Should be null terminated
		auto expected_empty = ""_embed;
		if (Memory::Compare(utf8, (const CHAR *)expected_empty, 1) != 0)
		{
			LOG_ERROR("UTF16->UTF8 empty content mismatch");
			return false;
		}

		return true;
	}
};
