#pragma once

#include "runtime/runtime.h"
#include "tests.h"

class StringFormatterTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running StringFormatter Tests...");

		RunTest(allPassed, &TestIntegerFormat, "Integer format");
		RunTest(allPassed, &TestUnsignedFormat, "Unsigned format");
		RunTest(allPassed, &TestHexFormat, "Hex format");
		RunTest(allPassed, &TestStringFormat, "String format");
		RunTest(allPassed, &TestCharFormat, "Char format");
		RunTest(allPassed, &TestWidthPadding, "Width and padding");
		RunTest(allPassed, &TestFloatFormat, "Float format");
		RunTest(allPassed, &TestPercentLiteral, "Percent literal");
		RunTest(allPassed, &TestErrorFormat, "Error format");
		// RunTest(allPassed, TestSizeFormat, L"Size format");

		if (allPassed)
			LOG_INFO("All StringFormatter tests passed!");
		else
			LOG_ERROR("Some StringFormatter tests failed!");

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
			return true;
		}
		return false;
	}

	static BOOL TestIntegerFormat()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = &CharWriter;
		auto fmt_d = "%d";
		// Positive integer
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_d, 42);
		auto expected_42 = "42";
		if (Memory::Compare(buffer, (const CHAR *)expected_42, 3) != 0)
		{
			LOG_ERROR("%%d 42: got '%s'", buffer);
			return false;
		}

		// Negative integer
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_d, -123);
		auto expected_neg123 = "-123";
		if (Memory::Compare(buffer, (const CHAR *)expected_neg123, 4) != 0)
		{
			LOG_ERROR("%%d -123: got '%s'", buffer);
			return false;
		}

		// Zero
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_d, 0);
		auto expected_0 = "0";
		if (Memory::Compare(buffer, (const CHAR *)expected_0, 2) != 0)
		{
			LOG_ERROR("%%d 0: got '%s'", buffer);
			return false;
		}

		return true;
	}

	static BOOL TestUnsignedFormat()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = &CharWriter;
		auto fmt_u = "%u";

		// Simple unsigned
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_u, (UINT32)12345);
		auto expected_12345 = "12345";
		if (Memory::Compare(buffer, (const CHAR *)expected_12345, 5) != 0)
		{
			LOG_ERROR("%%u 12345: got '%s'", buffer);
			return false;
		}

		// Large unsigned
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_u, (UINT32)4000000000);
		// Should format as "4000000000"
		auto expected_4b = "4000000000";
		if (Memory::Compare(buffer, (const CHAR *)expected_4b, 10) != 0)
		{
			LOG_ERROR("%%u 4000000000: got '%s'", buffer);
			return false;
		}

		return true;
	}

	static BOOL TestHexFormat()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = &CharWriter;
		auto fmt_x = "%x";
		auto fmt_X = "%X";
		auto fmt_hash_x = "%#x";

		// Lowercase hex
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_x, (UINT32)0xABCD);
		auto expected_abcd = "abcd";
		if (Memory::Compare(buffer, (const CHAR *)expected_abcd, 4) != 0)
		{
			LOG_ERROR("%%x 0xABCD: got '%s'", buffer);
			return false;
		}

		// Uppercase hex
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_X, (UINT32)0xABCD);
		auto expected_ABCD = "ABCD";
		if (Memory::Compare(buffer, (const CHAR *)expected_ABCD, 4) != 0)
		{
			LOG_ERROR("%%X 0xABCD: got '%s'", buffer);
			return false;
		}

		// Hex with prefix
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_hash_x, (UINT32)0xFF);
		auto expected_0xff = "0xff";
		if (Memory::Compare(buffer, (const CHAR *)expected_0xff, 4) != 0)
		{
			LOG_ERROR("%%#x 0xFF: got '%s'", buffer);
			return false;
		}

		// Zero in hex
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_x, (UINT32)0);
		auto expected_hex0 = "0";
		if (Memory::Compare(buffer, (const CHAR *)expected_hex0, 2) != 0)
		{
			LOG_ERROR("%%x 0: got '%s'", buffer);
			return false;
		}

		return true;
	}

	static BOOL TestStringFormat()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = &CharWriter;
		auto fmt_s = "%s";
		auto fmt_ss = "%s%s";
		auto testStr = "Hello";
		auto str1 = "A";
		auto str2 = "B";

		// Simple string
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_s, (const CHAR *)testStr);
		auto expected_hello = "Hello";
		if (Memory::Compare(buffer, (const CHAR *)expected_hello, 5) != 0)
		{
			LOG_ERROR("%%s 'Hello': got '%s'", buffer);
			return false;
		}

		// Multiple strings
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_ss, (const CHAR *)str1, (const CHAR *)str2);
		auto expected_AB = "AB";
		if (Memory::Compare(buffer, (const CHAR *)expected_AB, 2) != 0)
		{
			LOG_ERROR("%%s%%s 'A'+'B': got '%s'", buffer);
			return false;
		}

		return true;
	}

	static BOOL TestCharFormat()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = &CharWriter;
		auto fmt_c = "%c";
		auto fmt_ccc = "%c%c%c";

		// Single character
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_c, (INT32)'X');
		auto expected_X = "X";
		if (Memory::Compare(buffer, (const CHAR *)expected_X, 2) != 0)
		{
			LOG_ERROR("%%c 'X': got '%s'", buffer);
			return false;
		}

		// Multiple characters
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_ccc, (INT32)'A', (INT32)'B', (INT32)'C');
		auto expected_ABC = "ABC";
		if (Memory::Compare(buffer, (const CHAR *)expected_ABC, 3) != 0)
		{
			LOG_ERROR("%%c%%c%%c 'ABC': got '%s'", buffer);
			return false;
		}

		return true;
	}

	static BOOL TestWidthPadding()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = &CharWriter;
		auto fmt_5d = "%5d";
		auto fmt_05d = "%05d";
		auto fmt_m5d = "%-5d";

		// Right-aligned with spaces (default)
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_5d, 42);
		// Should be "   42" (3 spaces + "42")
		auto expected_pad42 = "   42";
		if (Memory::Compare(buffer, (const CHAR *)expected_pad42, 5) != 0)
		{
			LOG_ERROR("%%5d 42: got '%s'", buffer);
			return false;
		}

		// Zero padding
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_05d, 42);
		// Should be "00042"
		auto expected_zero42 = "00042";
		if (Memory::Compare(buffer, (const CHAR *)expected_zero42, 5) != 0)
		{
			LOG_ERROR("%%05d 42: got '%s'", buffer);
			return false;
		}

		// Left-aligned
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_m5d, 42);
		// Should be "42   " (42 + 3 spaces)
		auto expected_left42 = "42   ";
		if (Memory::Compare(buffer, (const CHAR *)expected_left42, 5) != 0)
		{
			LOG_ERROR("%%-5d 42: got '%s'", buffer);
			return false;
		}

		// Negative number with zero padding
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_05d, -7);
		// Should be "-0007"
		auto expected_neg7 = "-0007";
		if (Memory::Compare(buffer, (const CHAR *)expected_neg7, 5) != 0)
		{
			LOG_ERROR("%%05d -7: got '%s'", buffer);
			return false;
		}

		return true;
	}

	static BOOL TestFloatFormat()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = &CharWriter;
		auto fmt_2f = "%.2f";
		auto fmt_0f = "%.0f";
		auto fmt_1f = "%.1f";

		// Simple float - now passing DOUBLE directly!
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_2f, 3.14);
		// Should be "3.14"
		auto expected_314 = "3.14";
		if (Memory::Compare(buffer, (const CHAR *)expected_314, 4) != 0)
		{
			LOG_ERROR("%%.2f 3.14: got '%s'", buffer);
			return false;
		}

		// Integer value as float - now passing DOUBLE directly!
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_0f, 42.0);
		// Should be "42"
		auto expected_f42 = "42";
		if (Memory::Compare(buffer, (const CHAR *)expected_f42, 2) != 0)
		{
			LOG_ERROR("%%.0f 42.0: got '%s'", buffer);
			return false;
		}

		// Negative float - now passing DOUBLE directly!
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_1f, -1.5);
		// Should be "-1.5"
		auto expected_neg15 = "-1.5";
		if (Memory::Compare(buffer, (const CHAR *)expected_neg15, 4) != 0)
		{
			LOG_ERROR("%%.1f -1.5: got '%s'", buffer);
			return false;
		}

		return true;
	}

	static BOOL TestPercentLiteral()
	{
		CHAR buffer[64];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 64;
		auto fixed = &CharWriter;
		auto fmt = "100%%";

		// Double percent becomes single percent
		Memory::Zero(buffer, 64);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt);
		// Should be "100%"
		auto expected_100pct = "100%";
		if (Memory::Compare(buffer, (const CHAR *)expected_100pct, 4) != 0)
		{
			LOG_ERROR("100%%%% literal: got '%s'", buffer);
			return false;
		}

		return true;
	}

	static BOOL TestErrorFormat()
	{
		CHAR buffer[128];
		BufferContext ctx;
		ctx.buffer = buffer;
		ctx.index = 0;
		ctx.maxSize = 128;
		auto fixed = &CharWriter;
		auto fmt_e = "%e";

		// Test 1: Single runtime error -> "1"
		auto singleResult = Result<UINT32, Error>::Err(Error::Socket_CreateFailed_Open);
		Memory::Zero(buffer, 128);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_e, singleResult.Error());
		auto expected_single = "1";
		if (Memory::Compare(buffer, (const CHAR *)expected_single, 1) != 0)
		{
			LOG_ERROR("%%e runtime error: got '%s'", buffer);
			return false;
		}
		if (buffer[1] != '\0')
		{
			LOG_ERROR("%%e runtime error not null-terminated at pos 1");
			return false;
		}

		// Test 2: Windows error -> "0xC0000034[W]"
		auto winResult = Result<UINT32, Error>::Err(Error::Windows(0xC0000034));
		Memory::Zero(buffer, 128);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_e, winResult.Error());
		auto expected_win = "0xC0000034[W]";
		if (Memory::Compare(buffer, (const CHAR *)expected_win, 13) != 0)
		{
			LOG_ERROR("%%e Windows error: got '%s'", buffer);
			return false;
		}
		if (buffer[13] != '\0')
		{
			LOG_ERROR("%%e Windows error not null-terminated at pos 13");
			return false;
		}

		// Test 3: Posix error -> "111[P]"
		auto posixResult = Result<void, Error>::Err(Error::Posix(111));
		Memory::Zero(buffer, 128);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_e, posixResult.Error());
		auto expected_posix = "111[P]";
		if (Memory::Compare(buffer, (const CHAR *)expected_posix, 6) != 0)
		{
			LOG_ERROR("%%e Posix error: got '%s'", buffer);
			return false;
		}
		if (buffer[6] != '\0')
		{
			LOG_ERROR("%%e Posix error not null-terminated at pos 6");
			return false;
		}

		// Test 4: Chained propagation -> "16 <- 6 <- 0xC0000034[W]"
		auto twoCode = Result<UINT32, Error>::Err(
			Error::Windows(0xC0000034),
			Error::Socket_OpenFailed_Connect);
		auto propagated = Result<void, Error>::Err(twoCode, Error::Tls_OpenFailed_Socket);
		Memory::Zero(buffer, 128);
		ctx.index = 0;
		StringFormatter::Format<CHAR>(fixed, &ctx, fmt_e, propagated.Error());
		auto expected_prop = "16 <- 6 <- 0xC0000034[W]";
		USIZE propLen = 25;
		if (Memory::Compare(buffer, (const CHAR *)expected_prop, propLen) != 0)
		{
			LOG_ERROR("%%e propagated error: got '%s'", buffer);
			return false;
		}
		if (buffer[propLen] != '\0')
		{
			LOG_ERROR("%%e propagated error not null-terminated at pos %u", (UINT32)propLen);
			return false;
		}

		return true;
	}

	// static BOOL TestSizeFormat()
	// {
	// 	CHAR buffer[64];
	// 	BufferContext ctx;
	// 	ctx.buffer = buffer;
	// 	ctx.index = 0;
	// 	ctx.maxSize = 64;
	// 	auto fixed = &CharWriter;
	// 	auto fmt_zu = "%zu";
	// 	auto fmt_zd = "%zd";

	// 	// Test USIZE formatting (positive value)
	// 	Memory::Zero(buffer, 64);
	// 	ctx.index = 0;
	// 	StringFormatter::Format<CHAR>(fixed, &ctx, fmt_zu, (USIZE)1024);
	// 	auto expected_1024 = "1024";
	// 	if (Memory::Compare(buffer, (const CHAR*)expected_1024, 4) != 0)
	// 		return false;

	// 	// Test SSIZE formatting (positive value)
	// 	Memory::Zero(buffer, 64);
	// 	ctx.index = 0;
	// 	StringFormatter::Format<CHAR>(fixed, &ctx, fmt_zd, (SSIZE)512);
	// 	auto expected_512 = "512";
	// 	if (Memory::Compare(buffer, (const CHAR*)expected_512, 3) != 0)
	// 		return false;

	// 	// Test SSIZE formatting (negative value)
	// 	Memory::Zero(buffer, 64);
	// 	ctx.index = 0;
	// 	StringFormatter::Format<CHAR>(fixed, &ctx, fmt_zd, (SSIZE)-256);
	// 	auto expected_neg256 = "-256";
	// 	if (Memory::Compare(buffer, (const CHAR*)expected_neg256, 4) != 0)
	// 		return false;

	// 	// Test USIZE with zero
	// 	Memory::Zero(buffer, 64);
	// 	ctx.index = 0;
	// 	StringFormatter::Format<CHAR>(fixed, &ctx, fmt_zu, (USIZE)0);
	// 	auto expected_0 = "0";
	// 	if (Memory::Compare(buffer, (const CHAR*)expected_0, 1) != 0)
	// 		return false;

	// 	return true;
	// }
};
