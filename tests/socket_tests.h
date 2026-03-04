#pragma once

#include "runtime/runtime.h"
#include "tests.h"

// =============================================================================
// Socket Tests - AFD Socket Implementation Validation
// Server: one.one.one.one (1.1.1.1) - Cloudflare Public DNS
// =============================================================================

class SocketTests
{
private:
// Test server IP address: 1.1.1.1 (one.one.one.one)
#define TEST_SERVER_IP 0x01010101

	// Test 1: Socket creation
	static BOOL TestSocketCreation()
	{
		LOG_INFO("Test: Socket Creation");

		auto createResult = Socket::Create(IPAddress::FromIPv4(TEST_SERVER_IP), 80);
		if (!createResult)
		{
			LOG_ERROR("Socket creation failed (error: %e)", createResult.Error());
			return false;
		}
		Socket &sock = createResult.Value();

		LOG_INFO("Socket created successfully");
		(void)sock.Close();
		return true;
	}

	// Test 2: Socket connection to HTTP port
	static BOOL TestSocketConnection()
	{
		LOG_INFO("Test: Socket Connection (HTTP:80)");

		auto createResult = Socket::Create(IPAddress::FromIPv4(TEST_SERVER_IP), 80);
		if (!createResult)
		{
			LOG_ERROR("Socket creation failed (error: %e)", createResult.Error());
			return false;
		}
		Socket &sock = createResult.Value();

		auto openResult = sock.Open();
		if (!openResult)
		{
			LOG_ERROR("Socket connection failed (error: %e)", openResult.Error());
			(void)sock.Close();
			return false;
		}

		LOG_INFO("Socket connected successfully to one.one.one.one:80");
		(void)sock.Close();
		return true;
	}

	// Test 3: HTTP GET request (port 80)
	static BOOL TestHttpRequest()
	{
		LOG_INFO("Test: HTTP GET Request (port 80)");

		auto createResult = Socket::Create(IPAddress::FromIPv4(TEST_SERVER_IP), 80);
		if (!createResult)
		{
			LOG_ERROR("Socket creation failed (error: %e)", createResult.Error());
			return false;
		}
		Socket &sock = createResult.Value();

		auto openResult = sock.Open();
		if (!openResult)
		{
			LOG_ERROR("Socket initialization or connection failed (error: %e)", openResult.Error());
			return false;
		}

		auto request = "GET / HTTP/1.1\r\nHost: one.one.one.one\r\nConnection: close\r\n\r\n"_embed;
		auto writeResult = sock.Write(Span<const CHAR>((PCCHAR)request, request.Length()));

		if (!writeResult)
		{
			LOG_ERROR("Failed to send HTTP request (error: %e)", writeResult.Error());
			(void)sock.Close();
			return false;
		}
		if (writeResult.Value() != request.Length())
		{
			LOG_ERROR("Incomplete HTTP request (sent %d/%d bytes)", writeResult.Value(), request.Length());
			(void)sock.Close();
			return false;
		}

		CHAR buffer[512];
		Memory::Zero(buffer, sizeof(buffer));
		auto readResult = sock.Read(Span<CHAR>(buffer, sizeof(buffer) - 1));

		if (!readResult || readResult.Value() <= 0)
		{
			if (!readResult)
				LOG_ERROR("Failed to receive HTTP response (error: %e)", readResult.Error());
			else
				LOG_ERROR("Failed to receive HTTP response (zero bytes)");
			(void)sock.Close();
			return false;
		}

		(void)sock.Close();
		return true;
	}

	// Test 4: Multiple sequential connections
	static BOOL TestMultipleConnections()
	{
		LOG_INFO("Test: Multiple Sequential Connections");

		for (UINT32 i = 0; i < 3; i++)
		{
			auto createResult = Socket::Create(IPAddress::FromIPv4(TEST_SERVER_IP), 80);
			if (!createResult)
			{
				LOG_ERROR("Connection %d: socket creation failed (error: %e)", i + 1, createResult.Error());
				return false;
			}
			Socket &sock = createResult.Value();

			auto openResult = sock.Open();
			if (!openResult)
			{
				LOG_ERROR("Connection %d failed (error: %e)", i + 1, openResult.Error());
				return false;
			}

			auto request = "GET / HTTP/1.0\r\n\r\n"_embed;
			auto writeResult = sock.Write(Span<const CHAR>((PCCHAR)request, request.Length()));

			if (!writeResult)
			{
				LOG_ERROR("Connection %d: failed to send request (error: %e)", i + 1, writeResult.Error());
				(void)sock.Close();
				return false;
			}
			if (writeResult.Value() != request.Length())
			{
				LOG_ERROR("Connection %d: incomplete send (%d/%d bytes)", i + 1, writeResult.Value(), request.Length());
				(void)sock.Close();
				return false;
			}

			CHAR buffer[128];
			Memory::Zero(buffer, sizeof(buffer));
			auto readResult = sock.Read(Span<CHAR>(buffer, sizeof(buffer) - 1));

			if (!readResult)
			{
				LOG_ERROR("Connection %d: failed to receive response (error: %e)", i + 1, readResult.Error());
				(void)sock.Close();
				return false;
			}
			if (readResult.Value() <= 0)
			{
				LOG_ERROR("Connection %d: received zero bytes", i + 1);
				(void)sock.Close();
				return false;
			}

			(void)sock.Close();
		}

		LOG_INFO("All sequential connections successful");
		return true;
	}

