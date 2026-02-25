#pragma once

#include "runtime.h"
#include "tests.h"

class ResultTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		LOG_INFO("Running Result Tests...");

		RunTest(allPassed, EMBED_FUNC(TestOkCreation), L"Ok creation"_embed);
		RunTest(allPassed, EMBED_FUNC(TestErrCreation), L"Err creation"_embed);
		RunTest(allPassed, EMBED_FUNC(TestIsOkIsErr), L"IsOk/IsErr queries"_embed);
		RunTest(allPassed, EMBED_FUNC(TestOperatorBool), L"operator BOOL"_embed);
		RunTest(allPassed, EMBED_FUNC(TestValueAccess), L"Value access"_embed);
		RunTest(allPassed, EMBED_FUNC(TestErrorAccess), L"Error access"_embed);
		RunTest(allPassed, EMBED_FUNC(TestMoveConstruction), L"Move construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestMoveAssignment), L"Move assignment"_embed);
		RunTest(allPassed, EMBED_FUNC(TestVoidOk), L"Void specialization Ok"_embed);
		RunTest(allPassed, EMBED_FUNC(TestVoidErr), L"Void specialization Err"_embed);
		RunTest(allPassed, EMBED_FUNC(TestVoidMoveConstruction), L"Void move construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestNonTrivialDestructor), L"Non-trivial destructor"_embed);
		RunTest(allPassed, EMBED_FUNC(TestTypeAliases), L"Type aliases"_embed);

		if (allPassed)
			LOG_INFO("All Result tests passed!");
		else
			LOG_ERROR("Some Result tests failed!");

		return allPassed;
	}

private:
	struct Tracked
	{
		UINT32 value;
		BOOL *destroyed;

		Tracked(UINT32 v, BOOL *flag) : value(v), destroyed(flag) {}
		~Tracked() { if (destroyed) *destroyed = TRUE; }

		Tracked(Tracked &&other) noexcept
			: value(other.value), destroyed(other.destroyed)
		{
			other.destroyed = NULL;
		}

		Tracked &operator=(Tracked &&other) noexcept
		{
			value = other.value;
			destroyed = other.destroyed;
			other.destroyed = NULL;
			return *this;
		}

		Tracked(const Tracked &) = delete;
		Tracked &operator=(const Tracked &) = delete;
	};

	static BOOL TestOkCreation()
	{
		auto result = Result<UINT32, UINT32>::Ok(42);
		if (!result.IsOk())
			return FALSE;
		if (result.Value() != 42)
			return FALSE;
		return TRUE;
	}

	static BOOL TestErrCreation()
	{
		auto result = Result<UINT32, UINT32>::Err(99);
		if (!result.IsErr())
			return FALSE;
		if (result.Error() != 99)
			return FALSE;
		return TRUE;
	}

	static BOOL TestIsOkIsErr()
	{
		auto ok = Result<UINT32, UINT32>::Ok(1);
		auto err = Result<UINT32, UINT32>::Err(2);

		if (!ok.IsOk() || ok.IsErr())
			return FALSE;
		if (!err.IsErr() || err.IsOk())
			return FALSE;
		return TRUE;
	}

	static BOOL TestOperatorBool()
	{
		auto ok = Result<UINT32, UINT32>::Ok(1);
		auto err = Result<UINT32, UINT32>::Err(2);

		if (!ok)
			return FALSE;
		if (err)
			return FALSE;
		return TRUE;
	}

	static BOOL TestValueAccess()
	{
		auto result = Result<UINT32, UINT32>::Ok(123);
		if (result.Value() != 123)
			return FALSE;

		result.Value() = 456;
		if (result.Value() != 456)
			return FALSE;

		return TRUE;
	}

	static BOOL TestErrorAccess()
	{
		auto result = Result<UINT32, UINT32>::Err(789);
		if (result.Error() != 789)
			return FALSE;

		result.Error() = 101;
		if (result.Error() != 101)
			return FALSE;

		return TRUE;
	}

	static BOOL TestMoveConstruction()
	{
		auto original = Result<UINT32, UINT32>::Ok(55);
		auto moved = static_cast<Result<UINT32, UINT32> &&>(original);

		if (!moved.IsOk())
			return FALSE;
		if (moved.Value() != 55)
			return FALSE;

		auto errOriginal = Result<UINT32, UINT32>::Err(66);
		auto errMoved = static_cast<Result<UINT32, UINT32> &&>(errOriginal);

		if (!errMoved.IsErr())
			return FALSE;
		if (errMoved.Error() != 66)
			return FALSE;

		return TRUE;
	}

	static BOOL TestMoveAssignment()
	{
		auto a = Result<UINT32, UINT32>::Ok(10);
		auto b = Result<UINT32, UINT32>::Err(20);

		a = static_cast<Result<UINT32, UINT32> &&>(b);
		if (!a.IsErr())
			return FALSE;
		if (a.Error() != 20)
			return FALSE;

		return TRUE;
	}

	static BOOL TestVoidOk()
	{
		auto result = Result<void, UINT32>::Ok();
		if (!result.IsOk())
			return FALSE;
		if (result.IsErr())
			return FALSE;
		if (!result)
			return FALSE;
		return TRUE;
	}

	static BOOL TestVoidErr()
	{
		auto result = Result<void, UINT32>::Err(42);
		if (!result.IsErr())
			return FALSE;
		if (result.IsOk())
			return FALSE;
		if (result)
			return FALSE;
		if (result.Error() != 42)
			return FALSE;
		return TRUE;
	}

	static BOOL TestVoidMoveConstruction()
	{
		auto ok = Result<void, UINT32>::Ok();
		auto movedOk = static_cast<Result<void, UINT32> &&>(ok);
		if (!movedOk.IsOk())
			return FALSE;

		auto err = Result<void, UINT32>::Err(77);
		auto movedErr = static_cast<Result<void, UINT32> &&>(err);
		if (!movedErr.IsErr())
			return FALSE;
		if (movedErr.Error() != 77)
			return FALSE;

		return TRUE;
	}

	static BOOL TestNonTrivialDestructor()
	{
		BOOL valueDestroyed = FALSE;
		BOOL errorDestroyed = FALSE;

		{
			auto ok = Result<Tracked, UINT32>::Ok(Tracked(1, &valueDestroyed));
			if (valueDestroyed)
				return FALSE;
		}
		if (!valueDestroyed)
			return FALSE;

		{
			auto err = Result<UINT32, Tracked>::Err(Tracked(2, &errorDestroyed));
			if (errorDestroyed)
				return FALSE;
		}
		if (!errorDestroyed)
			return FALSE;

		BOOL moveFlag = FALSE;
		{
			auto original = Result<Tracked, UINT32>::Ok(Tracked(3, &moveFlag));
			moveFlag = FALSE;
			auto moved = static_cast<Result<Tracked, UINT32> &&>(original);
			moveFlag = FALSE;
		}

		return TRUE;
	}

	static BOOL TestTypeAliases()
	{
		static_assert(__is_same(Result<UINT32, UINT64>::ValueType, UINT32), "ValueType mismatch");
		static_assert(__is_same(Result<UINT32, UINT64>::ErrorType, UINT64), "ErrorType mismatch");
		static_assert(__is_same(Result<void, UINT32>::ValueType, void), "void ValueType mismatch");
		static_assert(__is_same(Result<void, UINT32>::ErrorType, UINT32), "void ErrorType mismatch");

		return TRUE;
	}
};
