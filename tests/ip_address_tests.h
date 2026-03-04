#pragma once

#include "runtime/runtime.h"
#include "tests.h"

// =============================================================================
// Compile-Time (constexpr) Verification
// =============================================================================
// These static_assert tests verify that IPAddress operations are fully evaluated
// at compile time, producing no .rdata or data section entries.

// Default constructor produces Invalid
static_assert(!IPAddress().IsValid());
static_assert(!IPAddress().IsIPv4());
static_assert(!IPAddress().IsIPv6());
static_assert(IPAddress().GetVersion() == IPVersion::Invalid);

// FromIPv4 factory
static_assert(IPAddress::FromIPv4(0x0100007F).IsIPv4());
static_assert(IPAddress::FromIPv4(0x0100007F).IsValid());
static_assert(!IPAddress::FromIPv4(0x0100007F).IsIPv6());
static_assert(IPAddress::FromIPv4(0x0100007F).ToIPv4() == 0x0100007F);

// FromIPv6 factory
constexpr UINT8 kTestIPv6[16] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
static_assert(IPAddress::FromIPv6(kTestIPv6).IsIPv6());
static_assert(IPAddress::FromIPv6(kTestIPv6).IsValid());
static_assert(!IPAddress::FromIPv6(kTestIPv6).IsIPv4());
static_assert(IPAddress::FromIPv6(kTestIPv6).ToIPv4() == 0xFFFFFFFF);
static_assert(IPAddress::FromIPv6(kTestIPv6).ToIPv6() != nullptr);

// Invalid factory
static_assert(!IPAddress::Invalid().IsValid());
static_assert(IPAddress::Invalid().GetVersion() == IPVersion::Invalid);

// LocalHost IPv4
static_assert(IPAddress::LocalHost().IsIPv4());
static_assert(IPAddress::LocalHost().ToIPv4() == 0x0100007F);

// LocalHost IPv6
static_assert(IPAddress::LocalHost(true).IsIPv6());
static_assert(IPAddress::LocalHost(true).IsValid());

// Equality operator
static_assert(IPAddress::FromIPv4(0x01010101) == IPAddress::FromIPv4(0x01010101));
static_assert(!(IPAddress::FromIPv4(0x01010101) == IPAddress::FromIPv4(0x08080808)));
static_assert(IPAddress::FromIPv4(0x01010101) != IPAddress::FromIPv4(0x08080808));
static_assert(IPAddress::Invalid() == IPAddress::Invalid());
static_assert(IPAddress::FromIPv6(kTestIPv6) == IPAddress::FromIPv6(kTestIPv6));
static_assert(IPAddress::FromIPv4(0x01010101) != IPAddress::Invalid());

// Copy constructor
static_assert(IPAddress(IPAddress::FromIPv4(0xC0A80001)).ToIPv4() == 0xC0A80001);
static_assert(IPAddress(IPAddress::FromIPv6(kTestIPv6)).IsIPv6());

// Assignment (verified through constexpr lambda)
static_assert([]() constexpr
{
	IPAddress a = IPAddress::FromIPv4(0x01010101);
	IPAddress b;
	b = a;
	return b.ToIPv4() == 0x01010101 && b.IsIPv4();
}());

static_assert([]() constexpr
{
	IPAddress a = IPAddress::FromIPv6(kTestIPv6);
	IPAddress b;
	b = a;
	return b.IsIPv6() && b == a;
}());

// =============================================================================
// IPAddress Tests - Runtime Validation
// =============================================================================

class IPAddressTests
{
private:
	static BOOL TestConstexprIPv4()
	{
		constexpr IPAddress ip = IPAddress::FromIPv4(0x0100007F);
		if (!ip.IsIPv4())
			return false;
		if (ip.ToIPv4() != 0x0100007F)
			return false;
		if (ip.IsIPv6())
			return false;
		return true;
	}

	static BOOL TestConstexprIPv6()
	{
		constexpr UINT8 addr[16] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
		constexpr IPAddress ip = IPAddress::FromIPv6(addr);
		if (!ip.IsIPv6())
			return false;
		if (ip.IsIPv4())
			return false;
		if (!ip.IsValid())
			return false;
		return true;
	}

	static BOOL TestConstexprLocalHost()
	{
		constexpr IPAddress v4 = IPAddress::LocalHost();
		constexpr IPAddress v6 = IPAddress::LocalHost(true);

		if (!v4.IsIPv4())
			return false;
		if (v4.ToIPv4() != 0x0100007F)
			return false;
		if (!v6.IsIPv6())
			return false;
		return true;
	}

