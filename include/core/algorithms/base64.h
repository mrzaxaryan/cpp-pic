/**
 * @file base64.h
 * @brief Base64 Encoding and Decoding
 *
 * @details Platform-independent Base64 encoding and decoding utilities.
 * Implements RFC 4648 Base64 encoding scheme for binary-to-text conversion.
 *
 * Base64 is commonly used for:
 * - Encoding binary data in text-based protocols (HTTP, SMTP)
 * - Embedding binary data in JSON/XML
 * - Data URLs
 * - Cryptographic operations requiring text output
 *
 * @note Implementation uses embedded lookup tables to avoid .rdata dependencies.
 *
 * @ingroup core
 *
 * @defgroup base64 Base64 Encoding
 * @ingroup core
 * @{
 */

#pragma once

#include "primitives.h"

/**
 * @class Base64
 * @brief Static class for Base64 encoding and decoding operations
 *
 * @details Provides position-independent Base64 encoding/decoding without
 * CRT dependencies. The encoding alphabet uses standard Base64 characters
 * (A-Z, a-z, 0-9, +, /) with '=' for padding.
 *
 * @par Example Usage:
 * @code
 * CHAR input[] = "Hello, World!";
 * UINT32 encSize = Base64::GetEncodeOutSize(sizeof(input) - 1);
 * CHAR encoded[64];
 * Base64::Encode(input, sizeof(input) - 1, encoded);
 * // encoded = "SGVsbG8sIFdvcmxkIQ=="
 *
 * CHAR decoded[64];
 * Base64::Decode(encoded, encSize, decoded);
 * // decoded = "Hello, World!"
 * @endcode
 */
class Base64
{
public:
    /**
     * @brief Encodes binary data to Base64 format
     * @param input Pointer to input binary data
     * @param inputSize Size of input data in bytes
     * @param output Pointer to output buffer (must be at least GetEncodeOutSize() bytes)
     * @return TRUE on success, FALSE on failure
     *
     * @note Output buffer must be large enough to hold the encoded data.
     * Use GetEncodeOutSize() to determine required buffer size.
     */
    static BOOL Encode(PCCHAR input, UINT32 inputSize, PCHAR output);

    /**
     * @brief Decodes Base64 formatted data back to binary
     * @param input Pointer to Base64 encoded string
     * @param inputSize Size of input string in bytes (including padding)
     * @param output Pointer to output buffer (must be at least GetDecodeOutSize() bytes)
     * @return TRUE on success, FALSE on failure (invalid Base64 input)
     *
     * @note Output buffer must be large enough to hold the decoded data.
     * Use GetDecodeOutSize() to determine required buffer size.
     */
    static BOOL Decode(PCCHAR input, UINT32 inputSize, PCHAR output);

    /**
     * @brief Calculates required output buffer size for encoding
     * @param inputSize Size of input data in bytes
     * @return Required output buffer size in bytes (including null terminator)
     *
     * @details Base64 encoding expands data by 4/3 ratio, rounded up to
     * the nearest multiple of 4 for padding.
     */
    static UINT32 GetEncodeOutSize(UINT32 inputSize);

    /**
     * @brief Calculates required output buffer size for decoding
     * @param inputSize Size of Base64 input string in bytes
     * @return Required output buffer size in bytes
     *
     * @details Base64 decoding contracts data by 3/4 ratio.
     */
    static UINT32 GetDecodeOutSize(UINT32 inputSize);
};

/** @} */ // end of base64 group
