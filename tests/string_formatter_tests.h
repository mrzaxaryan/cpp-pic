#pragma once

#include "ral.h"

class StringFormatterTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		Logger::Info<WCHAR>(L"Running StringFormatter Tests..."_embed);

		// Test 1: Integer formatting
		if (!TestIntegerFormat())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Integer format"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Integer format"_embed);
		}

		// Test 2: Unsigned integer formatting
		if (!TestUnsignedFormat())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Unsigned format"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Unsigned format"_embed);
		}

		// Test 3: Hexadecimal formatting
		if (!TestHexFormat())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Hex format"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Hex format"_embed);
		}

		// Test 4: String formatting
		if (!TestStringFormat())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: String format"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: String format"_embed);
		}

		// Test 5: Character formatting
		if (!TestCharFormat())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Char format"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Char format"_embed);
		}

		// Test 6: Width and padding
		if (!TestWidthPadding())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Width and padding"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Width and padding"_embed);
		}

		// Test 7: Float formatting
		if (!TestFloatFormat())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Float format"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Float format"_embed);
		}

		// Test 8: Percent literal
		if (!TestPercentLiteral())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Percent literal"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Percent literal"_embed);
		}

		// // Test 9: USIZE and SSIZE formatting
		// if (!TestSizeFormat())
		// {
		// 	allPassed = FALSE;
		// 	Logger::Error<WCHAR>(L"  FAILED: Size format"_embed);
		// }
		// else
		// {
		// 	Logger::Info<WCHAR>(L"  PASSED: Size format"_embed);
		// }

		if (allPassed)
		{
			Logger::Info<WCHAR>(L"All StringFormatter tests passed!"_embed);
		}
		else
		{
			Logger::Error<WCHAR>(L"Some StringFormatter tests failed!"_embed);
		}

		return allPassed;
	}

