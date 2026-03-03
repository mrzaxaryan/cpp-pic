#pragma once

#include "runtime/runtime.h"
#include "tests.h"

class ResultTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Result Tests...");

		// Construction
		RunTest(allPassed, EMBED_FUNC(TestOkConstruction), "Ok construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestErrConstruction), "Err construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestVoidOk), "Void Ok construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestVoidErr), "Void Err construction"_embed);

		// Queries
		RunTest(allPassed, EMBED_FUNC(TestIsOkIsErr), "IsOk/IsErr mutual exclusivity"_embed);
		RunTest(allPassed, EMBED_FUNC(TestOperatorBool), "operator BOOL"_embed);

		// Value access
		RunTest(allPassed, EMBED_FUNC(TestValueAccess), "Value access"_embed);
		RunTest(allPassed, EMBED_FUNC(TestValueMutation), "Value mutation"_embed);

		// Move semantics
		RunTest(allPassed, EMBED_FUNC(TestMoveConstruction), "Move construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestMoveAssignment), "Move assignment"_embed);
		RunTest(allPassed, EMBED_FUNC(TestVoidMoveConstruction), "Void move construction"_embed);

		// Non-trivial destructor
		RunTest(allPassed, EMBED_FUNC(TestNonTrivialDestructor), "Non-trivial destructor"_embed);
		RunTest(allPassed, EMBED_FUNC(TestMoveTransfersOwnership), "Move transfers ownership"_embed);

		// Single-error storage (E = Error)
		RunTest(allPassed, EMBED_FUNC(TestSingleError), "Single error storage"_embed);
		RunTest(allPassed, EMBED_FUNC(TestTwoArgErrCompat), "Two-arg Err compatibility"_embed);
		RunTest(allPassed, EMBED_FUNC(TestPropagationErrCompat), "Propagation Err compatibility"_embed);

		// Non-chainable E
		RunTest(allPassed, EMBED_FUNC(TestNonChainableErr), "Non-chainable E type"_embed);

		// Type aliases
		RunTest(allPassed, EMBED_FUNC(TestTypeAliases), "Type aliases"_embed);

		// Compact specialization (Result<void, Error>)
		RunTest(allPassed, EMBED_FUNC(TestCompactSize), "Compact specialization size"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCompactOk), "Compact void Ok"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCompactErr), "Compact void Err"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCompactPropagation), "Compact void propagation Err"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCompactTwoArgErr), "Compact void two-arg Err"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCompactErrorOnOk), "Compact Error() on Ok is well-defined"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCompactMoveConstruction), "Compact void move construction"_embed);

		if (allPassed)
			LOG_INFO("All Result tests passed!");
		else
			LOG_ERROR("Some Result tests failed!");

		return allPassed;
	}

