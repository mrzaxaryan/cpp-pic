#pragma once

#include "runtime.h"
#include "tests.h"

class ResultTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Result Tests...");

		// Construction
		RunTest(allPassed, EMBED_FUNC(TestOkConstruction), L"Ok construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestErrConstruction), L"Err construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestVoidOk), L"Void Ok construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestVoidErr), L"Void Err construction"_embed);

		// Queries
		RunTest(allPassed, EMBED_FUNC(TestIsOkIsErr), L"IsOk/IsErr mutual exclusivity"_embed);
		RunTest(allPassed, EMBED_FUNC(TestOperatorBool), L"operator BOOL"_embed);

		// Value access
		RunTest(allPassed, EMBED_FUNC(TestValueAccess), L"Value access"_embed);
		RunTest(allPassed, EMBED_FUNC(TestValueMutation), L"Value mutation"_embed);

		// Move semantics
		RunTest(allPassed, EMBED_FUNC(TestMoveConstruction), L"Move construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestMoveAssignment), L"Move assignment"_embed);
		RunTest(allPassed, EMBED_FUNC(TestVoidMoveConstruction), L"Void move construction"_embed);

		// Non-trivial destructor
		RunTest(allPassed, EMBED_FUNC(TestNonTrivialDestructor), L"Non-trivial destructor"_embed);
		RunTest(allPassed, EMBED_FUNC(TestMoveTransfersOwnership), L"Move transfers ownership"_embed);

		// Single-error storage (E = Error)
		RunTest(allPassed, EMBED_FUNC(TestSingleError), L"Single error storage"_embed);
		RunTest(allPassed, EMBED_FUNC(TestTwoArgErrCompat), L"Two-arg Err compatibility"_embed);
		RunTest(allPassed, EMBED_FUNC(TestPropagationErrCompat), L"Propagation Err compatibility"_embed);

		// Non-chainable E
		RunTest(allPassed, EMBED_FUNC(TestNonChainableErr), L"Non-chainable E type"_embed);

		// Type aliases
		RunTest(allPassed, EMBED_FUNC(TestTypeAliases), L"Type aliases"_embed);

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
			return false;
		if (r.Value() != 42)
			return false;
		return true;
	}

	static BOOL TestErrConstruction()
	{
		auto r = Result<UINT32, UINT32>::Err(99);
		if (!r.IsErr())
			return false;
		return true;
	}

	static BOOL TestVoidOk()
	{
		auto r = Result<void, UINT32>::Ok();
		if (!r.IsOk())
			return false;
		if (r.IsErr())
			return false;
		return true;
	}

	static BOOL TestVoidErr()
	{
		auto r = Result<void, Error>::Err(Error::Socket_CreateFailed_Open);
		if (!r.IsErr())
			return false;
		if (r.IsOk())
			return false;

		const Error &err = r.Error();
		if (err.Code != Error::Socket_CreateFailed_Open)
			return false;
		if (err.Platform != Error::PlatformKind::Runtime)
			return false;
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
			return false;
		if (!err.IsErr() || err.IsOk())
			return false;
		return true;
	}

	static BOOL TestOperatorBool()
	{
		auto ok = Result<UINT32, UINT32>::Ok(1);
		auto err = Result<UINT32, UINT32>::Err(2);

		if (!(BOOL)ok)
			return false;
		if ((BOOL)err)
			return false;

		// Idiomatic usage
		if (!ok)
			return false;
		if (err)
			return false;
		return true;
	}

	// =====================================================================
	// Value access
	// =====================================================================

	static BOOL TestValueAccess()
	{
		auto r = Result<UINT32, UINT32>::Ok(123);
		if (r.Value() != 123)
			return false;

		// Const access
		const auto &cr = r;
		if (cr.Value() != 123)
			return false;
		return true;
	}

	static BOOL TestValueMutation()
	{
		auto r = Result<UINT32, UINT32>::Ok(100);
		r.Value() = 200;
		if (r.Value() != 200)
			return false;
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
			return false;

		// Move Err
		auto err1 = Result<UINT32, Error>::Err(Error::Socket_OpenFailed_Connect);
		auto err2 = static_cast<Result<UINT32, Error> &&>(err1);
		if (!err2.IsErr())
			return false;
		if (err2.Error().Code != Error::Socket_OpenFailed_Connect)
			return false;
		return true;
	}

	static BOOL TestMoveAssignment()
	{
		auto r = Result<UINT32, UINT32>::Ok(10);
		if (!r.IsOk() || r.Value() != 10)
			return false;

		// Reassign from Err
		r = Result<UINT32, UINT32>::Err(20);
		if (!r.IsErr())
			return false;

		// Reassign back to Ok
		r = Result<UINT32, UINT32>::Ok(30);
		if (!r.IsOk() || r.Value() != 30)
			return false;
		return true;
	}

	static BOOL TestVoidMoveConstruction()
	{
		auto ok1 = Result<void, UINT32>::Ok();
		auto ok2 = static_cast<Result<void, UINT32> &&>(ok1);
		if (!ok2.IsOk())
			return false;

		auto err1 = Result<void, UINT32>::Err(7);
		auto err2 = static_cast<Result<void, UINT32> &&>(err1);
		if (!err2.IsErr())
			return false;
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
				return false; // should not be destroyed yet
		}
		// Tracked destructor must fire when Result leaves scope
		if (!destroyed)
			return false;
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
					return false; // not destroyed yet — r2 owns it
				if (r2.Value().value != 3)
					return false;
			}
			// r2 out of scope — destructor fires
			if (!destroyed)
				return false;
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
			return false;

		const Error &err = r.Error();
		if (err.Code != Error::Dns_ConnectFailed)
			return false;
		if (err.Platform != Error::PlatformKind::Runtime)
			return false;
		return true;
	}

	static BOOL TestTwoArgErrCompat()
	{
		// 2-arg Err stores only the last (outermost) code
		auto r = Result<UINT32, Error>::Err(
			Error::Windows(0xC0000034),
			Error::Socket_OpenFailed_Connect);
		if (!r.IsErr())
			return false;

		const Error &err = r.Error();
		if (err.Code != Error::Socket_OpenFailed_Connect)
			return false;
		if (err.Platform != Error::PlatformKind::Runtime)
			return false;
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
			return false;

		const Error &err = outer.Error();
		if (err.Code != Error::Tls_WriteFailed_Send)
			return false;
		if (err.Platform != Error::PlatformKind::Runtime)
			return false;
		return true;
	}

	// =====================================================================
	// Non-chainable E
	// =====================================================================

	static BOOL TestNonChainableErr()
	{
		auto r1 = Result<UINT32, UINT32>::Err(42);
		if (!r1.IsErr())
			return false;
		if (r1.IsOk())
			return false;

		auto r2 = Result<void, UINT32>::Err(7);
		if (!r2.IsErr())
			return false;

		// Ok path still works
		auto r3 = Result<UINT32, UINT32>::Ok(100);
		if (!r3.IsOk() || r3.Value() != 100)
			return false;
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
};
