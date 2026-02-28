#pragma once

#include "primitives.h"

/// Sentinel value indicating a runtime-determined extent
constexpr USIZE DYNAMIC_EXTENT = static_cast<USIZE>(-1);

// Forward declaration
template <typename T, USIZE Extent = DYNAMIC_EXTENT>
class Span;

// =============================================================================
// PARTIAL SPECIALIZATION: Dynamic Extent (Span<T> / Span<T, DYNAMIC_EXTENT>)
// =============================================================================

/**
 * @brief Non-owning view over a contiguous sequence of elements (dynamic extent)
 *
 * @tparam T Element type (can be const-qualified for read-only views)
 *
 * @details Stores a pointer and a runtime size. This is the default form
 * used in all function signatures throughout the codebase.
 *
 * Span<T> implicitly converts to Span<const T>, enabling functions that accept
 * Span<const T> to receive Span<T> arguments without explicit casting.
 *
 * Span<T, N> (static extent) implicitly converts to Span<T> (dynamic extent).
 *
 * @par Example Usage:
 * @code
 * UINT8 buffer[64];
 * Span<UINT8> writable(buffer);           // From array (size deduced)
 * Span<const UINT8> readable = writable;  // Implicit const conversion
 *
 * Span<UINT8> sub = writable.Subspan(4, 16);      // Runtime slicing
 * Span<UINT8, 16> fixed = writable.Subspan<4, 16>(); // Compile-time slicing
 * @endcode
 */
template <typename T>
class Span<T, DYNAMIC_EXTENT>
{
private:
	T *m_data;
	USIZE m_size;

public:
	constexpr FORCE_INLINE Span() : m_data(nullptr), m_size(0) {}

	constexpr FORCE_INLINE Span(T *data, USIZE size) : m_data(data), m_size(size) {}

	template <USIZE N>
	constexpr FORCE_INLINE Span(T (&arr)[N]) : m_data(arr), m_size(N) {}

	// Span<U> -> Span<const U> implicit conversion (dynamic to dynamic)
	template <typename U>
		requires(__is_same_as(T, const U))
	constexpr FORCE_INLINE Span(const Span<U> &other) : m_data(other.Data()), m_size(other.Size()) {}

	// Span<T, N> -> Span<T> implicit conversion (static to dynamic)
	template <typename U, USIZE N>
		requires(__is_same_as(T, U) && N != DYNAMIC_EXTENT)
	constexpr FORCE_INLINE Span(const Span<U, N> &other) : m_data(other.Data()), m_size(N) {}

	// Span<U, N> -> Span<const U> implicit conversion (static to dynamic + const)
	// !__is_const(U) prevents ambiguity when U is already const (line 62 handles that case)
	template <typename U, USIZE N>
		requires(__is_same_as(T, const U) && !__is_const(U) && N != DYNAMIC_EXTENT)
	constexpr FORCE_INLINE Span(const Span<U, N> &other) : m_data(other.Data()), m_size(N) {}

	constexpr FORCE_INLINE T *Data() const { return m_data; }
	constexpr FORCE_INLINE USIZE Size() const { return m_size; }
	constexpr FORCE_INLINE USIZE SizeBytes() const { return m_size * sizeof(T); }
	constexpr FORCE_INLINE BOOL IsEmpty() const { return m_size == 0; }

	constexpr FORCE_INLINE T &operator[](USIZE index) const { return m_data[index]; }

	// Runtime slicing — extent unknown at compile time
	constexpr FORCE_INLINE Span Subspan(USIZE offset) const { return Span(m_data + offset, m_size - offset); }
	constexpr FORCE_INLINE Span Subspan(USIZE offset, USIZE count) const { return Span(m_data + offset, count); }
	constexpr FORCE_INLINE Span First(USIZE count) const { return Span(m_data, count); }
	constexpr FORCE_INLINE Span Last(USIZE count) const { return Span(m_data + m_size - count, count); }

	// Compile-time slicing — count baked into return type, eliminating m_size in callee
	template <USIZE Count>
	constexpr FORCE_INLINE Span<T, Count> First() const { return Span<T, Count>(m_data); }

	template <USIZE Count>
	constexpr FORCE_INLINE Span<T, Count> Last() const { return Span<T, Count>(m_data + m_size - Count); }

	template <USIZE Offset, USIZE Count>
	constexpr FORCE_INLINE Span<T, Count> Subspan() const { return Span<T, Count>(m_data + Offset); }

	constexpr FORCE_INLINE T *begin() const { return m_data; }
	constexpr FORCE_INLINE T *end() const { return m_data + m_size; }

	// Stack-only
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }
};

// =============================================================================
// PRIMARY TEMPLATE: Static Extent (Span<T, N>)
// =============================================================================

