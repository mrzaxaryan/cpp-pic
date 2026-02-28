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
 * Span<UINT8> sub = writable.Subspan(4, 16);  // View of bytes [4..20)
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
	template <typename U, USIZE N>
		requires(__is_same_as(T, const U) && N != DYNAMIC_EXTENT)
	constexpr FORCE_INLINE Span(const Span<U, N> &other) : m_data(other.Data()), m_size(N) {}

	constexpr FORCE_INLINE T *Data() const { return m_data; }
	constexpr FORCE_INLINE USIZE Size() const { return m_size; }
	constexpr FORCE_INLINE USIZE SizeBytes() const { return m_size * sizeof(T); }
	constexpr FORCE_INLINE BOOL IsEmpty() const { return m_size == 0; }

	constexpr FORCE_INLINE T &operator[](USIZE index) const { return m_data[index]; }

	constexpr FORCE_INLINE Span Subspan(USIZE offset) const { return Span(m_data + offset, m_size - offset); }
	constexpr FORCE_INLINE Span Subspan(USIZE offset, USIZE count) const { return Span(m_data + offset, count); }
	constexpr FORCE_INLINE Span First(USIZE count) const { return Span(m_data, count); }
	constexpr FORCE_INLINE Span Last(USIZE count) const { return Span(m_data + m_size - count, count); }

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
 * @details Stores ONLY a pointer -- the size is a compile-time constant.
 * This eliminates the m_size member, halving the Span's footprint and
 * enabling the compiler to propagate the known size through optimizations.
 *
 * Implicitly converts to Span<T> (dynamic extent) when passed to functions.
 * Slicing methods return Span<T> (dynamic) since the resulting size is
 * generally not known at compile time.
 *
 * @par Example Usage:
 * @code
 * UINT8 digest[32];
 * Span<UINT8, 32> fixed(digest);          // Only stores pointer, Size() == 32
 * Span<UINT8> dynamic = fixed;            // Implicit static-to-dynamic conversion
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

	// Span<U, N> -> Span<const U, N> implicit conversion
	template <typename U>
		requires(__is_same_as(T, const U))
	constexpr FORCE_INLINE Span(const Span<U, Extent> &other) : m_data(other.Data()) {}

	constexpr FORCE_INLINE T *Data() const { return m_data; }
	constexpr FORCE_INLINE USIZE Size() const { return Extent; }
	constexpr FORCE_INLINE USIZE SizeBytes() const { return Extent * sizeof(T); }
	constexpr FORCE_INLINE BOOL IsEmpty() const { return Extent == 0; }

	constexpr FORCE_INLINE T &operator[](USIZE index) const { return m_data[index]; }

	// Slicing returns dynamic extent
	constexpr FORCE_INLINE Span<T> Subspan(USIZE offset) const { return Span<T>(m_data + offset, Extent - offset); }
	constexpr FORCE_INLINE Span<T> Subspan(USIZE offset, USIZE count) const { return Span<T>(m_data + offset, count); }
	constexpr FORCE_INLINE Span<T> First(USIZE count) const { return Span<T>(m_data, count); }
	constexpr FORCE_INLINE Span<T> Last(USIZE count) const { return Span<T>(m_data + Extent - count, count); }

	constexpr FORCE_INLINE T *begin() const { return m_data; }
	constexpr FORCE_INLINE T *end() const { return m_data + Extent; }

	// Stack-only
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }
};
