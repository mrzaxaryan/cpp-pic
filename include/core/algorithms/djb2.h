/**
 * @file djb2.h
 * @brief DJB2 Hash Algorithm Implementation
 *
 * @details Implements the DJB2 hash algorithm by Daniel J. Bernstein.
 * This is a fast, simple hash function commonly used for string hashing,
 * hash tables, and API name hashing in position-independent code.
 *
 * Key features:
 * - Compile-time seeding using build date for anti-analysis
 * - Case-insensitive hashing for API name resolution
 * - Both runtime and compile-time hash computation
 * - Support for narrow (CHAR) and wide (WCHAR) strings
 *
 * @note The seed is derived from __DATE__ at compile time, making hash values
 * unique per build. This helps evade signature-based detection.
 *
 * @ingroup core
 *
 * @defgroup djb2 DJB2 Hash Algorithm
 * @ingroup core
 * @{
 */

#pragma once
#include "primitives.h"
#include "string.h"

/**
 * @brief Compile-time FNV-1a hash for seed generation
 * @param s Null-terminated string to hash
 * @return Hash value computed at compile time
 *
 * @details Uses FNV-1a algorithm to generate a unique seed from the build date.
 * This function must be consteval to ensure compile-time evaluation.
 */
consteval USIZE ct_hash_str_seed(const CHAR *s)
{
    USIZE h = (USIZE)2166136261u;
    for (USIZE i = 0; s[i] != '\0'; ++i)
        h = (h ^ (USIZE)(UINT8)s[i]) * (USIZE)16777619u;
    return h;
}

/**
 * @class Djb2
 * @brief DJB2 hash algorithm implementation for string hashing
 *
 * @details Provides both runtime and compile-time string hashing using the DJB2
 * algorithm. The hash is case-insensitive, making it ideal for Windows API
 * name resolution where function names are case-insensitive.
 *
 * @par Example Usage:
 * @code
 * // Runtime hash computation
 * USIZE hash = Djb2::Hash("LoadLibraryA");
 *
 * // Compile-time hash computation (for constant expressions)
 * constexpr USIZE LOADLIBRARY_HASH = Djb2::HashCompileTime("LoadLibraryA");
 *
 * // Compare runtime hash with compile-time hash
 * if (Djb2::Hash(exportName) == LOADLIBRARY_HASH) {
 *     // Found LoadLibraryA
 * }
 * @endcode
 */
class Djb2
{
private:
    /**
     * @brief Compile-time seed derived from build date
     * @details Unique per build, provides anti-signature protection.
     */
    static constexpr USIZE Seed = ct_hash_str_seed(__DATE__);

public:
    /**
     * @brief Computes DJB2 hash at runtime
     * @tparam TChar Character type (CHAR or WCHAR)
     * @param value Null-terminated string to hash
     * @return Hash value
     *
     * @details Case-insensitive hash computation. Each character is converted
     * to lowercase before hashing. The algorithm uses the formula:
     * hash = ((hash << 5) + hash) + c, which is equivalent to hash * 33 + c.
     */
    template <typename TChar>
    static USIZE Hash(const TChar *value)
    {
        USIZE h = Seed;
        for (USIZE i = 0; value[i] != (TChar)0; ++i)
        {
            TChar c = String::ToLowerCase(value[i]);
            h = ((h << 5) + h) + (USIZE)c;
        }
        return h;
    }

    /**
     * @brief Computes DJB2 hash at compile time
     * @tparam TChar Character type (CHAR or WCHAR)
     * @tparam N Array size (deduced automatically)
     * @param value String literal to hash
     * @return Hash value computed at compile time
     *
     * @details Enables compile-time hash computation for string literals.
     * The result can be used in switch statements, constexpr variables,
     * and template parameters.
     *
     * @note Must be called with a string literal or constexpr array.
     */
    template <typename TChar, USIZE N>
    static consteval USIZE HashCompileTime(const TChar (&value)[N])
    {
        USIZE h = Seed;
        for (USIZE i = 0; value[i] != (TChar)0; ++i)
        {
            TChar c = String::ToLowerCase(value[i]);
            h = ((h << 5) + h) + (USIZE)c;
        }
        return h;
    }
};

/** @} */ // end of djb2 group
