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
		RunTest(allPassed, EMBED_FUNC(TestErrorFactoryMethods), L"Error factory methods"_embed);
		RunTest(allPassed, EMBED_FUNC(TestErrorChaining), L"Error chaining"_embed);

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

	static BOOL TestOkCreation()
	{
		auto result = Result<UINT32, UINT32>::Ok(42);
		if (!result.IsOk())
			return false;
		if (result.Value() != 42)
			return false;
		return true;
	}

	static BOOL TestErrCreation()
	{
		auto result = Result<UINT32, UINT32>::Err(99);
		if (!result.IsErr())
			return false;
		if (result.Error() != 99)
			return false;
		return true;
	}

	static BOOL TestIsOkIsErr()
	{
		auto ok = Result<UINT32, UINT32>::Ok(1);
		auto err = Result<UINT32, UINT32>::Err(2);

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

		if (!ok)
			return false;
		if (err)
			return false;
		return true;
	}

	static BOOL TestValueAccess()
	{
		auto result = Result<UINT32, UINT32>::Ok(123);
		if (result.Value() != 123)
			return false;

		result.Value() = 456;
		if (result.Value() != 456)
			return false;

		return true;
	}

	static BOOL TestErrorAccess()
	{
		auto result = Result<UINT32, UINT32>::Err(789);
		if (result.Error() != 789)
			return false;

		result.Error() = 101;
		if (result.Error() != 101)
			return false;

		return true;
	}

	static BOOL TestMoveConstruction()
	{
		auto original = Result<UINT32, UINT32>::Ok(55);
		auto moved = static_cast<Result<UINT32, UINT32> &&>(original);

		if (!moved.IsOk())
			return false;
		if (moved.Value() != 55)
			return false;

		auto errOriginal = Result<UINT32, UINT32>::Err(66);
		auto errMoved = static_cast<Result<UINT32, UINT32> &&>(errOriginal);

		if (!errMoved.IsErr())
			return false;
		if (errMoved.Error() != 66)
			return false;

		return true;
	}

	static BOOL TestMoveAssignment()
	{
		auto a = Result<UINT32, UINT32>::Ok(10);
		auto b = Result<UINT32, UINT32>::Err(20);

		a = static_cast<Result<UINT32, UINT32> &&>(b);
		if (!a.IsErr())
			return false;
		if (a.Error() != 20)
			return false;

		return true;
	}

	static BOOL TestVoidOk()
	{
		auto result = Result<void, UINT32>::Ok();
		if (!result.IsOk())
			return false;
		if (result.IsErr())
			return false;
		if (!result)
			return false;
		return true;
	}

	static BOOL TestVoidErr()
	{
		auto result = Result<void, UINT32>::Err(42);
		if (!result.IsErr())
			return false;
		if (result.IsOk())
			return false;
		if (result)
			return false;
		if (result.Error() != 42)
			return false;
		return true;
	}

	static BOOL TestVoidMoveConstruction()
	{
		auto ok = Result<void, UINT32>::Ok();
		auto movedOk = static_cast<Result<void, UINT32> &&>(ok);
		if (!movedOk.IsOk())
			return false;

		auto err = Result<void, UINT32>::Err(77);
		auto movedErr = static_cast<Result<void, UINT32> &&>(err);
		if (!movedErr.IsErr())
			return false;
		if (movedErr.Error() != 77)
			return false;

		return true;
	}

	static BOOL TestNonTrivialDestructor()
	{
		BOOL valueDestroyed = false;
		BOOL errorDestroyed = false;

		{
			auto ok = Result<Tracked, UINT32>::Ok(Tracked(1, &valueDestroyed));
			if (valueDestroyed)
				return false;
		}
		if (!valueDestroyed)
			return false;

		{
			auto err = Result<UINT32, Tracked>::Err(Tracked(2, &errorDestroyed));
			if (errorDestroyed)
				return false;
		}
		if (!errorDestroyed)
			return false;

		BOOL moveFlag = false;
		{
			auto original = Result<Tracked, UINT32>::Ok(Tracked(3, &moveFlag));
			moveFlag = false;
			auto moved = static_cast<Result<Tracked, UINT32> &&>(original);
			moveFlag = false;
		}

		return true;
	}

	static BOOL TestTypeAliases()
	{
		static_assert(__is_same(Result<UINT32, UINT64>::ValueType, UINT32), "ValueType mismatch");
		static_assert(__is_same(Result<UINT32, UINT64>::ErrorType, UINT64), "ErrorType mismatch");
		static_assert(__is_same(Result<void, UINT32>::ValueType, void), "void ValueType mismatch");
		static_assert(__is_same(Result<void, UINT32>::ErrorType, UINT32), "void ErrorType mismatch");

		return true;
	}

	static BOOL TestErrorFactoryMethods()
	{
		// Test Error::Windows factory
		Error winErr = Error::Windows(0xC0000034);
		if (winErr.Code != (Error::ErrorCodes)0xC0000034)
			return false;
		if (winErr.Platform != Error::PlatformKind::Windows)
			return false;

		// Test Error::Posix factory
		Error posixErr = Error::Posix(111);
		if (posixErr.Code != (Error::ErrorCodes)111)
			return false;
		if (posixErr.Platform != Error::PlatformKind::Posix)
			return false;

		// Test Error::Uefi factory
		Error uefiErr = Error::Uefi(0x80000001);
		if (uefiErr.Code != (Error::ErrorCodes)0x80000001)
			return false;
		if (uefiErr.Platform != Error::PlatformKind::Uefi)
			return false;

		// Test default constructor (Runtime)
		Error runtimeErr(Error::Socket_WriteFailed_Send);
		if (runtimeErr.Platform != Error::PlatformKind::Runtime)
			return false;
		if (runtimeErr.Code != Error::Socket_WriteFailed_Send)
			return false;

		return true;
	}

	static BOOL TestErrorChaining()
	{
		// Test single-code chainable Result
		auto single = Result<UINT32, Error>::Err(Error::Dns_ConnectFailed);
		if (!single.IsErr())
			return false;
		if (single.ErrorDepth() != 1)
			return false;
		if (single.Bottom().Code != Error::Dns_ConnectFailed)
			return false;
		if (single.Kind() != Error::PlatformKind::Runtime)
			return false;

		// Test two-code chain: OS error + runtime code
		auto twoCode = Result<UINT32, Error>::Err(
			Error::Windows(0xC0000034),
			Error::Socket_OpenFailed_Connect);
		if (twoCode.ErrorDepth() != 2)
			return false;
		if (twoCode.Bottom().Code != (Error::ErrorCodes)0xC0000034)
			return false;
		if (twoCode.Bottom().Platform != Error::PlatformKind::Windows)
			return false;
		if (twoCode.Top().Code != Error::Socket_OpenFailed_Connect)
			return false;
		if (twoCode.Kind() != Error::PlatformKind::Windows)
			return false;

		// Test propagation: copy chain from source + append
		auto propagated = Result<void, Error>::Err(twoCode, Error::Tls_OpenFailed_Socket);
		if (propagated.ErrorDepth() != 3)
			return false;
		if (propagated.ErrorAt(0).Code != (Error::ErrorCodes)0xC0000034)
			return false;
		if (propagated.ErrorAt(1).Code != Error::Socket_OpenFailed_Connect)
			return false;
		if (propagated.ErrorAt(2).Code != Error::Tls_OpenFailed_Socket)
			return false;

		// Test HasCode
		if (!propagated.HasCode(Error::Tls_OpenFailed_Socket))
			return false;
		if (!propagated.HasCode(Error::Socket_OpenFailed_Connect))
			return false;
		if (propagated.HasCode(Error::Dns_ConnectFailed))
			return false;

		return true;
	}
};
