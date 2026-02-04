#pragma once

#include "runtime.h"
#include "tests.h"

class MemoryTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		LOG_INFO("Running Memory Tests...");

		RunTest(allPassed, EMBED_FUNC(TestCopyBasic), L"Memory copy basic"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCopyNonOverlapping), L"Memory copy non-overlapping"_embed);
		RunTest(allPassed, EMBED_FUNC(TestZero), L"Memory zero"_embed);
		RunTest(allPassed, EMBED_FUNC(TestSet), L"Memory set"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCompareEqual), L"Memory compare equal"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCompareLessThan), L"Memory compare less than"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCompareGreaterThan), L"Memory compare greater than"_embed);
		RunTest(allPassed, EMBED_FUNC(TestZeroSize), L"Memory zero size operations"_embed);

		if (allPassed)
			LOG_INFO("All Memory tests passed!");
		else
			LOG_ERROR("Some Memory tests failed!");

		return allPassed;
	}

private:
	static BOOL TestCopyBasic()
	{
		auto src = "Hello, World!"_embed;
		CHAR dest[16];

		Memory::Copy(dest, (const CHAR*)src, 14); // Include null terminator

		// Verify each byte was copied correctly
		for (INT32 i = 0; i < 14; i++)
		{
			if (dest[i] != ((const CHAR*)src)[i])
				return FALSE;
		}
		return TRUE;
	}

	static BOOL TestCopyNonOverlapping()
	{
		UINT8 buffer[32];

		// Fill with pattern
		for (INT32 i = 0; i < 32; i++)
		{
			buffer[i] = (UINT8)i;
		}

		// Copy to different location (non-overlapping)
		UINT8 dest[16];
		Memory::Copy(dest, buffer, 16);

		for (INT32 i = 0; i < 16; i++)
		{
			if (dest[i] != (UINT8)i)
				return FALSE;
		}
		return TRUE;
	}

	static BOOL TestZero()
	{
		UINT8 buffer[16];

		// Fill with non-zero values
		for (INT32 i = 0; i < 16; i++)
		{
			buffer[i] = 0xFF;
		}

		// Zero the buffer
		Memory::Zero(buffer, 16);

		// Verify all bytes are zero
		for (INT32 i = 0; i < 16; i++)
		{
			if (buffer[i] != 0)
				return FALSE;
		}
		return TRUE;
	}

	static BOOL TestSet()
	{
		UINT8 buffer[16];

		// Set all bytes to a specific value
		Memory::Set(buffer, 0xAB, 16);

		// Verify all bytes have the expected value
		for (INT32 i = 0; i < 16; i++)
		{
			if (buffer[i] != 0xAB)
				return FALSE;
		}

		// Test setting to different value
		Memory::Set(buffer, 0x42, 8);
		for (INT32 i = 0; i < 8; i++)
		{
			if (buffer[i] != 0x42)
				return FALSE;
		}
		// Verify remaining bytes unchanged
		for (INT32 i = 8; i < 16; i++)
		{
			if (buffer[i] != 0xAB)
				return FALSE;
		}

		return TRUE;
	}

	static BOOL TestCompareEqual()
	{
		auto str1 = "Hello"_embed;
		auto str2 = "Hello"_embed;

		INT32 result = Memory::Compare((const CHAR*)str1, (const CHAR*)str2, 5);
		return result == 0;
	}

	static BOOL TestCompareLessThan()
	{
		auto str1 = "Apple"_embed;
		auto str2 = "Banana"_embed;

		INT32 result = Memory::Compare((const CHAR*)str1, (const CHAR*)str2, 5);
		return result < 0; // 'A' < 'B'
	}

	static BOOL TestCompareGreaterThan()
	{
		auto str1 = "Zebra"_embed;
		auto str2 = "Apple"_embed;

		INT32 result = Memory::Compare((const CHAR*)str1, (const CHAR*)str2, 5);
		return result > 0; // 'Z' > 'A'
	}

	static BOOL TestZeroSize()
	{
		UINT8 src[8];
		src[0] = 1; src[1] = 2; src[2] = 3; src[3] = 4;
		src[4] = 5; src[5] = 6; src[6] = 7; src[7] = 8;

		UINT8 dest[8];
		Memory::Zero(dest, 8);

		// Copy zero bytes should not change dest
		Memory::Copy(dest, src, 0);
		for (INT32 i = 0; i < 8; i++)
		{
			if (dest[i] != 0)
				return FALSE;
		}

		// Compare zero bytes should return 0 (equal)
		INT32 cmp = Memory::Compare(src, dest, 0);
		if (cmp != 0)
			return FALSE;

		return TRUE;
	}
};
