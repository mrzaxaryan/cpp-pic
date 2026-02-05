/**
 * @file embedded_string.h
 * @brief Position-Independent Compile-Time String Embedding
 *
 * @details Eliminates .rdata section usage by materializing string literals directly
 * in the code section as immediate values. Essential for:
 * - Shellcode development
 * - Code injection payloads
 * - Position-independent code (PIC)
 * - Environments where .rdata is not accessible
 *
 * Characters are packed into UINT64 words (8 chars or 4 wchars per word) at compile
 * time and written as immediate values, reducing instruction count by up to 8x
 * compared to character-by-character writes.
 *
 * @note Uses C++23 user-defined string literal operator for ergonomic syntax.
 *
 * @ingroup core
 *
 * @defgroup embedded_string Embedded String
 * @ingroup core
 * @{
 */

#pragma once

#include "primitives.h"

// =============================================================================
// INDEX SEQUENCE UTILITIES
// =============================================================================

/**
 * @brief Index sequence type for template parameter pack expansion
 * @tparam Is Variadic index values
 * @details Uses binary-split generation for O(log n) template instantiation depth.
 */
template <USIZE... Is>
struct IndexSeq
{
};

/** @brief Concatenates two index sequences */
template <typename, typename>
struct ConcatSeq;

template <USIZE... Is1, USIZE... Is2>
struct ConcatSeq<IndexSeq<Is1...>, IndexSeq<Is2...>>
{
    using type = IndexSeq<Is1..., (sizeof...(Is1) + Is2)...>;
};

/** @brief Generates index sequence 0, 1, 2, ..., N-1 */
template <USIZE N>
struct MakeIndexSeqImpl
{
    using type = typename ConcatSeq<
        typename MakeIndexSeqImpl<N / 2>::type,
        typename MakeIndexSeqImpl<N - N / 2>::type>::type;
};

template <>
struct MakeIndexSeqImpl<0>
{
    using type = IndexSeq<>;
};

template <>
struct MakeIndexSeqImpl<1>
{
    using type = IndexSeq<0>;
};

/** @brief Alias for index sequence generation */
template <USIZE N>
using MakeIndexSeq = typename MakeIndexSeqImpl<N>::type;

// =============================================================================
// CHARACTER TYPE CONSTRAINT
// =============================================================================

/**
 * @brief Concept constraining character types to CHAR or WCHAR
 * @tparam TChar Type to check
 * @details Ensures embedded strings only work with supported character types.
 */
template <typename TChar>
concept TCHAR = __is_same_as(TChar, CHAR) || __is_same_as(TChar, WCHAR);

// =============================================================================
// EMBEDDED_STRING CLASS
// =============================================================================

/**
 * @class EMBEDDED_STRING
 * @brief Position-independent string that embeds characters as immediate values
 *
 * @tparam TChar Character type (CHAR or WCHAR)
 * @tparam Cs Variadic character values from string literal
 *
 * @details The string is stored on the stack and characters are written as
 * packed 64-bit immediate values in the instruction stream, avoiding any
 * references to .rdata or other data sections.
 *
 * @par Memory Layout:
 * Characters are packed into UINT64 words:
 * - CHAR: 8 characters per word
 * - WCHAR: 4 characters per word (with -fshort-wchar)
 *
 * @par Assembly Output Example (x86_64):
 * @code
 * movabsq $0x6F6C6C6548, (%rsp)  ; "Hello"
 * @endcode
 *
 * @par Example Usage:
 * @code
 * auto msg = "Hello, World!"_embed;
 * Console::Print(msg);  // Use as const CHAR*
 *
 * auto wide = L"Wide string"_embed;
 * // Use as const WCHAR*
 * @endcode
 */
template <TCHAR TChar, TChar... Cs>
class EMBEDDED_STRING
{
private:
    static constexpr USIZE N = sizeof...(Cs) + 1;           ///< String length + null terminator
    static constexpr USIZE CharsPerWord = sizeof(UINT64) / sizeof(TChar);  ///< Characters per 64-bit word
    static constexpr USIZE NumWords = (N + CharsPerWord - 1) / CharsPerWord;  ///< Number of words needed
    static constexpr USIZE AllocN = NumWords * CharsPerWord;  ///< Allocated character count

    alignas(UINT64) TChar data[AllocN];  ///< String storage aligned for word access

    /**
     * @brief Packs characters into a 64-bit word at compile time
     * @tparam WordIndex Index of word to pack
     * @return Packed 64-bit word containing up to 8 characters
     */
    template <USIZE WordIndex>
    static consteval UINT64 GetPackedWord() noexcept
    {
        constexpr TChar chars[N] = {Cs..., TChar(0)};
        UINT64 result = 0;
        constexpr USIZE base = WordIndex * CharsPerWord;
        constexpr USIZE shift = sizeof(TChar) * 8;

        for (USIZE i = 0; i < CharsPerWord; ++i)
        {
            USIZE idx = base + i;
            TChar c = (idx < N) ? chars[idx] : TChar(0);
            result |= static_cast<UINT64>(static_cast<UINT16>(c)) << (i * shift);
        }
        return result;
    }

    /**
     * @brief Writes packed words to the data array
     * @tparam Is Index sequence for word indices
     * @details Uses fold expression to write all words as immediate values.
     */
    template <USIZE... Is>
    NOINLINE DISABLE_OPTIMIZATION void WritePackedWords(IndexSeq<Is...>) noexcept
    {
        UINT64 *dst = reinterpret_cast<UINT64 *>(data);
        ((dst[Is] = GetPackedWord<Is>()), ...);
    }

public:
    /**
     * @brief Returns the string length (excluding null terminator)
     * @return Number of characters in the string
     */
    static constexpr USIZE Length() noexcept { return N - 1; }

    /**
     * @brief Constructor that materializes the string on the stack
     * @details Marked NOINLINE and DISABLE_OPTIMIZATION to prevent the compiler
     * from moving the string data to .rdata.
     */
    NOINLINE DISABLE_OPTIMIZATION EMBEDDED_STRING() noexcept : data{}
    {
        WritePackedWords(MakeIndexSeq<NumWords>{});
    }

    /**
     * @brief Implicit conversion to const character pointer
     * @return Pointer to the string data
     */
    constexpr operator const TChar *() const noexcept { return data; }

    /**
     * @brief Array subscript operator
     * @param index Character index
     * @return Reference to character at index
     */
    constexpr const TChar &operator[](USIZE index) const noexcept { return data[index]; }
};

// =============================================================================
// USER-DEFINED LITERAL OPERATOR
// =============================================================================

/**
 * @brief User-defined literal operator for embedded strings
 * @tparam TChar Character type (CHAR or WCHAR)
 * @tparam Chars Character values from string literal
 * @return EMBEDDED_STRING instance
 *
 * @par Usage:
 * @code
 * auto str = "Hello"_embed;      // Narrow string
 * auto wstr = L"World"_embed;    // Wide string
 * @endcode
 */
template <TCHAR TChar, TChar... Chars>
FORCE_INLINE auto operator""_embed() noexcept
{
    return EMBEDDED_STRING<TChar, Chars...>{};
}

/** @} */ // end of embedded_string group
