#include "base64.h"

/// Padding character '=' used to align encoded output to a 4-character boundary
/// @see RFC 4648 Section 3.5 — Canonical Encoding
///      https://datatracker.ietf.org/doc/html/rfc4648#section-3.5
#define BASE64_PAD '='

/// Branchless mapping from a 6-bit value (0–63) to its Base64 alphabet character.
/// Uses arithmetic offsets to avoid a lookup table in .rdata.
///
/// The standard Base64 alphabet (RFC 4648 Section 4, Table 1):
///   0–25  → 'A'–'Z'  (offset: +'A')
///   26–51 → 'a'–'z'  (offset: +'a' - 26)
///   52–61 → '0'–'9'  (offset: +'0' - 52)
///   62    → '+'
///   63    → '/'
///
/// @see RFC 4648 Section 4 — Base 64 Encoding (Table 1: The Base 64 Alphabet)
///      https://datatracker.ietf.org/doc/html/rfc4648#section-4
#define BASE64_ENCODED_CHAR(v) ( \
    'A' + (((UINT32)(v)) & 63u) + ((((UINT32)(v)) & 63u) >= 26u) * ('a' - 'A' - 26) + ((((UINT32)(v)) & 63u) >= 52u) * ('0' - 'a' - 26) + ((((UINT32)(v)) & 63u) >= 62u) * ('+' - '0' - 10) + ((((UINT32)(v)) & 63u) >= 63u) * ('/' - '+' - 1))

/// Branchless mapping from a Base64 alphabet character to its 6-bit value (0–63).
/// Reverses the encoding alphabet (RFC 4648 Section 4, Table 1) using arithmetic
/// range checks. Returns 0 for any character not in the alphabet.
///
/// @see RFC 4648 Section 4 — Base 64 Encoding (Table 1: The Base 64 Alphabet)
///      https://datatracker.ietf.org/doc/html/rfc4648#section-4
#define BASE64_DECODE_CHAR(c) (                     \
    ((c) >= 'A' && (c) <= 'Z') * ((c) - 'A') +      \
    ((c) >= 'a' && (c) <= 'z') * ((c) - 'a' + 26) + \
    ((c) >= '0' && (c) <= '9') * ((c) - '0' + 52) + \
    ((c) == '+') * 62 +                             \
    ((c) == '/') * 63)

/**
 * @brief Encodes binary data to Base64 format
 *
 * @details Implements the encoding procedure from RFC 4648 Section 4:
 *
 * 1. Input bytes are consumed in 3-byte (24-bit) groups
 * 2. Each 24-bit group is split into four 6-bit values
 * 3. Each 6-bit value is mapped to the Base64 alphabet via BASE64_ENCODED_CHAR
 * 4. When fewer than 3 bytes remain (RFC 4648 Section 3.5):
 *    - 1 byte  → 2 alphabet chars + "==" (8 bits padded to 12, 2 zero bits discarded)
 *    - 2 bytes → 3 alphabet chars + "="  (16 bits padded to 18, 2 zero bits discarded)
 * 5. Output is null-terminated
 *
 * @see RFC 4648 Section 4 — Base 64 Encoding
 *      https://datatracker.ietf.org/doc/html/rfc4648#section-4
 * @see RFC 4648 Section 3.5 — Canonical Encoding (padding rules)
 *      https://datatracker.ietf.org/doc/html/rfc4648#section-3.5
 */
