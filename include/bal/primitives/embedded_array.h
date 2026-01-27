#pragma once

#include "primitives.h"

/**
 * EMBEDDED_ARRAY - Position-independent compile-time array data embedding
 *
 * Eliminates .rdata section usage by storing array elements as packed integer words.
 * Essential for embedding lookup tables, binary data, and constant arrays in position-
 * independent code without data section dependencies.
 *
 * COMMON USE CASES:
 *   - Shellcode & Position-Independent Code (PIC): Eliminates .rdata relocations
 *   - Kernel-Mode Drivers: Satisfies strict non-paged memory requirements
 *   - Lookup Tables: Embed constant arrays (hashes, opcodes, magic bytes)
 *   - Binary Data: Store small binary blobs without file resources
 *   - OS Development: Embedded systems and microkernels without data sections
 */

// ============================================================================
// TYPE SIZE MAPPING UTILITIES
// ============================================================================

/**
 * UINT_OF_SIZE - Maps byte sizes to corresponding unsigned integer types
 *
 * Used for type-safe element packing/unpacking during compile-time array
 * initialization and runtime element access.
 */
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
class EMBEDDED_ARRAY
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
    consteval EMBEDDED_ARRAY(const TChar (&src)[N]) : words{}
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

// ============================================================================
// HELPER FUNCTION
// ============================================================================

/**
 * MakeEmbedArray - Deduction helper for compile-time array embedding
 *
 * Usage:
 *   constexpr UINT32 data[] = {0x12345678, 0xABCDEF00};
 *   auto embedded = MakeEmbedArray(data);
 *
 * Automatically deduces element type and array size from the source array.
 */
template <typename TElement, USIZE N>
consteval auto MakeEmbedArray(const TElement (&arr)[N]) noexcept
{
    return EMBEDDED_ARRAY<TElement, N>(arr);
}
