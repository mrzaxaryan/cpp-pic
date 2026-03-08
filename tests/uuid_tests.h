#pragma once

#include "runtime/runtime.h"
#include "tests.h"

// =============================================================================
// Compile-Time (constexpr) Verification
// =============================================================================

// Default UUID is nil (all zeros)
static_assert(UUID().GetMostSignificantBits() == 0);
static_assert(UUID().GetLeastSignificantBits() == 0);

// UUID from known bytes: 550e8400-e29b-41d4-a716-446655440000
constexpr UINT8 kKnownUuidBytes[16] = {
	0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
	0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
};
static_assert(UUID(Span<const UINT8, 16>(kKnownUuidBytes)).GetMostSignificantBits() == 0x550e8400e29b41d4ULL);
static_assert(UUID(Span<const UINT8, 16>(kKnownUuidBytes)).GetLeastSignificantBits() == 0xa716446655440000ULL);

// =============================================================================
// UuidTests — Runtime Validation
// =============================================================================

class UuidTests
{
private:
	static BOOL TestNilUuid()
	{
		UUID nil;
		if (nil.GetMostSignificantBits() != 0)
			return false;
		if (nil.GetLeastSignificantBits() != 0)
			return false;
		return true;
	}

	static BOOL TestFromBytes()
	{
		auto mArray = MakeEmbedArray<UINT8>(
			0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
			0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
		);
		UINT8 unpacked[16];
		for (USIZE i = 0; i < 16; i++)
			unpacked[i] = mArray[i];
		Span<const UINT8, 16> unpackedSpan(unpacked);
		UUID uuid(unpackedSpan);
		if (uuid.GetMostSignificantBits() != 0x550e8400e29b41d4ULL)
			return false;
		if (uuid.GetLeastSignificantBits() != 0xa716446655440000ULL)
			return false;
		return true;
	}

	static BOOL TestFromStringLowercase()
	{
		auto str = "550e8400-e29b-41d4-a716-446655440000"_embed;
		auto result = UUID::FromString((PCCHAR)str);
		if (!result)
			return false;
		auto &uuid = result.Value();
		if (uuid.GetMostSignificantBits() != 0x550e8400e29b41d4ULL)
			return false;
		if (uuid.GetLeastSignificantBits() != 0xa716446655440000ULL)
			return false;
		return true;
	}

	static BOOL TestFromStringUppercase()
	{
		auto str = "550E8400-E29B-41D4-A716-446655440000"_embed;
		auto result = UUID::FromString((PCCHAR)str);
		if (!result)
			return false;
		auto &uuid = result.Value();
		if (uuid.GetMostSignificantBits() != 0x550e8400e29b41d4ULL)
			return false;
		if (uuid.GetLeastSignificantBits() != 0xa716446655440000ULL)
			return false;
		return true;
	}

	static BOOL TestToStringRoundTrip()
	{
		auto str = "550e8400-e29b-41d4-a716-446655440000"_embed;
		auto parseResult = UUID::FromString((PCCHAR)str);
		if (!parseResult)
			return false;
		CHAR buffer[37];
		auto toStrResult = parseResult.Value().ToString(Span<CHAR>(buffer));
		if (!toStrResult)
			return false;
		auto expected = "550e8400-e29b-41d4-a716-446655440000"_embed;
		if (!StringUtils::Equals(buffer, (PCCHAR)expected))
		{
			LOG_ERROR("ToString mismatch: got '%s', expected '%s'", buffer, (PCCHAR)expected);
			return false;
		}
		return true;
	}

	static BOOL TestNilUuidToString()
	{
		UUID nil;
		CHAR buffer[37];
		auto result = nil.ToString(Span<CHAR>(buffer));
		if (!result)
			return false;
		auto expected = "00000000-0000-0000-0000-000000000000"_embed;
		if (!StringUtils::Equals(buffer, (PCCHAR)expected))
		{
			LOG_ERROR("Nil UUID ToString mismatch: got '%s'", buffer);
			return false;
		}
		return true;
	}

	static BOOL TestToStringBufferTooSmall()
	{
		UUID nil;
		CHAR buffer[36]; // one byte too small (need 37)
		auto result = nil.ToString(Span<CHAR>(buffer));
		if (result)
			return false;
		return true;
	}

	static BOOL TestFromStringInvalidChar()
	{
		// 'g' is not a valid hex character
		auto str = "550e8400-e29b-41d4-a716-44665544000g"_embed;
		auto result = UUID::FromString((PCCHAR)str);
		if (result)
			return false;
		return true;
	}

	static BOOL TestFromStringTooShort()
	{
		// 31 hex digits instead of 32
		auto str = "550e8400-e29b-41d4-a716-44665544000"_embed;
		auto result = UUID::FromString((PCCHAR)str);
		if (result)
			return false;
		return true;
	}

	static BOOL TestFromStringTooLong()
	{
		// 33 hex digits instead of 32
		auto str = "550e8400-e29b-41d4-a716-4466554400001"_embed;
		auto result = UUID::FromString((PCCHAR)str);
		if (result)
			return false;
		return true;
	}

	static BOOL TestFromStringNoDashes()
	{
		// 32 hex digits with no dashes — still valid (dashes are ignored)
		auto str = "550e8400e29b41d4a716446655440000"_embed;
		auto result = UUID::FromString((PCCHAR)str);
		if (!result)
			return false;
		auto &uuid = result.Value();
		if (uuid.GetMostSignificantBits() != 0x550e8400e29b41d4ULL)
			return false;
		if (uuid.GetLeastSignificantBits() != 0xa716446655440000ULL)
			return false;
		return true;
	}

public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running UUID Tests...");

		RunTest(allPassed, EMBED_FUNC(TestNilUuid), "nil UUID default constructor"_embed);
		RunTest(allPassed, EMBED_FUNC(TestFromBytes), "UUID construction from bytes"_embed);
		RunTest(allPassed, EMBED_FUNC(TestFromStringLowercase), "FromString lowercase hex"_embed);
		RunTest(allPassed, EMBED_FUNC(TestFromStringUppercase), "FromString uppercase hex"_embed);
		RunTest(allPassed, EMBED_FUNC(TestToStringRoundTrip), "ToString round-trip"_embed);
		RunTest(allPassed, EMBED_FUNC(TestNilUuidToString), "nil UUID ToString"_embed);
		RunTest(allPassed, EMBED_FUNC(TestToStringBufferTooSmall), "ToString rejects buffer < 37 bytes"_embed);
		RunTest(allPassed, EMBED_FUNC(TestFromStringInvalidChar), "FromString rejects invalid hex char"_embed);
		RunTest(allPassed, EMBED_FUNC(TestFromStringTooShort), "FromString rejects too few hex digits"_embed);
		RunTest(allPassed, EMBED_FUNC(TestFromStringTooLong), "FromString rejects too many hex digits"_embed);
		RunTest(allPassed, EMBED_FUNC(TestFromStringNoDashes), "FromString accepts no-dash format"_embed);

		if (allPassed)
			LOG_INFO("All UUID tests passed!");
		else
			LOG_ERROR("Some UUID tests failed!");

		return allPassed;
	}
};