void Base64::Encode(Span<const CHAR> input, Span<CHAR> output)
{
	UINT32 inputSize = (UINT32)input.Size();
	UINT32 i, o;

	i = 0;
	o = 0;

	/* Process full 3-byte (24-bit) groups — RFC 4648 Section 4 */
	while (i + 2u < inputSize)
	{
		UINT32 v =
			((UINT32)(UINT8)input[i] << 16) |
			((UINT32)(UINT8)input[i + 1] << 8) |
			((UINT32)(UINT8)input[i + 2]);

		output[o++] = (CHAR)BASE64_ENCODED_CHAR((v >> 18) & 63u);
		output[o++] = (CHAR)BASE64_ENCODED_CHAR((v >> 12) & 63u);
		output[o++] = (CHAR)BASE64_ENCODED_CHAR((v >> 6) & 63u);
		output[o++] = (CHAR)BASE64_ENCODED_CHAR(v & 63u);

		i += 3u;
	}

	/* Tail padding — RFC 4648 Section 3.5 */
	if (i < inputSize)
	{
		UINT32 v = ((UINT32)(UINT8)input[i] << 16);

		output[o++] = (CHAR)BASE64_ENCODED_CHAR((v >> 18) & 63u);

		if (i + 1u < inputSize)
		{
			/* 2 remaining bytes → 3 encoded chars + 1 pad */
			v |= ((UINT32)(UINT8)input[i + 1] << 8);

			output[o++] = (CHAR)BASE64_ENCODED_CHAR((v >> 12) & 63u);
			output[o++] = (CHAR)BASE64_ENCODED_CHAR((v >> 6) & 63u);
			output[o++] = (CHAR)BASE64_PAD;
		}
		else
		{
			/* 1 remaining byte → 2 encoded chars + 2 pads */
			output[o++] = (CHAR)BASE64_ENCODED_CHAR((v >> 12) & 63u);
			output[o++] = (CHAR)BASE64_PAD;
			output[o++] = (CHAR)BASE64_PAD;
		}
	}

	output[o] = 0;
}

/**
 * @brief Decodes Base64 formatted data back to binary
 *
 * @details Implements the decoding procedure from RFC 4648 Section 4:
 *
 * 1. Input is consumed in 4-character blocks
 * 2. Each character is reverse-mapped to its 6-bit value via BASE64_DECODE_CHAR
 * 3. The four 6-bit values are combined into a 24-bit group
 * 4. The 24-bit group is split into 3 output bytes
 * 5. Padding characters ('=') in the last block reduce output:
 *    - "xx==" → 1 output byte  (RFC 4648 Section 3.5)
 *    - "xxx=" → 2 output bytes (RFC 4648 Section 3.5)
 *
 * @see RFC 4648 Section 4 — Base 64 Encoding (decoding procedure)
 *      https://datatracker.ietf.org/doc/html/rfc4648#section-4
 * @see RFC 4648 Section 3.3 — Interpretation of Non-Alphabet Characters
 *      https://datatracker.ietf.org/doc/html/rfc4648#section-3.3
 */
Result<void, Error> Base64::Decode(Span<const CHAR> input, Span<CHAR> output)
{
	UINT32 inputSize = (UINT32)input.Size();
	UINT32 i, o;

	i = 0;
	o = 0;

	/* Process full 4-character blocks — RFC 4648 Section 4 */
	while (i + 3u < inputSize)
	{
		UINT32 a, b, c, d;
		UINT32 v;

		if (input[i] == BASE64_PAD)
			break;

		a = BASE64_DECODE_CHAR(input[i]);
		b = BASE64_DECODE_CHAR(input[i + 1]);
		c = (input[i + 2] == BASE64_PAD) ? 0 : BASE64_DECODE_CHAR(input[i + 2]);
		d = (input[i + 3] == BASE64_PAD) ? 0 : BASE64_DECODE_CHAR(input[i + 3]);

		/* Reassemble 24-bit group from four 6-bit values */
		v = (a << 18) | (b << 12) | (c << 6) | d;

		output[o++] = (UINT8)((v >> 16) & 0xFF);

		/* Padding check — fewer output bytes when '=' is present (RFC 4648 Section 3.5) */
		if (input[i + 2] != BASE64_PAD)
			output[o++] = (UINT8)((v >> 8) & 0xFF);

		if (input[i + 3] != BASE64_PAD)
			output[o++] = (UINT8)(v & 0xFF);

		i += 4u;
	}

	return Result<void, Error>::Ok();
}
