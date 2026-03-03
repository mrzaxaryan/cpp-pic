/**
 * @file uuid.h
 * @brief Universally Unique Identifier (UUID) Generation and Manipulation
 *
 * @details Provides UUID generation (version 4, random), parsing from string
 * representation, and serialization to the standard 8-4-4-4-12 hex format.
 *
 * @see RFC 9562 — Universally Unique IDentifiers (UUIDs)
 *      https://datatracker.ietf.org/doc/html/rfc9562
 *
 * @ingroup core
 */

#pragma once

#include "core/types/primitives.h"
#include "core/types/span.h"
#include "core/types/error.h"
#include "core/types/result.h"
#include "core/types/embedded/embedded_string.h"
#include "core/memory/memory.h"

/**
 * @class UUID
 * @brief 128-bit Universally Unique Identifier
 *
 * @details Stores a 128-bit UUID as a 16-byte array and provides factory
 * methods for random generation and string parsing.
 */
class UUID
{
private:
	UINT8 data[16];

public:
	/**
	 * @brief Default constructor — initializes to nil UUID (all zeros)
	 */
	UUID()
	{
		Memory::Zero(data, 16);
	}

	/**
	 * @brief Construct from a 16-byte span
	 * @param bytes Exactly 16 bytes of UUID data
	 */
	UUID(Span<const UINT8, 16> bytes)
	{
		Memory::Copy(data, bytes.Data(), 16);
	}

	/**
	 * @brief Parse a UUID from its string representation
	 * @param str Null-terminated UUID string (e.g., "550e8400-e29b-41d4-a716-446655440000")
	 * @return Parsed UUID (unrecognized characters are skipped)
	 */
	static UUID FromString(PCCHAR str)
	{
		UINT8 bytes[16];
		Memory::Zero(bytes, 16);
		INT32 byteIndex = 0;
		INT32 count = 0;

		for (INT32 i = 0; str[i] != '\0'; i++)
		{
			if (str[i] == '-')
				continue;
			if (byteIndex >= 16)
				break;

			UINT8 value = 0;
			CHAR c = str[i];
			if (c >= '0' && c <= '9')
				value = c - '0';
			else if (c >= 'a' && c <= 'f')
				value = c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				value = c - 'A' + 10;
			else
				continue;

			if (count == 0)
			{
				bytes[byteIndex] = value << 4;
				count = 1;
			}
			else
			{
				bytes[byteIndex] |= value;
				byteIndex++;
				count = 0;
			}
		}

		return UUID(Span<const UINT8, 16>(bytes));
	}

	/**
	 * @brief Convert UUID to its standard string representation
	 * @param buffer Output buffer (must be at least 37 bytes: 32 hex + 4 dashes + null)
	 * @return Ok on success, Err if the buffer is too small
	 */
	[[nodiscard]] Result<void, Error> ToString(Span<CHAR> buffer) const
	{
		if (buffer.Size() < 37)
			return Result<void, Error>::Err(Error::Uuid_ToStringFailed);

		INT32 index = 0;
		auto hex = "0123456789abcdef"_embed;

		for (INT32 i = 0; i < 16; i++)
		{
			buffer[index++] = hex[(data[i] >> 4) & 0xF];
			buffer[index++] = hex[data[i] & 0xF];
			if (i == 3 || i == 5 || i == 7 || i == 9)
				buffer[index++] = '-';
		}
		buffer[index] = '\0';

		return Result<void, Error>::Ok();
	}

	/**
	 * @brief Get the most significant 64 bits of the UUID
	 * @return Upper 64 bits (bytes 0-7)
	 */
	UINT64 GetMostSignificantBits() const
	{
		UINT64 msb = 0;
		for (INT32 i = 0; i < 8; i++)
		{
			msb = (msb << 8) | data[i];
		}
		return msb;
	}

	/**
	 * @brief Get the least significant 64 bits of the UUID
	 * @return Lower 64 bits (bytes 8-15)
	 */
	UINT64 GetLeastSignificantBits() const
	{
		UINT64 lsb = 0;
		for (INT32 i = 8; i < 16; i++)
		{
			lsb = (lsb << 8) | data[i];
		}
		return lsb;
	}
};
