#pragma once

#include "runtime.h"
#include "random.h"
#include "tests.h"

class RandomTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Random Tests...");

		// Test 1: Basic instantiation
		LOG_INFO("  Creating Random object...");
		Random rng;
		LOG_INFO("  Random object created!");

		RunTest(allPassed, EMBED_FUNC(TestBasicGeneration), L"Basic random number generation"_embed);
		RunTest(allPassed, EMBED_FUNC(TestValueRange), L"Random values within range"_embed);
		RunTest(allPassed, EMBED_FUNC(TestSequenceVariability), L"Random sequence variability"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCharGeneration), L"Random character generation"_embed);
		RunTest(allPassed, EMBED_FUNC(TestStringGenerationNarrow), L"Random string generation (narrow)"_embed);
		RunTest(allPassed, EMBED_FUNC(TestStringGenerationWide), L"Random string generation (wide)"_embed);
		RunTest(allPassed, EMBED_FUNC(TestByteArrayGeneration), L"Random byte array generation"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEmptyString), L"Empty string generation"_embed);

		if (allPassed)
			LOG_INFO("All Random tests passed!");
		else
			LOG_ERROR("Some Random tests failed!");

		return allPassed;
	}

private:
	static BOOL TestBasicGeneration()
	{
		Random rng;

		// Generate a few random numbers and verify they're generated
		// We just verify the calls succeed without checking specific values
		rng.Get();
		rng.Get();
		rng.Get();

		// Just verify we got values (not checking for specific values due to randomness)
		// We'll check range in another test
		return true;
	}

	static BOOL TestValueRange()
	{
		Random rng;

		// Test 100 random values to ensure they're all within range [0, Random::MAX]
		for (INT32 i = 0; i < 100; i++)
		{
			INT32 val = rng.Get();
			if (val < 0 || val >= Random::MAX)
			{
				LOG_ERROR("Random value out of range: %d (max: %d)", val, Random::MAX);
				return false;
			}
		}

		return true;
	}

	static BOOL TestSequenceVariability()
	{
		Random rng;

		// Generate 20 values and verify they're not all the same
		INT32 values[20];
		for (INT32 i = 0; i < 20; i++)
		{
			values[i] = rng.Get();
		}

		// Check that at least some values differ
		BOOL foundDifferent = false;
		for (INT32 i = 1; i < 20; i++)
		{
			if (values[i] != values[0])
			{
				foundDifferent = true;
				break;
			}
		}

		if (!foundDifferent)
		{
			LOG_ERROR("All 20 random values are identical: %d", values[0]);
			return false;
		}
		return true;
	}

	static BOOL TestCharGeneration()
	{
		Random rng;

		// Test narrow char generation
		for (INT32 i = 0; i < 50; i++)
		{
			CHAR c = rng.GetChar<CHAR>();
			// Verify character is lowercase a-z
			if (c < 'a' || c > 'z')
			{
				LOG_ERROR("Narrow char out of range: 0x%02X", (UINT32)(UINT8)c);
				return false;
			}
		}

		// Test wide char generation
		for (INT32 i = 0; i < 50; i++)
		{
			WCHAR c = rng.GetChar<WCHAR>();
			// Verify character is lowercase a-z
			if (c < L'a' || c > L'z')
			{
				LOG_ERROR("Wide char out of range: 0x%04X", (UINT32)c);
				return false;
			}
		}

		return true;
	}

	static BOOL TestStringGenerationNarrow()
	{
		Random rng;
		CHAR buffer[32];

		// Generate string of length 10
		UINT32 len = rng.GetString<CHAR>(Span<CHAR>(buffer, 10));

		// Verify length
		if (len != 10)
		{
			LOG_ERROR("Narrow string length: expected 10, got %u", len);
			return false;
		}

		// Verify null termination
		if (buffer[10] != '\0')
		{
			LOG_ERROR("Narrow string not null-terminated at position 10");
			return false;
		}

		// Verify all characters are lowercase letters
		for (UINT32 i = 0; i < len; i++)
		{
			if (buffer[i] < 'a' || buffer[i] > 'z')
			{
				LOG_ERROR("Narrow string char[%u] out of range: 0x%02X", i, (UINT32)(UINT8)buffer[i]);
				return false;
			}
		}

		return true;
	}

	static BOOL TestStringGenerationWide()
	{
		Random rng;
		WCHAR buffer[32];

		// Generate string of length 15
		UINT32 len = rng.GetString<WCHAR>(Span<WCHAR>(buffer, 15));

		// Verify length
		if (len != 15)
		{
			LOG_ERROR("Wide string length: expected 15, got %u", len);
			return false;
		}

		// Verify null termination
		if (buffer[15] != L'\0')
		{
			LOG_ERROR("Wide string not null-terminated at position 15");
			return false;
		}

		// Verify all characters are lowercase letters
		for (UINT32 i = 0; i < len; i++)
		{
			if (buffer[i] < L'a' || buffer[i] > L'z')
			{
				LOG_ERROR("Wide string char[%u] out of range: 0x%04X", i, (UINT32)buffer[i]);
				return false;
			}
		}

		return true;
	}

	static BOOL TestByteArrayGeneration()
	{
		Random rng;
		UINT8 buffer[64];

		// Initialize buffer to known value
		Memory::Zero(buffer, sizeof(buffer));

		// Fill buffer with random bytes
		INT32 result = rng.GetArray(Span<UINT8>(buffer, 64));

		// Verify success
		if (result != 1)
		{
			LOG_ERROR("GetArray returned %d, expected 1", result);
			return false;
		}

		// Verify at least some bytes are non-zero (very unlikely all would be zero)
		BOOL foundNonZero = false;
		for (USIZE i = 0; i < 64; i++)
		{
			if (buffer[i] != 0)
			{
				foundNonZero = true;
				break;
			}
		}

		if (!foundNonZero)
		{
			LOG_ERROR("All 64 random bytes are zero");
			return false;
		}
		return true;
	}

	static BOOL TestEmptyString()
	{
		Random rng;
		CHAR buffer[16];

		// Generate string of length 0
		UINT32 len = rng.GetString<CHAR>(Span<CHAR>(buffer, 0));

		// Verify length is 0
		if (len != 0)
		{
			LOG_ERROR("Empty string length: expected 0, got %u", len);
			return false;
		}

		// Verify null termination at position 0
		if (buffer[0] != '\0')
		{
			LOG_ERROR("Empty string not null-terminated at position 0");
			return false;
		}

		return true;
	}
};