	// Test 5: IP address conversion
	static BOOL TestIpConversion()
	{
		LOG_INFO("Test: IP Address Conversion");

		auto ipStr = "1.1.1.1"_embed;
		auto convertedResult = IPAddress::FromString((PCCHAR)ipStr);

		if (!convertedResult)
		{
			LOG_ERROR("IP conversion failed for valid IP");
			return false;
		}
		IPAddress convertedIp = convertedResult.Value();

		if (convertedIp.ToIPv4() != TEST_SERVER_IP)
		{
			LOG_ERROR("IP conversion mismatch: expected 0x%08X, got 0x%08X", TEST_SERVER_IP, convertedIp.ToIPv4());
			return false;
		}

		LOG_INFO("IP conversion successful: %s -> 0x%08X", (PCCHAR)ipStr, convertedIp.ToIPv4());

		LOG_INFO("  [D1] Testing invalid IP: 256.1.1.1");
		auto invalidIp1 = "256.1.1.1"_embed;
		auto parseResult1 = IPAddress::FromString((PCCHAR)invalidIp1);
		if (parseResult1)
		{
			LOG_ERROR("Failed to reject invalid IP: %s", (PCCHAR)invalidIp1);
			return false;
		}

		LOG_INFO("  [D2] Testing invalid IP: 192.168.1");
		auto invalidIp2 = "192.168.1"_embed;
		auto parseResult2 = IPAddress::FromString((PCCHAR)invalidIp2);
		if (parseResult2)
		{
			LOG_ERROR("Failed to reject invalid IP: %s", (PCCHAR)invalidIp2);
			return false;
		}

		LOG_INFO("  [D3] Testing invalid IP: abc.def.ghi.jkl");
		auto invalidIp3 = "abc.def.ghi.jkl"_embed;
		auto parseResult3 = IPAddress::FromString((PCCHAR)invalidIp3);
		if (parseResult3)
		{
			LOG_ERROR("Failed to reject invalid IP: %s", (PCCHAR)invalidIp3);
			return false;
		}

		LOG_INFO("  [D4] Testing IPv6: 2001:db8::1");
		auto ipv6Str = "2001:db8::1"_embed;
		LOG_INFO("  [D5] Calling FromString");
		auto ipv6Result = IPAddress::FromString((PCCHAR)ipv6Str);
		LOG_INFO("  [D6] FromString done: ok=%d", (INT32)(BOOL)ipv6Result);
		if (!ipv6Result)
		{
			LOG_ERROR("IPv6 conversion failed for valid IPv6");
			return false;
		}
		IPAddress ipv6Address = ipv6Result.Value();
		if (!ipv6Address.IsIPv6())
		{
			LOG_ERROR("IPv6 conversion returned non-IPv6 address");
			return false;
		}

		LOG_INFO("  [D7] About to format with %%s arg");
		LOG_INFO("IPv6 conversion successful: %s", (PCCHAR)ipv6Str);
		LOG_INFO("Invalid IP rejection tests passed");
		return true;
	}

