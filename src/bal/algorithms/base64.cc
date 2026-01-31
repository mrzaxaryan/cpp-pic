#include "bal/algorithms/base64.h"

#define BASE64_PAD '='                                                // Padding character for Base64
#define BASE64_ENCODE_OUT_SIZE(s) ((UINT32)((((s) + 2) / 3) * 4 + 1)) // Calculate the output size for Base64 encoding
#define BASE64_DECODE_OUT_SIZE(s) ((UINT32)(((s) >> 2) * 3))          // Calculate the output size for Base64 decoding (s/4 = s>>2)

// Macro to get the encoded character for a given 6-bit value
#define BASE64_ENCODED_CHAR(v) ( \
    'A' + (((UINT32)(v)) & 63u) + ((((UINT32)(v)) & 63u) >= 26u) * ('a' - 'A' - 26) + ((((UINT32)(v)) & 63u) >= 52u) * ('0' - 'a' - 26) + ((((UINT32)(v)) & 63u) >= 62u) * ('+' - '0' - 10) + ((((UINT32)(v)) & 63u) >= 63u) * ('/' - '+'))

// Macro to get the decoded value for a given Base64 character
#define BASE64_DECODE_CHAR(c) (                     \
    ((c) >= 'A' && (c) <= 'Z') * ((c) - 'A') +      \
    ((c) >= 'a' && (c) <= 'z') * ((c) - 'a' + 26) + \
    ((c) >= '0' && (c) <= '9') * ((c) - '0' + 52) + \
    ((c) == '+') * 62 +                             \
    ((c) == '/') * 63)

// Encode input data to Base64 format
BOOL Base64::Encode(PCCHAR input, UINT32 inputSize, PCHAR output)
{
    UINT32 i, o;

    i = 0;
    o = 0;

    /* process full 3-byte blocks */
    while (i + 2u < inputSize)
    {
        UINT32 v =
            ((UINT32)input[i] << 16) |
            ((UINT32)input[i + 1] << 8) |
            ((UINT32)input[i + 2]);

        output[o++] = (CHAR)BASE64_ENCODED_CHAR((v >> 18) & 63u);
        output[o++] = (CHAR)BASE64_ENCODED_CHAR((v >> 12) & 63u);
        output[o++] = (CHAR)BASE64_ENCODED_CHAR((v >> 6) & 63u);
        output[o++] = (CHAR)BASE64_ENCODED_CHAR(v & 63u);

        i += 3u;
    }

    /* tail (1 or 2 bytes) */
    if (i < inputSize)
    {
        UINT32 v = ((UINT32)input[i] << 16);

        output[o++] = (CHAR)BASE64_ENCODED_CHAR((v >> 18) & 63u);

        if (i + 1u < inputSize)
        {
            v |= ((UINT32)input[i + 1] << 8);

            output[o++] = (CHAR)BASE64_ENCODED_CHAR((v >> 12) & 63u);
            output[o++] = (CHAR)BASE64_ENCODED_CHAR((v >> 6) & 63u);
            output[o++] = (CHAR)BASE64_PAD;
        }
        else
        {
            output[o++] = (CHAR)BASE64_ENCODED_CHAR((v >> 12) & 63u);
            output[o++] = (CHAR)BASE64_PAD;
            output[o++] = (CHAR)BASE64_PAD;
        }
    }

    output[o] = 0;
    return TRUE;
}

// Decoder for Base64 formatted data to original format
BOOL Base64::Decode(PCCHAR input, UINT32 inputSize, PCHAR output)
{
    UINT32 i, o;

    i = 0;
    o = 0;
    // Proces full 4-character blocks
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

        v = (a << 18) | (b << 12) | (c << 6) | d;

        output[o++] = (UINT8)((v >> 16) & 0xFF);

        if (input[i + 2] != BASE64_PAD)
            output[o++] = (UINT8)((v >> 8) & 0xFF);

        if (input[i + 3] != BASE64_PAD)
            output[o++] = (UINT8)(v & 0xFF);

        i += 4u;
    }

    return TRUE;
}

// Calculate the output size needed for Base64 encoding
UINT32 Base64::GetEncodeOutSize(UINT32 inputSize)
{
    return BASE64_ENCODE_OUT_SIZE(inputSize);
}

// Calculate the output size needed for Base64 decoding
UINT32 Base64::GetDecodeOutSize(UINT32 inputSize)
{
    return BASE64_DECODE_OUT_SIZE(inputSize);
}