private:
	// Helper structure to collect formatted output
	struct BufferContext
	{
		CHAR *buffer;
		INT32 index;
		INT32 maxSize;
	};

	static BOOL CharWriter(PVOID ctx, CHAR ch)
	{
		BufferContext *bc = (BufferContext *)ctx;
		if (bc->index < bc->maxSize - 1)
		{
			bc->buffer[bc->index++] = ch;
			bc->buffer[bc->index] = '\0';
			return TRUE;
		}
		return FALSE;
	}

	static BOOL TestIntegerFormat()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = EMBED_FUNC(CharWriter);
		auto fmt_d = "%d"_embed;
		// Positive integer
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_d, 42);
		auto expected_42 = "42"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_42, 3) != 0)
			return FALSE;

		// Negative integer
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_d, -123);
		auto expected_neg123 = "-123"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_neg123, 4) != 0)
			return FALSE;

		// Zero
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_d, 0);
		auto expected_0 = "0"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_0, 2) != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestUnsignedFormat()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = EMBED_FUNC(CharWriter);
		auto fmt_u = "%u"_embed;

		// Simple unsigned
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_u, (UINT32)12345);
		auto expected_12345 = "12345"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_12345, 5) != 0)
			return FALSE;

		// Large unsigned
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_u, (UINT32)4000000000);
		// Should format as "4000000000"
		auto expected_4b = "4000000000"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_4b, 10) != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestHexFormat()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = EMBED_FUNC(CharWriter);
		auto fmt_x = "%x"_embed;
		auto fmt_X = "%X"_embed;
		auto fmt_hash_x = "%#x"_embed;

		// Lowercase hex
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_x, (UINT32)0xABCD);
		auto expected_abcd = "abcd"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_abcd, 4) != 0)
			return FALSE;

		// Uppercase hex
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_X, (UINT32)0xABCD);
		auto expected_ABCD = "ABCD"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_ABCD, 4) != 0)
			return FALSE;

		// Hex with prefix
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_hash_x, (UINT32)0xFF);
		auto expected_0xff = "0xff"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_0xff, 4) != 0)
			return FALSE;

		// Zero in hex
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_x, (UINT32)0);
		auto expected_hex0 = "0"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_hex0, 2) != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestStringFormat()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = EMBED_FUNC(CharWriter);
		auto fmt_s = "%s"_embed;
		auto fmt_ss = "%s%s"_embed;
		auto testStr = "Hello"_embed;
		auto str1 = "A"_embed;
		auto str2 = "B"_embed;

		// Simple string
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_s, (const CHAR *)testStr);
		auto expected_hello = "Hello"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_hello, 5) != 0)
			return FALSE;

		// Multiple strings
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_ss, (const CHAR *)str1, (const CHAR *)str2);
		auto expected_AB = "AB"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_AB, 2) != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestCharFormat()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = EMBED_FUNC(CharWriter);
		auto fmt_c = "%c"_embed;
		auto fmt_ccc = "%c%c%c"_embed;

		// Single character
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_c, (INT32)'X');
		auto expected_X = "X"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_X, 2) != 0)
			return FALSE;

		// Multiple characters
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_ccc, (INT32)'A', (INT32)'B', (INT32)'C');
		auto expected_ABC = "ABC"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_ABC, 3) != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestWidthPadding()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = EMBED_FUNC(CharWriter);
		auto fmt_5d = "%5d"_embed;
		auto fmt_05d = "%05d"_embed;
		auto fmt_m5d = "%-5d"_embed;

		// Right-aligned with spaces (default)
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_5d, 42);
		// Should be "   42" (3 spaces + "42")
		auto expected_pad42 = "   42"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_pad42, 5) != 0)
			return FALSE;

		// Zero padding
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_05d, 42);
		// Should be "00042"
		auto expected_zero42 = "00042"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_zero42, 5) != 0)
			return FALSE;

		// Left-aligned
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_m5d, 42);
		// Should be "42   " (42 + 3 spaces)
		auto expected_left42 = "42   "_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_left42, 5) != 0)
			return FALSE;

		// Negative number with zero padding
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_05d, -7);
		// Should be "-0007"
		auto expected_neg7 = "-0007"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_neg7, 5) != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestFloatFormat()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = EMBED_FUNC(CharWriter);
		auto fmt_2f = "%.2f"_embed;
		auto fmt_0f = "%.0f"_embed;
		auto fmt_1f = "%.1f"_embed;

		// Simple float - now passing DOUBLE directly!
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_2f, 3.14_embed);
		// Should be "3.14"
		auto expected_314 = "3.14"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_314, 4) != 0)
			return FALSE;

		// Integer value as float - now passing DOUBLE directly!
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_0f, 42.0_embed);
		// Should be "42"
		auto expected_f42 = "42"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_f42, 2) != 0)
			return FALSE;

		// Negative float - now passing DOUBLE directly!
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_1f, -1.5_embed);
		// Should be "-1.5"
		auto expected_neg15 = "-1.5"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_neg15, 4) != 0)
			return FALSE;

		return TRUE;
	}

	static BOOL TestPercentLiteral()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = EMBED_FUNC(CharWriter);
		auto fmt = "100%%"_embed;

		// Double percent becomes single percent
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt);
		// Should be "100%"
		auto expected_100pct = "100%"_embed;
		if (Memory::Compare(buffer, (const CHAR *)expected_100pct, 4) != 0)
			return FALSE;

		return TRUE;
	}

	// static BOOL TestSizeFormat()
	// {
	// 	CHAR buffer[64];
	// 	BufferContext ctx;
	// 	ctx.buffer = buffer;
	// 	ctx.index = 0;
	// 	ctx.maxSize = 64;
	// 	auto fixed = EMBED_FUNC(CharWriter);
	// 	auto fmt_zu = "%zu"_embed;
	// 	auto fmt_zd = "%zd"_embed;

	// 	// Test USIZE formatting (positive value)
	// 	Memory::Zero(buffer, 64);
	// 	ctx.index = 0;
	// 	StringFormatter::Format<CHAR>(fixed, &ctx, fmt_zu, (USIZE)1024);
	// 	auto expected_1024 = "1024"_embed;
	// 	if (Memory::Compare(buffer, (const CHAR*)expected_1024, 4) != 0)
	// 		return FALSE;

	// 	// Test SSIZE formatting (positive value)
	// 	Memory::Zero(buffer, 64);
	// 	ctx.index = 0;
	// 	StringFormatter::Format<CHAR>(fixed, &ctx, fmt_zd, (SSIZE)512);
	// 	auto expected_512 = "512"_embed;
	// 	if (Memory::Compare(buffer, (const CHAR*)expected_512, 3) != 0)
	// 		return FALSE;

	// 	// Test SSIZE formatting (negative value)
	// 	Memory::Zero(buffer, 64);
	// 	ctx.index = 0;
	// 	StringFormatter::Format<CHAR>(fixed, &ctx, fmt_zd, (SSIZE)-256);
	// 	auto expected_neg256 = "-256"_embed;
	// 	if (Memory::Compare(buffer, (const CHAR*)expected_neg256, 4) != 0)
	// 		return FALSE;

	// 	// Test USIZE with zero
	// 	Memory::Zero(buffer, 64);
	// 	ctx.index = 0;
	// 	StringFormatter::Format<CHAR>(fixed, &ctx, fmt_zu, (USIZE)0);
	// 	auto expected_0 = "0"_embed;
	// 	if (Memory::Compare(buffer, (const CHAR*)expected_0, 1) != 0)
	// 		return FALSE;

	// 	return TRUE;
	// }
};