	// Test 6: IPv6 Socket connection
	static BOOL TestIPv6Connection()
	{
		LOG_INFO("Test: IPv6 Socket Connection (HTTP:80)");

		auto ipv6Str = "2606:4700:4700::1111"_embed;
		auto ipv6Result = IPAddress::FromString((PCCHAR)ipv6Str);

		if (!ipv6Result || !ipv6Result.Value().IsIPv6())
		{
			LOG_ERROR("Failed to parse IPv6 address: %s", (PCCHAR)ipv6Str);
			return false;
		}
		IPAddress ipv6Address = ipv6Result.Value();

		auto createResult = Socket::Create(ipv6Address, 80);
		if (!createResult)
		{
			LOG_WARNING("IPv6 socket creation failed (error: %e) (IPv6 may not be available)", createResult.Error());
			return true; // non-fatal: IPv6 may be unavailable
		}
		Socket &sock = createResult.Value();

		auto openResult = sock.Open();
		if (!openResult)
		{
			LOG_WARNING("IPv6 socket connection failed (error: %e) (IPv6 may not be available)", openResult.Error());
			(void)sock.Close();
			return true; // non-fatal: IPv6 may be unavailable
		}

		LOG_INFO("IPv6 socket connected successfully to %s:80", (PCCHAR)ipv6Str);

		auto request = "GET / HTTP/1.1\r\nHost: one.one.one.one\r\nConnection: close\r\n\r\n"_embed;
		auto writeResult = sock.Write(Span<const CHAR>((PCCHAR)request, request.Length()));

		if (!writeResult)
		{
			LOG_ERROR("Failed to send HTTP request over IPv6 (error: %e)", writeResult.Error());
			(void)sock.Close();
			return false;
		}
		if (writeResult.Value() != request.Length())
		{
			LOG_ERROR("Incomplete HTTP request over IPv6 (sent %d/%d bytes)", writeResult.Value(), request.Length());
			(void)sock.Close();
			return false;
		}

		CHAR buffer[512];
		Memory::Zero(buffer, sizeof(buffer));
		auto readResult = sock.Read(Span<CHAR>(buffer, sizeof(buffer) - 1));

		if (!readResult)
		{
			LOG_ERROR("Failed to receive HTTP response over IPv6 (error: %e)", readResult.Error());
			(void)sock.Close();
			return false;
		}
		if (readResult.Value() <= 0)
		{
			LOG_ERROR("Received zero bytes over IPv6");
			(void)sock.Close();
			return false;
		}

		(void)sock.Close();
		return true;
	}

	// Test 7: HTTP GET request to httpbin.org
	static BOOL TestHttpBin()
	{
		auto dnsResult = DNS::Resolve("httpbin.org"_embed);
		if (!dnsResult)
		{
			LOG_ERROR("Failed to resolve httpbin.org (error: %e)", dnsResult.Error());
			return false;
		}

		auto createResult = Socket::Create(dnsResult.Value(), 80);
		if (!createResult)
		{
			LOG_ERROR("Socket creation failed for httpbin.org (error: %e)", createResult.Error());
			return false;
		}
		Socket &sock = createResult.Value();
		auto openResult = sock.Open();
		if (!openResult)
		{
			LOG_ERROR("Failed to open socket to httpbin.org (error: %e)", openResult.Error());
			return false;
		}

		auto request = "GET /get HTTP/1.1\r\nHost: httpbin.org\r\nConnection: close\r\n\r\n"_embed;
		auto writeResult = sock.Write(Span<const CHAR>((PCCHAR)request, request.Length()));
		if (!writeResult)
		{
			LOG_ERROR("Failed to send HTTP request to httpbin.org (error: %e)", writeResult.Error());
			(void)sock.Close();
			return false;
		}
		if (writeResult.Value() != request.Length())
		{
			LOG_ERROR("Incomplete HTTP request to httpbin.org (sent %d/%d bytes)", writeResult.Value(), request.Length());
			(void)sock.Close();
			return false;
		}

		CHAR buffer[1024];
		Memory::Zero(buffer, sizeof(buffer));
		auto readResult = sock.Read(Span<CHAR>(buffer, sizeof(buffer) - 1));
		if (!readResult)
		{
			LOG_ERROR("Failed to receive HTTP response from httpbin.org (error: %e)", readResult.Error());
			(void)sock.Close();
			return false;
		}
		if (readResult.Value() <= 0)
		{
			LOG_ERROR("Received zero bytes from httpbin.org");
			(void)sock.Close();
			return false;
		}

		LOG_INFO("Received %d bytes from httpbin.org", readResult.Value());
		(void)sock.Close();
		return true;
	}

public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Socket Tests...");
		LOG_INFO("  Test Server: one.one.one.one (1.1.1.1 / 2606:4700:4700::1111)");

		RunTest(allPassed, EMBED_FUNC(TestSocketCreation), "Socket creation"_embed);
		RunTest(allPassed, EMBED_FUNC(TestSocketConnection), "Socket connection (HTTP:80)"_embed);
		RunTest(allPassed, EMBED_FUNC(TestHttpRequest), "HTTP GET request"_embed);
		RunTest(allPassed, EMBED_FUNC(TestMultipleConnections), "Multiple sequential connections"_embed);
		RunTest(allPassed, EMBED_FUNC(TestIpConversion), "IP address conversion"_embed);
		RunTest(allPassed, EMBED_FUNC(TestIPv6Connection), "IPv6 connection"_embed);
		RunTest(allPassed, EMBED_FUNC(TestHttpBin), "HTTP GET request to httpbin.org"_embed);

		if (allPassed)
			LOG_INFO("All Socket tests passed!");
		else
			LOG_ERROR("Some Socket tests failed!");

		return allPassed;
	}
};
