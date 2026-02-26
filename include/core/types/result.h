/**
 * @file result.h
 * @brief Zero-Cost Result Type for Error Handling
 *
 * @details Tagged union `Result<T, E>` — either a success value (T) or error (E).
 * When T is void, a trivial sentinel replaces the value member so a single
 * template handles both cases without a separate specialization.
 *
 * E is stored directly — no chain, no overhead beyond sizeof(E).
 * Compile-time safety via `[[nodiscard]]` and `operator BOOL` ensures callers
 * check results without any runtime cost.
 *
 * Backward-compatible multi-arg and propagation Err() overloads are provided
 * for source compatibility; they store only the outermost error code.
 *
 * @note Uses Clang builtin __is_trivially_destructible for zero-overhead destruction.
 *
 * @ingroup core
 *
 * @defgroup result Result Type
 * @ingroup core
 * @{
 */

#pragma once

#include "primitives.h"

/// Placement new for constructing objects in pre-allocated storage
FORCE_INLINE PVOID operator new(USIZE, PVOID ptr) noexcept { return ptr; }

/// Trivial sentinel replacing T when T is void
struct VOID_TAG
{
};

template <typename T>
struct VOID_TO_TAG
{
	using Type = T;
};

template <>
struct VOID_TO_TAG<void>
{
	using Type = VOID_TAG;
};

template <typename T, typename E>
class [[nodiscard]] Result
{
	static constexpr bool IS_VOID = __is_same_as(T, void);
	using STORED_TYPE = typename VOID_TO_TAG<T>::Type;

	union
	{
		STORED_TYPE m_value;
		E m_error;
	};
	BOOL m_isOk;

	void DestroyActive() noexcept
	{
		if constexpr (!__is_trivially_destructible(STORED_TYPE) || !__is_trivially_destructible(E))
		{
			if (m_isOk)
			{
				if constexpr (!__is_trivially_destructible(STORED_TYPE))
					m_value.~STORED_TYPE();
			}
			else
			{
				if constexpr (!__is_trivially_destructible(E))
					m_error.~E();
			}
		}
	}

public:
	using ValueType = T;
	using ErrorType = E;

	// =====================================================================
	// Ok factories
	// =====================================================================

	[[nodiscard]] static FORCE_INLINE Result Ok(STORED_TYPE value) noexcept
		requires(!IS_VOID)
	{
		Result r;
		r.m_isOk = true;
		new (&r.m_value) STORED_TYPE(static_cast<STORED_TYPE &&>(value));
		return r;
	}

	[[nodiscard]] static FORCE_INLINE Result Ok() noexcept
		requires(IS_VOID)
	{
		Result r;
		r.m_isOk = true;
		return r;
	}

	// =====================================================================
	// Err factories
	// =====================================================================

	/// Single error — stores E directly (zero-cost)
	[[nodiscard]] static FORCE_INLINE Result Err(E error) noexcept
	{
		Result r;
		r.m_isOk = false;
		new (&r.m_error) E(static_cast<E &&>(error));
		return r;
	}

	/// Backward-compatible 2-arg Err — stores only the last (outermost) code.
	/// Keeps source compatibility with Err(osError, runtimeCode) pattern.
	template <typename First>
		requires(requires(First f) { E(f); })
	[[nodiscard]] static FORCE_INLINE Result Err(First, E last) noexcept
	{
		return Err(static_cast<E &&>(last));
	}

	/// Backward-compatible propagation Err — stores only the appended code.
	/// Keeps source compatibility with Err(failedResult, runtimeCode) pattern.
	template <typename OtherT>
	[[nodiscard]] static FORCE_INLINE Result Err(const Result<OtherT, E> &, E code) noexcept
	{
		return Err(static_cast<E &&>(code));
	}

	// =====================================================================
	// Destructor + move semantics
	// =====================================================================

	~Result() noexcept { DestroyActive(); }

	Result(Result &&other) noexcept : m_isOk(other.m_isOk)
	{
		if (m_isOk)
			new (&m_value) STORED_TYPE(static_cast<STORED_TYPE &&>(other.m_value));
		else
			new (&m_error) E(static_cast<E &&>(other.m_error));
	}

	Result &operator=(Result &&other) noexcept
	{
		if (this != &other)
		{
			DestroyActive();
			m_isOk = other.m_isOk;
			if (m_isOk)
				new (&m_value) STORED_TYPE(static_cast<STORED_TYPE &&>(other.m_value));
			else
				new (&m_error) E(static_cast<E &&>(other.m_error));
		}
		return *this;
	}

	Result(const Result &) = delete;
	Result &operator=(const Result &) = delete;

	// =====================================================================
	// Value / error queries
	// =====================================================================

	[[nodiscard]] FORCE_INLINE BOOL IsOk() const noexcept { return m_isOk; }
	[[nodiscard]] FORCE_INLINE BOOL IsErr() const noexcept { return !m_isOk; }
	[[nodiscard]] FORCE_INLINE operator BOOL() const noexcept { return m_isOk; }

	[[nodiscard]] FORCE_INLINE STORED_TYPE &Value() noexcept
		requires(!IS_VOID)
	{
		return m_value;
	}
	[[nodiscard]] FORCE_INLINE const STORED_TYPE &Value() const noexcept
		requires(!IS_VOID)
	{
		return m_value;
	}

	/// Returns the stored error for inspection and %e formatting.
	[[nodiscard]] FORCE_INLINE const E &Error() const noexcept { return m_error; }

private:
	Result() noexcept {}
};

/** @} */ // end of result group
