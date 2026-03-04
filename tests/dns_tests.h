#pragma once

#include "runtime/runtime.h"
#include "core/types/embedded/embedded_string.h"
#include "tests.h"

class DnsTests
{
private:
	// Test 1: Localhost resolution
	static BOOL TestLocalhostResolution()
	{
		LOG_INFO("Test: Localhost Resolution");

		auto result = DNS::CloudflareResolve("localhost"_embed, DnsRecordType::A);
		if (!result)
		{
			LOG_ERROR("Localhost A resolution failed (error: %e)", result.Error());
			return false;
		}
		auto& ip = result.Value();

		// localhost should resolve to 127.0.0.1 = 0x7F000001 in network byte order = 0x0100007F
		if (ip.ToIPv4() != 0x0100007F)
		{
			LOG_ERROR("Localhost resolution failed: expected 0x0100007F, got 0x%08X", ip.ToIPv4());
			return false;
		}

		auto result6 = DNS::CloudflareResolve("localhost"_embed, DnsRecordType::AAAA);
		if (!result6)
		{
			LOG_ERROR("Localhost AAAA resolution failed (error: %e)", result6.Error());
			return false;
		}
		auto& ip6 = result6.Value();

		// localhost should resolve to ::1 for IPv6
		UINT8 expectedIPv6[16]{};
		expectedIPv6[15] = 1; // ::1 in IPv6
		if (ip6.IsIPv6() == false || Memory::Compare(ip6.ToIPv6(), expectedIPv6, 16) != 0)
		{
			LOG_ERROR("Localhost IPv6 resolution failed: expected ::1, got different address");
			return false;
		}

		LOG_INFO("Localhost resolved correctly");
		return true;
	}

	// Test 2: Cloudflare DNS resolution
	static BOOL TestCloudflareResolve()
	{
		LOG_INFO("Test: Cloudflare DNS Resolution (dns.google)");

		auto result = DNS::CloudflareResolve("dns.google"_embed, DnsRecordType::A);
		if (!result)
		{
			LOG_ERROR("Cloudflare DNS resolution failed (error: %e)", result.Error());
			return false;
		}
		auto& ip = result.Value();

		// dns.google should resolve to 8.8.8.8 or 8.8.4.4
		// 8.8.8.8 in network byte order = 0x08080808
		// 8.8.4.4 in network byte order = 0x04040808
		if (ip.ToIPv4() != 0x08080808 && ip.ToIPv4() != 0x04040808)
		{
			LOG_ERROR("Unexpected IP for dns.google: 0x%08X", ip.ToIPv4());
			return false;
		}

		LOG_INFO("Cloudflare resolved dns.google to 0x%08X", ip.ToIPv4());
		return true;
	}

	// Test 3: Google DNS resolution
	static BOOL TestGoogleResolve()
	{
		LOG_INFO("Test: Google DNS Resolution (one.one.one.one)");

		auto result = DNS::GoogleResolve("one.one.one.one"_embed, DnsRecordType::A);
		if (!result)
		{
			LOG_ERROR("Google DNS resolution failed (error: %e)", result.Error());
			return false;
		}
		auto& ip = result.Value();

		// one.one.one.one should resolve to 1.1.1.1 or 1.0.0.1
		// 1.1.1.1 in network byte order = 0x01010101
		// 1.0.0.1 in network byte order = 0x01000001
		if (ip.ToIPv4() != 0x01010101 && ip.ToIPv4() != 0x01000001)
		{
			LOG_ERROR("Unexpected IP for one.one.one.one: 0x%08X", ip.ToIPv4());
			return false;
		}

		LOG_INFO("Google resolved one.one.one.one to 0x%08X", ip.ToIPv4());
		return true;
	}

	// Test 4: Main DNS Resolve function (tries IPv6 first, falls back to IPv4)
	static BOOL TestMainResolve()
	{
		LOG_INFO("Test: Main DNS Resolve Function");

		auto result = DNS::Resolve("example.com"_embed);
		if (!result)
		{
			LOG_ERROR("Main DNS resolution failed (error: %e)", result.Error());
			return false;
		}

		// example.com has both IPv4 and IPv6, so this may return either
		LOG_INFO("Main Resolve resolved example.com successfully");
		return true;
	}

	// Test 5: Resolution with known static IP (IPv6 first, falls back to IPv4)
	static BOOL TestKnownIpResolution()
	{
		LOG_INFO("Test: Known IP Resolution (dns.google)");

		auto result = DNS::Resolve("dns.google"_embed);
		if (!result)
		{
			LOG_ERROR("DNS resolution for dns.google failed (error: %e)", result.Error());
			return false;
		}

		// dns.google has both IPv4 and IPv6 addresses, so accept either
		LOG_INFO("Known IP resolution passed: dns.google resolved successfully");
		return true;
	}

public:
	// Run all DNS tests
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running DNS Tests...");
		LOG_INFO("  Testing DNS resolution via DoH (binary wireformat)");

		RunTest(allPassed, EMBED_FUNC(TestLocalhostResolution), "Localhost resolution"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCloudflareResolve), "Cloudflare DNS resolution"_embed);
		RunTest(allPassed, EMBED_FUNC(TestGoogleResolve), "Google DNS resolution"_embed);
		RunTest(allPassed, EMBED_FUNC(TestMainResolve), "Main DNS resolve function"_embed);
		RunTest(allPassed, EMBED_FUNC(TestKnownIpResolution), "Known IP resolution"_embed);

		if (allPassed)
			LOG_INFO("All DNS tests passed!");
		else
			LOG_ERROR("Some DNS tests failed!");

		return allPassed;
	}
};
