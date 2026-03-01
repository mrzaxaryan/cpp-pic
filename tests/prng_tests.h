#pragma once

#include "runtime/runtime.h"
#include "tests.h"

class PrngTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Prng Tests...");

		RunTest(allPassed, EMBED_FUNC(TestDeterministicSequence), L"Deterministic sequence with known seed"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDifferentSeeds), L"Different seeds produce different sequences"_embed);
		RunTest(allPassed, EMBED_FUNC(TestValueRange), L"Values within [0, MAX)"_embed);
		RunTest(allPassed, EMBED_FUNC(TestGetArray), L"GetArray fills buffer"_embed);
		RunTest(allPassed, EMBED_FUNC(TestGetChar), L"GetChar produces lowercase a-z"_embed);
		RunTest(allPassed, EMBED_FUNC(TestGetString), L"GetString fills and null-terminates"_embed);
		RunTest(allPassed, EMBED_FUNC(TestIsSeeded), L"IsSeeded and Seed"_embed);

		if (allPassed)
			LOG_INFO("All Prng tests passed!");
		else
			LOG_ERROR("Some Prng tests failed!");

		return allPassed;
	}

private:
	static BOOL TestDeterministicSequence()
	{
		Prng prng(1);

		// Expected xorshift64 output for seed=1
		constexpr INT32 expected[] = {1082269761, 201397313, 1854285353, 1432191013, 274305637};
		auto embedded = MakeEmbedArray(expected);

		for (INT32 i = 0; i < 5; i++)
		{
			INT32 val = prng.Get();
			if (val != embedded[(USIZE)i])
			{
				LOG_ERROR("Seed 1, index %d: expected %d, got %d", i, embedded[(USIZE)i], val);
				return false;
			}
		}

		return true;
	}

	static BOOL TestDifferentSeeds()
	{
		Prng a(1);
		Prng b(42);

		// First values from different seeds must differ
		INT32 va = a.Get();
		INT32 vb = b.Get();

		if (va == vb)
		{
			LOG_ERROR("Seeds 1 and 42 produced same first value: %d", va);
			return false;
		}

		return true;
	}

	static BOOL TestValueRange()
	{
		Prng prng(12345);

		for (INT32 i = 0; i < 1000; i++)
		{
			INT32 val = prng.Get();
			if (val < 0 || val >= Prng::Max)
			{
				LOG_ERROR("Value out of range: %d (max: %d)", val, Prng::Max);
				return false;
			}
		}

		return true;
	}

	static BOOL TestGetArray()
	{
		Prng prng(99);
		UINT8 buffer[32];
		Memory::Zero(buffer, sizeof(buffer));

		INT32 result = prng.GetArray(Span<UINT8>(buffer, 32));
		if (result != 1)
		{
			LOG_ERROR("GetArray returned %d, expected 1", result);
			return false;
		}

		// At least some bytes should be non-zero
		BOOL foundNonZero = false;
		for (USIZE i = 0; i < 32; i++)
		{
			if (buffer[i] != 0)
			{
				foundNonZero = true;
				break;
			}
		}

		if (!foundNonZero)
		{
			LOG_ERROR("All 32 bytes are zero after GetArray");
			return false;
		}

		return true;
	}

	static BOOL TestGetChar()
	{
		Prng prng(777);

		for (INT32 i = 0; i < 100; i++)
		{
			CHAR c = prng.GetChar<CHAR>();
			if (c < 'a' || c > 'z')
			{
				LOG_ERROR("Char out of range: 0x%02X", (UINT32)(UINT8)c);
				return false;
			}
		}

		for (INT32 i = 0; i < 100; i++)
		{
			WCHAR c = prng.GetChar<WCHAR>();
			if (c < L'a' || c > L'z')
			{
				LOG_ERROR("Wide char out of range: 0x%04X", (UINT32)c);
				return false;
			}
		}

		return true;
	}

	static BOOL TestGetString()
	{
		Prng prng(555);
		CHAR buffer[16];

		UINT32 len = prng.GetString<CHAR>(Span<CHAR>(buffer, 11));
		if (len != 10)
		{
			LOG_ERROR("String length: expected 10, got %u", len);
			return false;
		}

		if (buffer[10] != '\0')
		{
			LOG_ERROR("String not null-terminated at position 10");
			return false;
		}

		for (UINT32 i = 0; i < len; i++)
		{
			if (buffer[i] < 'a' || buffer[i] > 'z')
			{
				LOG_ERROR("String char[%u] out of range: 0x%02X", i, (UINT32)(UINT8)buffer[i]);
				return false;
			}
		}

		// Empty string
		CHAR empty[4];
		UINT32 emptyLen = prng.GetString<CHAR>(Span<CHAR>(empty, 1));
		if (emptyLen != 0 || empty[0] != '\0')
		{
			LOG_ERROR("Empty string: expected len=0 and null terminator");
			return false;
		}

		return true;
	}

	static BOOL TestIsSeeded()
	{
		Prng prng;
		if (prng.IsSeeded())
		{
			LOG_ERROR("Default-constructed Prng reports seeded");
			return false;
		}

		prng.Seed(42);
		if (!prng.IsSeeded())
		{
			LOG_ERROR("Prng reports unseeded after Seed(42)");
			return false;
		}

		return true;
	}
};
