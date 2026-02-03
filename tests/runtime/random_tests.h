#pragma once

#include "runtime.h"
#include "random.h"
#include "tests.h"

class RandomTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		LOG_INFO("Running Random Tests...");

		// Test 1: Basic instantiation
		LOG_INFO("  Creating Random object...");
		Random rng;
		LOG_INFO("  Random object created!");

		RUN_TEST(allPassed, TestBasicGeneration, "Basic random number generation");
		RUN_TEST(allPassed, TestValueRange, "Random values within range");
		RUN_TEST(allPassed, TestSequenceVariability, "Random sequence variability");
		RUN_TEST(allPassed, TestCharGeneration, "Random character generation");
		RUN_TEST(allPassed, TestStringGenerationNarrow, "Random string generation (narrow)");
		RUN_TEST(allPassed, TestStringGenerationWide, "Random string generation (wide)");
		RUN_TEST(allPassed, TestByteArrayGeneration, "Random byte array generation");
		RUN_TEST(allPassed, TestEmptyString, "Empty string generation");

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
		return TRUE;
	}

	static BOOL TestValueRange()
	{
		Random rng;

		// Test 100 random values to ensure they're all within range [0, Random::MAX]
		for (INT32 i = 0; i < 100; i++)
		{
			INT32 val = rng.Get();
			if (val < 0 || val >= Random::MAX)
				return FALSE;
		}

		return TRUE;
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
		BOOL foundDifferent = FALSE;
		for (INT32 i = 1; i < 20; i++)
		{
			if (values[i] != values[0])
			{
				foundDifferent = TRUE;
				break;
			}
		}

		return foundDifferent;
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
				return FALSE;
		}

		// Test wide char generation
		for (INT32 i = 0; i < 50; i++)
		{
			WCHAR c = rng.GetChar<WCHAR>();
			// Verify character is lowercase a-z
			if (c < L'a' || c > L'z')
				return FALSE;
		}

		return TRUE;
	}

	static BOOL TestStringGenerationNarrow()
	{
		Random rng;
		CHAR buffer[32];

		// Generate string of length 10
		UINT32 len = rng.GetString<CHAR>(buffer, 10);

		// Verify length
		if (len != 10)
			return FALSE;

		// Verify null termination
		if (buffer[10] != '\0')
			return FALSE;

		// Verify all characters are lowercase letters
		for (UINT32 i = 0; i < len; i++)
		{
			if (buffer[i] < 'a' || buffer[i] > 'z')
				return FALSE;
		}

		return TRUE;
	}

	static BOOL TestStringGenerationWide()
	{
		Random rng;
		WCHAR buffer[32];

		// Generate string of length 15
		UINT32 len = rng.GetString<WCHAR>(buffer, 15);

		// Verify length
		if (len != 15)
			return FALSE;

		// Verify null termination
		if (buffer[15] != L'\0')
			return FALSE;

		// Verify all characters are lowercase letters
		for (UINT32 i = 0; i < len; i++)
		{
			if (buffer[i] < L'a' || buffer[i] > L'z')
				return FALSE;
		}

		return TRUE;
	}

	static BOOL TestByteArrayGeneration()
	{
		Random rng;
		UINT8 buffer[64];

		// Initialize buffer to known value
		Memory::Zero(buffer, sizeof(buffer));

		// Fill buffer with random bytes
		INT32 result = rng.GetArray(64, buffer);

		// Verify success
		if (result != 1)
			return FALSE;

		// Verify at least some bytes are non-zero (very unlikely all would be zero)
		BOOL foundNonZero = FALSE;
		for (USIZE i = 0; i < 64; i++)
		{
			if (buffer[i] != 0)
			{
				foundNonZero = TRUE;
				break;
			}
		}

		return foundNonZero;
	}

	static BOOL TestEmptyString()
	{
		Random rng;
		CHAR buffer[16];

		// Generate string of length 0
		UINT32 len = rng.GetString<CHAR>(buffer, 0);

		// Verify length is 0
		if (len != 0)
			return FALSE;

		// Verify null termination at position 0
		if (buffer[0] != '\0')
			return FALSE;

		return TRUE;
	}
};