private:
	// Move-only RAII helper that tracks destruction via a boolean flag.
	struct Tracked
	{
		UINT32 value;
		BOOL *destroyed;

		Tracked(UINT32 v, BOOL *flag) : value(v), destroyed(flag) {}
		~Tracked()
		{
			if (destroyed)
				*destroyed = true;
		}

		Tracked(Tracked &&other) noexcept
			: value(other.value), destroyed(other.destroyed)
		{
			other.destroyed = nullptr;
		}

		Tracked &operator=(Tracked &&other) noexcept
		{
			value = other.value;
			destroyed = other.destroyed;
			other.destroyed = nullptr;
			return *this;
		}

		Tracked(const Tracked &) = delete;
		Tracked &operator=(const Tracked &) = delete;
	};

	// =====================================================================
	// Construction
	// =====================================================================

	static BOOL TestOkConstruction()
	{
		auto r = Result<UINT32, UINT32>::Ok(42);
		if (!r.IsOk())
		{
			LOG_ERROR("Ok(42).IsOk() returned false");
			return false;
		}
		if (r.Value() != 42)
		{
			LOG_ERROR("Ok(42).Value() != 42, got %u", r.Value());
			return false;
		}
		return true;
	}

	static BOOL TestErrConstruction()
	{
		auto r = Result<UINT32, UINT32>::Err(99);
		if (!r.IsErr())
		{
			LOG_ERROR("Err(99).IsErr() returned false");
			return false;
		}
		return true;
	}

	static BOOL TestVoidOk()
	{
		auto r = Result<void, UINT32>::Ok();
		if (!r.IsOk())
		{
			LOG_ERROR("Void Ok().IsOk() returned false");
			return false;
		}
		if (r.IsErr())
		{
			LOG_ERROR("Void Ok().IsErr() returned true");
			return false;
		}
		return true;
	}

	static BOOL TestVoidErr()
	{
		auto r = Result<void, Error>::Err(Error::Socket_CreateFailed_Open);
		if (!r.IsErr())
		{
			LOG_ERROR("Void Err().IsErr() returned false");
			return false;
		}
		if (r.IsOk())
		{
			LOG_ERROR("Void Err().IsOk() returned true");
			return false;
		}

		const Error &err = r.Error();
		if (err.Code != Error::Socket_CreateFailed_Open)
		{
			LOG_ERROR("Error code mismatch: expected Socket_CreateFailed_Open");
			return false;
		}
		if (err.Platform != Error::PlatformKind::Runtime)
		{
			LOG_ERROR("Platform mismatch: expected Runtime");
			return false;
		}
		return true;
	}

	// =====================================================================
	// Queries
	// =====================================================================

	static BOOL TestIsOkIsErr()
	{
		auto ok = Result<UINT32, UINT32>::Ok(1);
		auto err = Result<UINT32, UINT32>::Err(2);

		// Mutual exclusivity
		if (!ok.IsOk() || ok.IsErr())
		{
			LOG_ERROR("Ok result: IsOk/IsErr not mutually exclusive");
			return false;
		}
		if (!err.IsErr() || err.IsOk())
		{
			LOG_ERROR("Err result: IsOk/IsErr not mutually exclusive");
			return false;
		}
		return true;
	}

	static BOOL TestOperatorBool()
	{
		auto ok = Result<UINT32, UINT32>::Ok(1);
		auto err = Result<UINT32, UINT32>::Err(2);

		if (!(BOOL)ok)
		{
			LOG_ERROR("(BOOL)ok returned false");
			return false;
		}
		if ((BOOL)err)
		{
			LOG_ERROR("(BOOL)err returned true");
			return false;
		}

		// Idiomatic usage
		if (!ok)
		{
			LOG_ERROR("!ok evaluated to true");
			return false;
		}
		if (err)
		{
			LOG_ERROR("err evaluated to true");
			return false;
		}
		return true;
	}

	// =====================================================================
	// Value access
	// =====================================================================

	static BOOL TestValueAccess()
	{
		auto r = Result<UINT32, UINT32>::Ok(123);
		if (r.Value() != 123)
		{
			LOG_ERROR("Value() != 123, got %u", r.Value());
			return false;
		}

		// Const access
		const auto &cr = r;
		if (cr.Value() != 123)
		{
			LOG_ERROR("Const Value() != 123");
			return false;
		}
		return true;
	}

	static BOOL TestValueMutation()
	{
		auto r = Result<UINT32, UINT32>::Ok(100);
		r.Value() = 200;
		if (r.Value() != 200)
		{
			LOG_ERROR("After mutation, Value() != 200, got %u", r.Value());
			return false;
		}
		return true;
	}

	// =====================================================================
	// Move semantics
	// =====================================================================

	static BOOL TestMoveConstruction()
	{
		// Move Ok
		auto ok1 = Result<UINT32, UINT32>::Ok(42);
		auto ok2 = static_cast<Result<UINT32, UINT32> &&>(ok1);
		if (!ok2.IsOk() || ok2.Value() != 42)
		{
			LOG_ERROR("Move Ok: value mismatch after move");
			return false;
		}

		// Move Err
		auto err1 = Result<UINT32, Error>::Err(Error::Socket_OpenFailed_Connect);
		auto err2 = static_cast<Result<UINT32, Error> &&>(err1);
		if (!err2.IsErr())
		{
			LOG_ERROR("Move Err: IsErr() false after move");
			return false;
		}
		if (err2.Error().Code != Error::Socket_OpenFailed_Connect)
		{
			LOG_ERROR("Move Err: error code mismatch after move");
			return false;
		}
		return true;
	}

	static BOOL TestMoveAssignment()
	{
		auto r = Result<UINT32, UINT32>::Ok(10);
		if (!r.IsOk() || r.Value() != 10)
		{
			LOG_ERROR("Initial Ok(10) check failed");
			return false;
		}

		// Reassign from Err
		r = Result<UINT32, UINT32>::Err(20);
		if (!r.IsErr())
		{
			LOG_ERROR("After reassign to Err: IsErr() false");
			return false;
		}

		// Reassign back to Ok
		r = Result<UINT32, UINT32>::Ok(30);
		if (!r.IsOk() || r.Value() != 30)
		{
			LOG_ERROR("After reassign to Ok(30): check failed");
			return false;
		}
		return true;
	}

	static BOOL TestVoidMoveConstruction()
	{
		auto ok1 = Result<void, UINT32>::Ok();
		auto ok2 = static_cast<Result<void, UINT32> &&>(ok1);
		if (!ok2.IsOk())
		{
			LOG_ERROR("Void move Ok: IsOk() false");
			return false;
		}

		auto err1 = Result<void, UINT32>::Err(7);
		auto err2 = static_cast<Result<void, UINT32> &&>(err1);
		if (!err2.IsErr())
		{
			LOG_ERROR("Void move Err: IsErr() false");
			return false;
		}
		return true;
	}

	// =====================================================================
	// Non-trivial destructor
	// =====================================================================

	static BOOL TestNonTrivialDestructor()
	{
		BOOL destroyed = false;
		{
			auto r = Result<Tracked, UINT32>::Ok(Tracked(1, &destroyed));
			if (destroyed)
			{
				LOG_ERROR("Tracked destroyed prematurely inside scope");
				return false;
			}
		}
		// Tracked destructor must fire when Result leaves scope
		if (!destroyed)
		{
			LOG_ERROR("Tracked not destroyed after scope exit");
			return false;
		}
		return true;
	}

	static BOOL TestMoveTransfersOwnership()
	{
		BOOL destroyed = false;
		{
			auto r1 = Result<Tracked, UINT32>::Ok(Tracked(3, &destroyed));
			{
				auto r2 = static_cast<Result<Tracked, UINT32> &&>(r1);
				if (destroyed)
				{
					LOG_ERROR("Tracked destroyed after move (r2 still alive)");
					return false;
				}
				if (r2.Value().value != 3)
				{
					LOG_ERROR("Tracked value mismatch after move: expected 3");
					return false;
				}
			}
			// r2 out of scope — destructor fires
			if (!destroyed)
			{
				LOG_ERROR("Tracked not destroyed after r2 scope exit");
				return false;
			}
		}
		// r1 source was nullified by move — no double-destroy
		return true;
	}

	// =====================================================================
	// Single-error storage (E = Error)
	// =====================================================================

	static BOOL TestSingleError()
	{
		auto r = Result<UINT32, Error>::Err(Error::Dns_ConnectFailed);
		if (!r.IsErr())
		{
			LOG_ERROR("SingleError: IsErr() false");
			return false;
		}

		const Error &err = r.Error();
		if (err.Code != Error::Dns_ConnectFailed)
		{
			LOG_ERROR("SingleError: Code mismatch");
			return false;
		}
		if (err.Platform != Error::PlatformKind::Runtime)
		{
			LOG_ERROR("SingleError: Platform mismatch");
			return false;
		}
		return true;
	}

	static BOOL TestTwoArgErrCompat()
	{
		// 2-arg Err stores only the last (outermost) code
		auto r = Result<UINT32, Error>::Err(
			Error::Windows(0xC0000034),
			Error::Socket_OpenFailed_Connect);
		if (!r.IsErr())
		{
			LOG_ERROR("TwoArgErr: IsErr() false");
			return false;
		}

		const Error &err = r.Error();
		if (err.Code != Error::Socket_OpenFailed_Connect)
		{
			LOG_ERROR("TwoArgErr: Code mismatch, expected Socket_OpenFailed_Connect");
			return false;
		}
		if (err.Platform != Error::PlatformKind::Runtime)
		{
			LOG_ERROR("TwoArgErr: Platform mismatch");
			return false;
		}
		return true;
	}

	static BOOL TestPropagationErrCompat()
	{
		// Build an inner error
		auto inner = Result<UINT32, Error>::Err(
			Error::Posix(111),
			Error::Socket_WriteFailed_Send);

		// Propagate — stores only the appended code
		auto outer = Result<void, Error>::Err(inner, Error::Tls_WriteFailed_Send);
		if (!outer.IsErr())
		{
			LOG_ERROR("PropagationErr: IsErr() false");
			return false;
		}

		const Error &err = outer.Error();
		if (err.Code != Error::Tls_WriteFailed_Send)
		{
			LOG_ERROR("PropagationErr: Code mismatch, expected Tls_WriteFailed_Send");
			return false;
		}
		if (err.Platform != Error::PlatformKind::Runtime)
		{
			LOG_ERROR("PropagationErr: Platform mismatch");
			return false;
		}
		return true;
	}

	// =====================================================================
	// Non-chainable E
	// =====================================================================

	static BOOL TestNonChainableErr()
	{
		auto r1 = Result<UINT32, UINT32>::Err(42);
		if (!r1.IsErr())
		{
			LOG_ERROR("NonChainable Err(42): IsErr() false");
			return false;
		}
		if (r1.IsOk())
		{
			LOG_ERROR("NonChainable Err(42): IsOk() true");
			return false;
		}

		auto r2 = Result<void, UINT32>::Err(7);
		if (!r2.IsErr())
		{
			LOG_ERROR("NonChainable void Err(7): IsErr() false");
			return false;
		}

		// Ok path still works
		auto r3 = Result<UINT32, UINT32>::Ok(100);
		if (!r3.IsOk() || r3.Value() != 100)
		{
			LOG_ERROR("NonChainable Ok(100): check failed");
			return false;
		}
		return true;
	}

	// =====================================================================
	// Type aliases
	// =====================================================================

	static BOOL TestTypeAliases()
	{
		static_assert(__is_same(Result<UINT32, UINT64>::ValueType, UINT32));
		static_assert(__is_same(Result<UINT32, UINT64>::ErrorType, UINT64));
		static_assert(__is_same(Result<void, UINT32>::ValueType, void));
		static_assert(__is_same(Result<void, UINT32>::ErrorType, UINT32));
		return true;
	}

	// =====================================================================
	// Compact specialization (Result<void, Error>)
	// =====================================================================

	static BOOL TestCompactSize()
	{
		static_assert(sizeof(Result<void, Error>) == sizeof(Error),
			"Compact specialization must equal sizeof(Error)");
		static_assert(sizeof(Result<void, Error>) == 8,
			"Compact specialization must be 8 bytes");
		// Primary template is NOT affected
		static_assert(sizeof(Result<void, UINT32>) > sizeof(UINT32),
			"Primary template Result<void, UINT32> should have m_isOk overhead");
		return true;
	}

	static BOOL TestCompactOk()
	{
		auto r = Result<void, Error>::Ok();
		if (!r.IsOk())
		{
			LOG_ERROR("Compact Ok: IsOk() returned false");
			return false;
		}
		if (r.IsErr())
		{
			LOG_ERROR("Compact Ok: IsErr() returned true");
			return false;
		}
		if (!r)
		{
			LOG_ERROR("Compact Ok: operator BOOL returned false");
			return false;
		}
		return true;
	}

	static BOOL TestCompactErr()
	{
		auto r = Result<void, Error>::Err(Error::Socket_CreateFailed_Open);
		if (!r.IsErr())
		{
			LOG_ERROR("Compact Err: IsErr() returned false");
			return false;
		}
		if (r.IsOk())
		{
			LOG_ERROR("Compact Err: IsOk() returned true");
			return false;
		}
		if (r)
		{
			LOG_ERROR("Compact Err: operator BOOL returned true");
			return false;
		}

		const Error &err = r.Error();
		if (err.Code != Error::Socket_CreateFailed_Open)
		{
			LOG_ERROR("Compact Err: Code mismatch");
			return false;
		}
		if (err.Platform != Error::PlatformKind::Runtime)
		{
			LOG_ERROR("Compact Err: Platform mismatch");
			return false;
		}
		return true;
	}

	static BOOL TestCompactPropagation()
	{
		auto inner = Result<UINT32, Error>::Err(
			Error::Posix(111),
			Error::Socket_WriteFailed_Send);

		auto outer = Result<void, Error>::Err(inner, Error::Tls_WriteFailed_Send);
		if (!outer.IsErr())
		{
			LOG_ERROR("Compact propagation: IsErr() false");
			return false;
		}

		const Error &err = outer.Error();
		if (err.Code != Error::Tls_WriteFailed_Send)
		{
			LOG_ERROR("Compact propagation: Code mismatch");
			return false;
		}
		if (err.Platform != Error::PlatformKind::Runtime)
		{
			LOG_ERROR("Compact propagation: Platform mismatch");
			return false;
		}
		return true;
	}

	static BOOL TestCompactTwoArgErr()
	{
		auto r = Result<void, Error>::Err(
			Error::Windows(0xC0000034),
			Error::Socket_OpenFailed_Connect);
		if (!r.IsErr())
		{
			LOG_ERROR("Compact two-arg Err: IsErr() false");
			return false;
		}

		const Error &err = r.Error();
		if (err.Code != Error::Socket_OpenFailed_Connect)
		{
			LOG_ERROR("Compact two-arg Err: Code mismatch");
			return false;
		}
		if (err.Platform != Error::PlatformKind::Runtime)
		{
			LOG_ERROR("Compact two-arg Err: Platform mismatch");
			return false;
		}
		return true;
	}

	static BOOL TestCompactErrorOnOk()
	{
		auto r = Result<void, Error>::Ok();
		// Unlike the primary template (UB), the compact specialization
		// returns a well-defined Error{None, Runtime} on Ok results
		const Error &err = r.Error();
		if (err.Code != Error::None)
		{
			LOG_ERROR("Compact Error() on Ok: Code != None");
			return false;
		}
		if (err.Platform != Error::PlatformKind::Runtime)
		{
			LOG_ERROR("Compact Error() on Ok: Platform != Runtime");
			return false;
		}
		return true;
	}

	static BOOL TestCompactMoveConstruction()
	{
		// Move Ok
		auto ok1 = Result<void, Error>::Ok();
		auto ok2 = static_cast<Result<void, Error> &&>(ok1);
		if (!ok2.IsOk())
		{
			LOG_ERROR("Compact move Ok: IsOk() false");
			return false;
		}

		// Move Err
		auto err1 = Result<void, Error>::Err(Error::Dns_ConnectFailed);
		auto err2 = static_cast<Result<void, Error> &&>(err1);
		if (!err2.IsErr())
		{
			LOG_ERROR("Compact move Err: IsErr() false");
			return false;
		}
		if (err2.Error().Code != Error::Dns_ConnectFailed)
		{
			LOG_ERROR("Compact move Err: Code mismatch after move");
			return false;
		}
		return true;
	}
};
