#pragma once

#include "primitives.h"

template <USIZE Bytes>
struct UINT_OF_SIZE;
template <>
struct UINT_OF_SIZE<1>
{
    using type = UINT8;
};
template <>
struct UINT_OF_SIZE<2>
{
    using type = UINT16;
};
template <>
struct UINT_OF_SIZE<4>
{
    using type = UINT32;
};
template <>
struct UINT_OF_SIZE<8>
{
    using type = UINT64;
};

template <typename TChar, USIZE N>
class EMBED_ARRAY
{
public:
    static constexpr USIZE Count = N;
    static constexpr USIZE SizeBytes = N * sizeof(TChar);

private:
    static constexpr USIZE WordBytes = sizeof(USIZE);
    static constexpr USIZE WordCount = (SizeBytes + WordBytes - 1) / WordBytes;

    alignas(USIZE) USIZE words[WordCount]{};

    consteval VOID SetByte(USIZE byteIndex, UINT8 v)
    {
        const USIZE wi = byteIndex / WordBytes;
        const USIZE sh = (byteIndex % WordBytes) * 8u;

        const USIZE mask = (USIZE)0xFFu << sh;
        words[wi] = (words[wi] & ~mask) | ((USIZE)v << sh);
    }

    UINT8 GetByte(USIZE byteIndex) const
    {
        const USIZE wi = byteIndex / WordBytes;
        const USIZE sh = (byteIndex % WordBytes) * 8u;
        return (UINT8)((words[wi] >> sh) & (USIZE)0xFFu);
    }

public:
    consteval EMBED_ARRAY(const TChar (&src)[N]) : words{}
    {
        using U = typename UINT_OF_SIZE<sizeof(TChar)>::type;

        for (USIZE i = 0; i < N; ++i)
        {
            const U v = (U)src[i];

            for (USIZE b = 0; b < sizeof(TChar); ++b)
            {
                const UINT8 data = (UINT8)((v >> (int)(b * 8u)) & (U)0xFFu);
                SetByte(i * sizeof(TChar) + b, data);
            }
        }
    }

    TChar operator[](USIZE index) const
    {
        using U = typename UINT_OF_SIZE<sizeof(TChar)>::type;

        U v = 0;
        const USIZE base = index * sizeof(TChar);

        for (USIZE b = 0; b < sizeof(TChar); ++b)
            v |= static_cast<U>(static_cast<unsigned long long>(GetByte(base + b)) << (b * 8u));

        return (TChar)v;
    }
    constexpr operator const VOID *() const
    {
        return (const VOID *)words;
    }
    constexpr const USIZE *Words() const { return words; }
    static constexpr USIZE WordsCount = WordCount;
};

template <typename TChar, USIZE N>
consteval auto MakeEmbedArray(const TChar (&arr)[N])
{
    return EMBED_ARRAY<TChar, N>(arr);
}
