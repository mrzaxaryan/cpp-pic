#pragma once

#include "primitives.h"

/**
 * EMBEDDED_STRING - Position-independent compile-time string literal embedding
 *
 * Eliminates .rdata section usage by materializing string literals directly in code.
 * Essential for shellcode, injection payloads, and strict PIC environments.
 *
 * Characters are packed into UINT64 words (8 chars or 4 wchars per word) at compile
 * time and written as immediate values, reducing instruction count by up to 8x
 * compared to character-by-character writes.
 */

// ============================================================================
// INDEX SEQUENCE (Binary-split for O(log n) template depth)
// ============================================================================

template <USIZE... Is>
struct IndexSeq
{
};

template <typename, typename>
struct ConcatSeq;

template <USIZE... Is1, USIZE... Is2>
struct ConcatSeq<IndexSeq<Is1...>, IndexSeq<Is2...>>
{
    using type = IndexSeq<Is1..., (sizeof...(Is1) + Is2)...>;
};

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

template <USIZE N>
using MakeIndexSeq = typename MakeIndexSeqImpl<N>::type;

// ============================================================================
// CHARACTER TYPE CONSTRAINT
// ============================================================================

template <typename TChar>
concept TCHAR = __is_same_as(TChar, CHAR) || __is_same_as(TChar, WCHAR);

// ============================================================================
// EMBEDDED_STRING CLASS
// ============================================================================

template <TCHAR TChar, TChar... Cs>
class EMBEDDED_STRING
{
private:
    static constexpr USIZE N = sizeof...(Cs) + 1;
    static constexpr USIZE CharsPerWord = sizeof(UINT64) / sizeof(TChar);
    static constexpr USIZE NumWords = (N + CharsPerWord - 1) / CharsPerWord;
    static constexpr USIZE AllocN = NumWords * CharsPerWord;

    alignas(UINT64) TChar data[AllocN];

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

    template <USIZE... Is>
    NOINLINE DISABLE_OPTIMIZATION void WritePackedWords(IndexSeq<Is...>) noexcept
    {
        UINT64 *dst = reinterpret_cast<UINT64 *>(data);
        ((dst[Is] = GetPackedWord<Is>()), ...);
    }

public:
    static constexpr USIZE Length() noexcept { return N - 1; }

    NOINLINE DISABLE_OPTIMIZATION EMBEDDED_STRING() noexcept : data{}
    {
        WritePackedWords(MakeIndexSeq<NumWords>{});
    }

    constexpr operator const TChar *() const noexcept { return data; }

    constexpr const TChar &operator[](USIZE index) const noexcept { return data[index]; }
};

// ============================================================================
// USER-DEFINED LITERAL OPERATOR
// ============================================================================

template <TCHAR TChar, TChar... Chars>
FORCE_INLINE auto operator""_embed() noexcept
{
    return EMBEDDED_STRING<TChar, Chars...>{};
}