	static BOOL TestConstexprEquality()
	{
		constexpr IPAddress a = IPAddress::FromIPv4(0x01010101);
		constexpr IPAddress b = IPAddress::FromIPv4(0x01010101);
		constexpr IPAddress c = IPAddress::FromIPv4(0x08080808);

		if (!(a == b))
			return false;
		if (a == c)
			return false;
		if (!(a != c))
			return false;
		return true;
	}

	static BOOL TestConstexprCopy()
	{
		constexpr IPAddress original = IPAddress::FromIPv4(0xC0A80001);
		constexpr IPAddress copy(original);

		if (copy.ToIPv4() != original.ToIPv4())
			return false;
		if (!(copy == original))
			return false;
		return true;
	}

	static BOOL TestConstexprInvalid()
	{
		constexpr IPAddress inv = IPAddress::Invalid();
		if (inv.IsValid())
			return false;
		if (inv.IsIPv4())
			return false;
		if (inv.IsIPv6())
			return false;

		constexpr IPAddress def;
		if (def.IsValid())
			return false;
		if (!(inv == def))
			return false;
		return true;
	}

	static BOOL TestFromStringIPv4()
	{
		auto ipStr = "192.168.1.1"_embed;
		auto result = IPAddress::FromString((PCCHAR)ipStr);
		if (!result)
			return false;
		auto& ip = result.Value();
		if (!ip.IsIPv4())
			return false;

		// Verify round-trip through ToString
		CHAR buffer[64];
		auto toStrResult = ip.ToString(Span<CHAR>(buffer));
		if (!toStrResult)
			return false;

		// Verify the string matches
		auto expected = "192.168.1.1"_embed;
		if (!StringUtils::Equals((PCCHAR)buffer, (PCCHAR)expected))
		{
			LOG_ERROR("ToString mismatch: got '%s', expected '%s'", buffer, (PCCHAR)expected);
			return false;
		}
		return true;
	}

	static BOOL TestFromStringIPv6()
	{
		auto ipStr = "2001:db8::1"_embed;
		auto result = IPAddress::FromString((PCCHAR)ipStr);
		if (!result)
			return false;
		auto& ip = result.Value();
		if (!ip.IsIPv6())
			return false;
		if (ip.IsIPv4())
			return false;
		return true;
	}

	static BOOL TestFromStringInvalid()
	{
		auto inv1 = "256.1.1.1"_embed;
		if (IPAddress::FromString((PCCHAR)inv1))
			return false;

		auto inv2 = "192.168.1"_embed;
		if (IPAddress::FromString((PCCHAR)inv2))
			return false;

		auto inv3 = "abc.def.ghi.jkl"_embed;
		if (IPAddress::FromString((PCCHAR)inv3))
			return false;

		if (IPAddress::FromString(nullptr))
			return false;

		return true;
	}

	static BOOL TestIPv6Equality()
	{
		auto str1 = "2001:db8::1"_embed;
		auto str2 = "2001:db8::1"_embed;
		auto str3 = "2001:db8::2"_embed;

		auto r1 = IPAddress::FromString((PCCHAR)str1);
		auto r2 = IPAddress::FromString((PCCHAR)str2);
		auto r3 = IPAddress::FromString((PCCHAR)str3);
		if (!r1 || !r2 || !r3)
			return false;

		if (!(r1.Value() == r2.Value()))
			return false;
		if (r1.Value() == r3.Value())
			return false;
		return true;
	}

public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running IPAddress Tests...");

		RunTest(allPassed, EMBED_FUNC(TestConstexprIPv4), "constexpr IPv4 construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestConstexprIPv6), "constexpr IPv6 construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestConstexprLocalHost), "constexpr LocalHost"_embed);
		RunTest(allPassed, EMBED_FUNC(TestConstexprEquality), "constexpr equality operators"_embed);
		RunTest(allPassed, EMBED_FUNC(TestConstexprCopy), "constexpr copy constructor"_embed);
		RunTest(allPassed, EMBED_FUNC(TestConstexprInvalid), "constexpr Invalid factory"_embed);
		RunTest(allPassed, EMBED_FUNC(TestFromStringIPv4), "FromString IPv4 + ToString round-trip"_embed);
		RunTest(allPassed, EMBED_FUNC(TestFromStringIPv6), "FromString IPv6"_embed);
		RunTest(allPassed, EMBED_FUNC(TestFromStringInvalid), "FromString rejects invalid input"_embed);
		RunTest(allPassed, EMBED_FUNC(TestIPv6Equality), "IPv6 equality comparison"_embed);

		if (allPassed)
			LOG_INFO("All IPAddress tests passed!");
		else
			LOG_ERROR("Some IPAddress tests failed!");

		return allPassed;
	}
};
