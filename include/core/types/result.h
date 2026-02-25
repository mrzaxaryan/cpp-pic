/**
 * @file result.h
 * @brief Lightweight Result Type for Error Handling
 *
 * @details Tagged union `Result<T, E>` — either a success value (T) or error (E).
 * When T is void, a trivial sentinel replaces the value member so a single
 * template handles both cases without a separate specialization.
 *
 * When E declares `static constexpr UINT32 MaxChainDepth`, Result stores an
 * internal error chain (array of E + depth counter) and exposes chain query
 * methods (Bottom, Top, ErrorAt, HasCode, etc.) plus a propagation factory
 * `Err(failedResult, codes...)`. Non-chainable E types use plain single-error
 * storage with no overhead.
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

/// Error chain storage — array of E + depth counter (used when CHAINABLE)
template <typename U, UINT32 N>
struct ErrorChain
{
	U codes[N];
	UINT32 depth;
};

/// Type selector (no STL needed)
template <bool C, typename A, typename B>
struct SelectType
{
	using Type = A;
};

template <typename A, typename B>
struct SelectType<false, A, B>
{
	using Type = B;
};

template <typename T, typename E>
class [[nodiscard]] Result
{
	static constexpr bool IS_VOID = __is_same_as(T, void);
	using STORED_TYPE = typename VOID_TO_TAG<T>::Type;

	// Detect if E supports error chaining (e.g., Error declares MaxChainDepth)
	template <typename U>
	static consteval UINT32 GetChainDepth()
	{
		if constexpr (requires { U::MaxChainDepth; })
			return U::MaxChainDepth;
		else
			return 0;
	}

	static constexpr UINT32 CHAIN_DEPTH = GetChainDepth<E>();
	static constexpr bool CHAINABLE = CHAIN_DEPTH > 0;

	using ERROR_TYPE = typename SelectType<CHAINABLE, ErrorChain<E, CHAINABLE ? CHAIN_DEPTH : 1>, E>::Type;

	// Cross-Result access for propagation Err overload
	template <typename, typename>
	friend class Result;

	union
	{
		STORED_TYPE m_value;
		ERROR_TYPE m_error;
	};
	BOOL m_isOk;

	// -- Internal helpers for chain storage --

	FORCE_INLINE void PushCode(E code) noexcept
		requires(CHAINABLE)
	{
		if (m_error.depth < CHAIN_DEPTH)
			m_error.codes[m_error.depth] = code;
		m_error.depth++;
	}

	FORCE_INLINE void DestroyActive() noexcept
	{
		if constexpr (!__is_trivially_destructible(STORED_TYPE) || !__is_trivially_destructible(ERROR_TYPE))
		{
			if (m_isOk)
			{
				if constexpr (!__is_trivially_destructible(STORED_TYPE))
					m_value.~STORED_TYPE();
			}
			else
			{
				if constexpr (!__is_trivially_destructible(ERROR_TYPE))
					m_error.~ERROR_TYPE();
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

	/// Single error — non-chainable E: stores E directly
	[[nodiscard]] static FORCE_INLINE Result Err(E error) noexcept
		requires(!CHAINABLE)
	{
		Result r;
		r.m_isOk = false;
		new (&r.m_error) E(static_cast<E &&>(error));
		return r;
	}

	/// Single error — chainable E: stores one code at depth=1
	[[nodiscard]] static FORCE_INLINE Result Err(E error) noexcept
		requires(CHAINABLE)
	{
		Result r;
		r.m_isOk = false;
		r.m_error.depth = 1;
		r.m_error.codes[0] = error;
		return r;
	}

	/// Variadic — chainable only, 2+ args, each convertible to E.
	/// Pushes codes in order: first arg = innermost (index 0).
	template <typename First, typename... Rest>
		requires(CHAINABLE && sizeof...(Rest) >= 1 && requires(First f) { E(f); })
	[[nodiscard]] static FORCE_INLINE Result Err(First first, Rest... rest) noexcept
	{
		Result r;
		r.m_isOk = false;
		r.m_error.depth = 0;
		r.PushCode(E(first));
		(r.PushCode(E(rest)), ...);
		return r;
	}

	/// Propagation — copies the error chain from a failed Result and appends
	/// additional codes. Replaces the old `err = r.Error(); err.Push(X); Err(err)` pattern.
	template <typename OtherT, typename... Codes>
		requires(CHAINABLE)
	[[nodiscard]] static FORCE_INLINE Result Err(const Result<OtherT, E> &source, Codes... codes) noexcept
	{
		Result r;
		r.m_isOk = false;
		r.m_error.depth = 0;

		// Copy source chain (only if source is in error state)
		if (!source.m_isOk)
		{
			UINT32 srcStored = source.m_error.depth < CHAIN_DEPTH ? source.m_error.depth : CHAIN_DEPTH;
			for (UINT32 i = 0; i < srcStored; i++)
				r.m_error.codes[i] = source.m_error.codes[i];
			r.m_error.depth = source.m_error.depth;
		}

		// Append additional codes
		(r.PushCode(E(codes)), ...);
		return r;
	}

	// =====================================================================
	// Destructor + move semantics
	// =====================================================================

	FORCE_INLINE ~Result() noexcept { DestroyActive(); }

	FORCE_INLINE Result(Result &&other) noexcept : m_isOk(other.m_isOk)
	{
		if (m_isOk)
			new (&m_value) STORED_TYPE(static_cast<STORED_TYPE &&>(other.m_value));
		else
			new (&m_error) ERROR_TYPE(static_cast<ERROR_TYPE &&>(other.m_error));
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
				new (&m_error) ERROR_TYPE(static_cast<ERROR_TYPE &&>(other.m_error));
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

	/// Non-chainable: returns E& (unchanged from before)
	[[nodiscard]] FORCE_INLINE E &Error() noexcept
		requires(!CHAINABLE)
	{
		return m_error;
	}
	[[nodiscard]] FORCE_INLINE const E &Error() const noexcept
		requires(!CHAINABLE)
	{
		return m_error;
	}

	/// Chainable: returns reference to the bottom (innermost) code
	[[nodiscard]] FORCE_INLINE E &Error() noexcept
		requires(CHAINABLE)
	{
		return m_error.codes[0];
	}
	[[nodiscard]] FORCE_INLINE const E &Error() const noexcept
		requires(CHAINABLE)
	{
		return m_error.codes[0];
	}

	// =====================================================================
	// Chain query methods (chainable E only)
	// =====================================================================

	/// Returns the innermost (first pushed, lowest-layer) code, or default if empty.
	[[nodiscard]] FORCE_INLINE E Bottom() const noexcept
		requires(CHAINABLE)
	{
		return m_error.depth > 0 ? m_error.codes[0] : E();
	}

	/// Returns the outermost (last pushed, highest-layer) code, or default if empty.
	[[nodiscard]] FORCE_INLINE E Top() const noexcept
		requires(CHAINABLE)
	{
		if (m_error.depth == 0)
			return E();
		UINT32 stored = m_error.depth < CHAIN_DEPTH ? m_error.depth : CHAIN_DEPTH;
		return m_error.codes[stored - 1];
	}

	/// Returns the PlatformKind from the innermost code.
	[[nodiscard]] FORCE_INLINE auto Kind() const noexcept
		requires(CHAINABLE)
	{
		return Bottom().Platform;
	}

	/// Returns total codes pushed (may exceed CHAIN_DEPTH on overflow).
	[[nodiscard]] FORCE_INLINE UINT32 ErrorDepth() const noexcept
		requires(CHAINABLE)
	{
		return m_error.depth;
	}

	/// Returns true if more codes were pushed than CHAIN_DEPTH can store.
	[[nodiscard]] FORCE_INLINE BOOL ErrorOverflow() const noexcept
		requires(CHAINABLE)
	{
		return m_error.depth > CHAIN_DEPTH;
	}

	/// Returns the code at position index (0 = innermost). Returns default if out of range.
	[[nodiscard]] FORCE_INLINE E ErrorAt(UINT32 index) const noexcept
		requires(CHAINABLE)
	{
		return (index < CHAIN_DEPTH && index < m_error.depth) ? m_error.codes[index] : E();
	}

	/// Returns true if any stored code matches the given code value.
	template <typename CodeType>
	[[nodiscard]] FORCE_INLINE BOOL HasCode(CodeType code) const noexcept
		requires(CHAINABLE && requires(E e) { e.Code == code; })
	{
		UINT32 stored = m_error.depth < CHAIN_DEPTH ? m_error.depth : CHAIN_DEPTH;
		for (UINT32 i = 0; i < stored; i++)
		{
			if (m_error.codes[i].Code == code)
				return true;
		}
		return false;
	}

private:
	Result() noexcept {}
};

/** @} */ // end of result group
