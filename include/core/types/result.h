/**
 * @file result.h
 * @brief Lightweight Result Type for Error Handling
 *
 * @details Tagged union `Result<T, E>` — either a success value (T) or error (E).
 * When T is void, a trivial sentinel replaces the value member so a single
 * template handles both cases without a separate specialization.
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
struct VOID_TAG {};

template <typename T>
struct VOID_TO_TAG { using Type = T; };

template <>
struct VOID_TO_TAG<void> { using Type = VOID_TAG; };

template <typename T, typename E>
class [[nodiscard]] Result
{
	static constexpr bool IS_VOID = __is_same_as(T, void);
	using STORED_TYPE = typename VOID_TO_TAG<T>::Type;

	union { STORED_TYPE m_value; E m_error; };
	BOOL m_isOk;

	FORCE_INLINE void DestroyActive() noexcept
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

	[[nodiscard]] static FORCE_INLINE Result Ok(STORED_TYPE value) noexcept
		requires (!IS_VOID)
	{
		Result r;
		r.m_isOk = TRUE;
		new (&r.m_value) STORED_TYPE(static_cast<STORED_TYPE &&>(value));
		return r;
	}

	[[nodiscard]] static FORCE_INLINE Result Ok() noexcept
		requires (IS_VOID)
	{
		Result r;
		r.m_isOk = TRUE;
		return r;
	}

	[[nodiscard]] static FORCE_INLINE Result Err(E error) noexcept
	{
		Result r;
		r.m_isOk = FALSE;
		new (&r.m_error) E(static_cast<E &&>(error));
		return r;
	}

	FORCE_INLINE ~Result() noexcept { DestroyActive(); }

	FORCE_INLINE Result(Result &&other) noexcept : m_isOk(other.m_isOk)
	{
		if (m_isOk)
			new (&m_value) STORED_TYPE(static_cast<STORED_TYPE &&>(other.m_value));
		else
			new (&m_error) E(static_cast<E &&>(other.m_error));
	}

	FORCE_INLINE Result &operator=(Result &&other) noexcept
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

	[[nodiscard]] FORCE_INLINE BOOL IsOk() const noexcept { return m_isOk; }
	[[nodiscard]] FORCE_INLINE BOOL IsErr() const noexcept { return !m_isOk; }
	[[nodiscard]] FORCE_INLINE operator BOOL() const noexcept { return m_isOk; }

	[[nodiscard]] FORCE_INLINE STORED_TYPE &Value() noexcept requires (!IS_VOID) { return m_value; }
	[[nodiscard]] FORCE_INLINE const STORED_TYPE &Value() const noexcept requires (!IS_VOID) { return m_value; }
	[[nodiscard]] FORCE_INLINE E &Error() noexcept { return m_error; }
	[[nodiscard]] FORCE_INLINE const E &Error() const noexcept { return m_error; }

private:
	Result() noexcept {}
};

/** @} */ // end of result group
