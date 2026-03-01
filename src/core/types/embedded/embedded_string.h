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
 * Characters are packed into machine words (sizeof(USIZE)/sizeof(TChar) chars per
 * word) at compile time and written as immediate values, reducing instruction count
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
 * Characters are packed into USIZE (machine-word) words:
 * - CHAR: sizeof(USIZE) characters per word
 * - WCHAR: sizeof(USIZE)/sizeof(WCHAR) characters per word
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
	static constexpr USIZE CharsPerWord = sizeof(USIZE) / sizeof(TChar);  ///< Characters per machine word
	static constexpr USIZE NumWords = (N + CharsPerWord - 1) / CharsPerWord;  ///< Number of words needed
	static constexpr USIZE AllocN = NumWords * CharsPerWord;  ///< Allocated character count

	alignas(USIZE) TChar data[AllocN];  ///< String storage aligned for word access

	/**
	 * @brief Packs characters into a machine word at compile time
	 * @tparam WordIndex Index of word to pack
	 * @return Packed USIZE word containing CharsPerWord characters
	 */
	template <USIZE WordIndex>
	static consteval USIZE GetPackedWord() noexcept
	{
		constexpr TChar chars[N] = {Cs..., TChar(0)};
		USIZE result = 0;
		constexpr USIZE base = WordIndex * CharsPerWord;
		constexpr USIZE shift = sizeof(TChar) * 8;

		for (USIZE i = 0; i < CharsPerWord; ++i)
		{
			USIZE idx = base + i;
			TChar c = (idx < N) ? chars[idx] : TChar(0);
			constexpr USIZE charMask = (sizeof(TChar) >= sizeof(USIZE)) ? ~(USIZE)0 : (((USIZE)1 << (sizeof(TChar) * 8)) - 1);
			result |= (static_cast<USIZE>(c) & charMask) << (i * shift);
		}
		return result;
	}

	/**
	 * @brief Recursively writes packed words to the data array
	 * @tparam I Current word index (starts at 0)
	 * @details Uses if constexpr recursion to write all words as immediate values.
	 * The "+r" asm barrier per word forces each constant into a register as an
	 * immediate operand and prevents LLVM from coalescing stores into .rdata + memcpy.
	 */
	template <USIZE I = 0>
	FORCE_INLINE void WritePackedWord() noexcept
	{
		if constexpr (I < NumWords)
		{
			USIZE word = GetPackedWord<I>();
			__asm__ volatile("" : "+r"(word));
			reinterpret_cast<USIZE *>(data)[I] = word;
			WritePackedWord<I + 1>();
		}
	}

public:
	/**
	 * @brief Returns the string length (excluding null terminator)
	 * @return Number of characters in the string
	 */
	static constexpr USIZE Length() noexcept { return N - 1; }

	/**
	 * @brief Constructor that materializes the string on the stack
	 * @details FORCE_INLINE eliminates per-string function overhead. Each packed
	 * word is laundered through a register via an asm barrier in WritePackedWord(),
	 * preventing LLVM from coalescing stores into a .rdata constant + memcpy.
	 * Zero-initialization is omitted because WritePackedWord() writes every byte
	 * (padding positions are already zero in the packed words).
	 */
	FORCE_INLINE EMBEDDED_STRING() noexcept
	{
		WritePackedWord();
	}

	/**
	 * @brief Destructor that prevents the compiler from reusing the stack storage
	 * @details At -O1+ the compiler may determine the object is "dead" after
	 * conversion to a raw pointer, and reuse its stack slot for other temporaries.
	 * The "+m" (read+write) constraint tells the compiler the asm both reads and
	 * modifies the data array, making it impossible to reuse the stack slot or
	 * reorder stores into it before the destructor runs at end of full-expression.
	 *
	 * The operand type must be TChar(*)[AllocN] to match the actual data type,
	 * ensuring the constraint and caller reads are in the same TBAA alias set.
	 */
	FORCE_INLINE ~EMBEDDED_STRING() noexcept
	{
		__asm__ volatile("" : "+m"(*(TChar (*)[AllocN])data));
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
