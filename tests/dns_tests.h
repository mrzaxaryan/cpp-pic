#pragma once

#include "ral.h"
#include "embedded_string.h"

class DnsTests
{
private:
	// Test 1: Localhost resolution
	static BOOL TestLocalhostResolution()
	{
		LOG_INFO("Test: Localhost Resolution");

		IPAddress ip = DNS::ResolveOverTls("localhost"_embed, A);

		// localhost should resolve to 127.0.0.1 = 0x7F000001 in network byte order = 0x0100007F
		if (ip.ToIPv4() != 0x0100007F)
		{
			LOG_ERROR("Localhost resolution failed: expected 0x0100007F, got 0x%08X", ip.ToIPv4());
			return FALSE;
		}

		LOG_INFO("Localhost resolved correctly to 127.0.0.1");
		return TRUE;
	}

	// Test 2: Cloudflare DNS resolution
	static BOOL TestCloudflareResolve()
	{
		LOG_INFO("Test: Cloudflare DNS Resolution (dns.google)");

		IPAddress ip = DNS::CloudflareResolve("dns.google"_embed, A);

		if (ip .IsValid() == FALSE)
		{
			LOG_ERROR("Cloudflare DNS resolution failed");
			return FALSE;
		}

		// dns.google should resolve to 8.8.8.8 or 8.8.4.4
		// 8.8.8.8 in network byte order = 0x08080808
		// 8.8.4.4 in network byte order = 0x04040808
		if (ip.ToIPv4() != 0x08080808 && ip.ToIPv4() != 0x04040808)
		{
			LOG_ERROR("Unexpected IP for dns.google: 0x%08X", ip.ToIPv4());
			return FALSE;
		}

		LOG_INFO("Cloudflare resolved dns.google to 0x%08X", ip.ToIPv4());
		return TRUE;
	}

	// Test 3: Google DNS resolution
	static BOOL TestGoogleResolve()
	{
		LOG_INFO("Test: Google DNS Resolution (one.one.one.one)");

		IPAddress ip = DNS::GoogleResolve("one.one.one.one"_embed, A);

		if (ip .IsValid() == FALSE)
		{
			LOG_ERROR("Google DNS resolution failed");
			return FALSE;
		}

		// one.one.one.one should resolve to 1.1.1.1 or 1.0.0.1
		// 1.1.1.1 in network byte order = 0x01010101
		// 1.0.0.1 in network byte order = 0x01000001
		if (ip.ToIPv4() != 0x01010101 && ip.ToIPv4() != 0x01000001)
		{
			LOG_ERROR("Unexpected IP for one.one.one.one: 0x%08X", ip.ToIPv4());
			return FALSE;
		}

		LOG_INFO("Google resolved one.one.one.one to 0x%08X", ip.ToIPv4());
		return TRUE;
	}

	// Test 4: DNS over TLS resolution
	static BOOL TestDnsOverTls()
	{
		LOG_INFO("Test: DNS over TLS Resolution");

		IPAddress ip = DNS::ResolveOverTls("cloudflare.com"_embed, A);

		if (ip .IsValid() == FALSE)
		{
			LOG_ERROR("DNS over TLS resolution failed");
			return FALSE;
		}

		LOG_INFO("DNS over TLS resolved cloudflare.com to 0x%08X", ip.ToIPv4());
		return TRUE;
	}

	// Test 5: DNS over HTTPS (JSON format) resolution
	static BOOL TestDnsOverHttps()
	{
		LOG_INFO("Test: DNS over HTTPS (JSON) Resolution");

		IPAddress ip = DNS::ResolveOverHttp("google.com"_embed, A);

		if (ip .IsValid() == FALSE)
		{
			LOG_ERROR("DNS over HTTPS resolution failed");
			return FALSE;
		}

		LOG_INFO("DNS over HTTPS resolved google.com to 0x%08X", ip.ToIPv4());
		return TRUE;
	}

	// Test 6: Main DNS Resolve function (tries IPv6 first, falls back to IPv4)
	static BOOL TestMainResolve()
	{
		LOG_INFO("Test: Main DNS Resolve Function");

		IPAddress ip = DNS::Resolve("example.com"_embed);

		if (ip .IsValid() == FALSE)
		{
			LOG_ERROR("Main DNS resolution failed");
			return FALSE;
		}

		// example.com has both IPv4 and IPv6, so this may return either
		LOG_INFO("Main Resolve resolved example.com successfully");
		return TRUE;
	}

	// Test 7: Resolution with known static IP (IPv6 first, falls back to IPv4)
	static BOOL TestKnownIpResolution()
	{
		LOG_INFO("Test: Known IP Resolution (dns.google)");

		IPAddress ip = DNS::Resolve("dns.google"_embed);

		if (ip .IsValid() == FALSE)
		{
			LOG_ERROR("DNS resolution for dns.google failed");
			return FALSE;
		}

		// dns.google has both IPv4 and IPv6 addresses, so accept either
		LOG_INFO("Known IP resolution passed: dns.google resolved successfully");
		return TRUE;
	}

public:
	// Run all DNS tests
	static BOOL RunAll()
	{
		LOG_INFO("=== Starting DNS Tests ===");
		LOG_INFO("Testing DNS resolution via DoT, DoH (JSON), and DoH (binary wireformat)");

		UINT32 passed = 0;
		UINT32 total = 7;

		// Run tests
		if (TestLocalhostResolution())
			passed++;
		if (TestCloudflareResolve())
			passed++;
		if (TestGoogleResolve())
			passed++;
		if (TestDnsOverTls())
			passed++;
		if (TestDnsOverHttps())
			passed++;
		if (TestMainResolve())
			passed++;
		if (TestKnownIpResolution())
			passed++;

		BOOL allPassed = (passed == total);
		LOG_INFO("=== DNS Tests Complete: %d/%d passed ===", passed, total);
		return allPassed;
	}
};