/**
 * @brief Non-owning view over a contiguous sequence of elements (static extent)
 *
 * @tparam T Element type
 * @tparam Extent Compile-time element count
 *
 * @details Stores ONLY a pointer — the size is a compile-time constant baked
 * into the type. This eliminates the m_size member entirely, halving the
 * Span's footprint and enabling the compiler to propagate the known size
 * through all downstream optimizations (loop unrolling, dead-store elim, etc.).
 *
 * Implicitly converts to Span<T> (dynamic extent) when passed to functions.
 *
 * Compile-time slicing (First<N>(), Last<N>(), Subspan<O,N>(), Subspan<O>())
 * returns Span<T, N> so the static extent is preserved through the chain,
 * preventing unnecessary size loads/stores in the generated code.
 *
 * @par Example Usage:
 * @code
 * UINT8 digest[32];
 * Span<UINT8, 32> fixed(digest);           // Pointer only — Size() == 32 is free
 * Span<UINT8, 16> lo = fixed.First<16>();  // Static slice — no m_size at all
 * Span<UINT8, 16> hi = fixed.Last<16>();
 * Span<UINT8, 8>  mid = fixed.Subspan<4, 8>();
 * Span<UINT8, 28> tail = fixed.Subspan<4>(); // Count = 32-4 deduced from type
 * Span<UINT8> dynamic = fixed;             // Implicit static-to-dynamic conversion
 * @endcode
 */
template <typename T, USIZE Extent>
class Span
{
private:
	T *m_data;

public:
	constexpr FORCE_INLINE Span() : m_data(nullptr) {}

	constexpr FORCE_INLINE Span(T (&arr)[Extent]) : m_data(arr) {}

	/// Construct from a raw pointer when the extent is known at compile time.
	/// Marked explicit to prevent accidental implicit construction from T*.
	/// Template parameter makes this a function template so the non-template
	/// array-reference constructor above wins when the argument is an array,
	/// while this constructor wins when the argument is a plain pointer.
	template <typename = void>
	constexpr FORCE_INLINE explicit Span(T *ptr) : m_data(ptr) {}

	// Span<U, N> -> Span<const U, N> implicit conversion
	template <typename U>
		requires(__is_same_as(T, const U))
	constexpr FORCE_INLINE Span(const Span<U, Extent> &other) : m_data(other.Data()) {}

	constexpr FORCE_INLINE T *Data() const { return m_data; }
	constexpr FORCE_INLINE USIZE Size() const { return Extent; }
	constexpr FORCE_INLINE USIZE SizeBytes() const { return Extent * sizeof(T); }
	constexpr FORCE_INLINE BOOL IsEmpty() const { return Extent == 0; }

	constexpr FORCE_INLINE T &operator[](USIZE index) const { return m_data[index]; }

	// Runtime slicing — returns dynamic extent (extent becomes runtime value)
	constexpr FORCE_INLINE Span<T> Subspan(USIZE offset) const { return Span<T>(m_data + offset, Extent - offset); }
	constexpr FORCE_INLINE Span<T> Subspan(USIZE offset, USIZE count) const { return Span<T>(m_data + offset, count); }
	constexpr FORCE_INLINE Span<T> First(USIZE count) const { return Span<T>(m_data, count); }
	constexpr FORCE_INLINE Span<T> Last(USIZE count) const { return Span<T>(m_data + Extent - count, count); }

	// Compile-time slicing — extent baked into return type, zero runtime cost
	template <USIZE Count>
	constexpr FORCE_INLINE Span<T, Count> First() const
	{
		static_assert(Count <= Extent, "First<Count>: Count exceeds static extent");
		return Span<T, Count>(m_data);
	}

	template <USIZE Count>
	constexpr FORCE_INLINE Span<T, Count> Last() const
	{
		static_assert(Count <= Extent, "Last<Count>: Count exceeds static extent");
		return Span<T, Count>(m_data + Extent - Count);
	}

	template <USIZE Offset, USIZE Count>
	constexpr FORCE_INLINE Span<T, Count> Subspan() const
	{
		static_assert(Offset <= Extent, "Subspan<Offset, Count>: Offset exceeds static extent");
		static_assert(Count <= Extent - Offset, "Subspan<Offset, Count>: Count exceeds remaining extent");
		return Span<T, Count>(m_data + Offset);
	}

	/// Subspan from Offset to end; resulting count (Extent - Offset) is deduced from type.
	template <USIZE Offset>
	constexpr FORCE_INLINE Span<T, Extent - Offset> Subspan() const
	{
		static_assert(Offset <= Extent, "Subspan<Offset>: Offset exceeds static extent");
		return Span<T, Extent - Offset>(m_data + Offset);
	}

	constexpr FORCE_INLINE T *begin() const { return m_data; }
	constexpr FORCE_INLINE T *end() const { return m_data + Extent; }

	// Stack-only
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }
};
