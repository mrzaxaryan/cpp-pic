/**
 * @file embedded_array.h
 * @brief Position-Independent Compile-Time Array Data Embedding
 *
 * @details Eliminates .rdata section usage by storing array elements as packed
 * integer words. Essential for embedding lookup tables, binary data, and constant
 * arrays in position-independent code without data section dependencies.
 *
 * Common Use Cases:
 * - Shellcode & Position-Independent Code (PIC): Eliminates .rdata relocations
 * - Kernel-Mode Drivers: Satisfies strict non-paged memory requirements
 * - Lookup Tables: Embed constant arrays (hashes, opcodes, magic bytes)
 * - Binary Data: Store small binary blobs without file resources
 * - OS Development: Embedded systems and microkernels without data sections
 *
 * @ingroup core
 *
 * @defgroup embedded_array Embedded Array
 * @ingroup core
 * @{
 */

#pragma once

#include "primitives.h"

// =============================================================================
// TYPE SIZE MAPPING UTILITIES
// =============================================================================

/**
 * @struct UINT_OF_SIZE
 * @brief Maps byte sizes to corresponding unsigned integer types
 *
 * @tparam Bytes Number of bytes
 *
 * @details Used for type-safe element packing/unpacking during compile-time
 * array initialization and runtime element access.
 *
 * Specializations:
 * - 1 byte: UINT8
 * - 2 bytes: UINT16
 * - 4 bytes: UINT32
 * - 8 bytes: UINT64
 */
template <USIZE Bytes>
struct UINT_OF_SIZE;

template <>
struct UINT_OF_SIZE<1>
{
	using Type =UINT8;
};
template <>
struct UINT_OF_SIZE<2>
{
	using Type =UINT16;
};
template <>
struct UINT_OF_SIZE<4>
{
	using Type =UINT32;
};
template <>
struct UINT_OF_SIZE<8>
{
	using Type =UINT64;
};

/**
 * @class EMBEDDED_ARRAY
 * @brief Position-independent array that embeds elements as packed integer words
 *
 * @tparam TChar Element type
 * @tparam N Number of elements
 *
 * @details Array elements are packed into machine-word-sized integers at compile
 * time and stored in the .text section. Element access unpacks bytes at runtime.
 *
 * @par Memory Layout:
 * Elements are packed byte-by-byte into USIZE words for efficient storage.
 * Access operations unpack the bytes to reconstruct the original element.
 *
 * @par Example Usage:
 * @code
 * constexpr UINT32 lookup[] = {0x12345678, 0xABCDEF00, 0x11223344};
 * auto embedded = MakeEmbedArray(lookup);
 * UINT32 value = embedded[1];  // Access element (unpacked at runtime)
 * @endcode
 */
template <typename TChar, USIZE N>
class EMBEDDED_ARRAY
{
public:
	static constexpr USIZE Count = N;                    ///< Number of elements
	static constexpr USIZE SizeBytes = N * sizeof(TChar);  ///< Total size in bytes

private:
	static constexpr USIZE WordBytes = sizeof(USIZE);    ///< Bytes per word
	static constexpr USIZE WordCount = (SizeBytes + WordBytes - 1) / WordBytes;  ///< Number of words

	alignas(USIZE) USIZE words[WordCount]{};             ///< Packed word storage

	/**
	 * @brief Sets a byte in the packed word storage
	 * @param byteIndex Index of byte to set
	 * @param v Byte value
	 */
	consteval VOID SetByte(USIZE byteIndex, UINT8 v)
	{
		const USIZE wi = byteIndex / WordBytes;
		const USIZE sh = (byteIndex % WordBytes) * 8u;

		const USIZE mask = (USIZE)0xFFu << sh;
		words[wi] = (words[wi] & ~mask) | ((USIZE)v << sh);
	}

	/**
	 * @brief Gets a byte from the packed word storage
	 * @param byteIndex Index of byte to get
	 * @return Byte value
	 *
	 * @details The register barrier prevents the compiler from vectorizing
	 * the byte-unpacking loop, which would emit SIMD constants into
	 * __TEXT,__literal16 or __TEXT,__const sections at -O2/-O3/-Os.
	 */
	UINT8 GetByte(USIZE byteIndex) const
	{
		const USIZE wi = byteIndex / WordBytes;
		const USIZE sh = (byteIndex % WordBytes) * 8u;
		USIZE word = words[wi];
		__asm__ volatile("" : "+r"(word));
		return (UINT8)((word >> sh) & (USIZE)0xFFu);
	}

public:
	/**
	 * @brief Compile-time constructor that packs array elements into words
	 * @param src Source array to embed
	 *
	 * @details Called at compile time (consteval) to pack array elements
	 * byte-by-byte into machine words.
	 */
	consteval EMBEDDED_ARRAY(const TChar (&src)[N]) : words{}
	{
		using U = typename UINT_OF_SIZE<sizeof(TChar)>::Type;

		for (USIZE i = 0; i < N; ++i)
		{
			const U v = (U)src[i];

			for (USIZE b = 0; b < sizeof(TChar); ++b)
			{
				const UINT8 data = (UINT8)((v >> (INT32)(b * 8u)) & (U)0xFFu);
				SetByte(i * sizeof(TChar) + b, data);
			}
		}
	}

	/**
	 * @brief Destructor that prevents the compiler from reusing the stack
	 * storage after conversion to a raw pointer. The "+m" constraint tells
	 * the compiler the asm both reads and modifies the words array.
	 */
	FORCE_INLINE ~EMBEDDED_ARRAY() noexcept
	{
		__asm__ volatile("" : "+m"(*(USIZE(*)[WordCount])words));
	}

	/**
	 * @brief Array subscript operator - unpacks element at runtime
	 * @param index Element index
	 * @return Element value (unpacked from words)
	 */
	TChar operator[](USIZE index) const
	{
		using U = typename UINT_OF_SIZE<sizeof(TChar)>::Type;

		U v = 0;
		const USIZE base = index * sizeof(TChar);

		for (USIZE b = 0; b < sizeof(TChar); ++b)
		{
			U byte_val = static_cast<U>(GetByte(base + b));
			U shifted = byte_val << (INT32)(b * 8u);
			v |= shifted;
		}

		return (TChar)v;
	}

	/**
	 * @brief Implicit conversion to const void pointer
	 * @return Pointer to raw word storage
	 */
	operator const VOID *() const
	{
		return (const VOID *)words;
	}

	/**
	 * @brief Access raw word storage
	 * @return Pointer to word array
	 */
	const USIZE *Words() const { return words; }

	static constexpr USIZE WordsCount = WordCount;  ///< Number of words in storage
};

// =============================================================================
// HELPER FUNCTION
// =============================================================================

/**
 * @brief Deduction helper for compile-time array embedding
 *
 * @tparam TElement Element type (deduced)
 * @tparam N Array size (deduced)
 * @param arr Source array to embed
 * @return EMBEDDED_ARRAY instance
 *
 * @details Automatically deduces element type and array size from the source array.
 *
 * @par Example:
 * @code
 * constexpr UINT32 data[] = {0x12345678, 0xABCDEF00};
 * auto embedded = MakeEmbedArray(data);
 * @endcode
 */
template <typename TElement, USIZE N>
consteval auto MakeEmbedArray(const TElement (&arr)[N]) noexcept
{
	return EMBEDDED_ARRAY<TElement, N>(arr);
}

/** @} */ // end of embedded_array group
