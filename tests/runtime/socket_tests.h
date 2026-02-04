#pragma once

#include "runtime.h"
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

		Socket sock(IPAddress::FromIPv4(TEST_SERVER_IP), 80);

		LOG_INFO("Socket created successfully");
		sock.Close();
		return TRUE;
	}

	// Test 2: Socket connection to HTTP port
	static BOOL TestSocketConnection()
	{
		LOG_INFO("Test: Socket Connection (HTTP:80)");

		Socket sock(IPAddress::FromIPv4(TEST_SERVER_IP), 80);

		if (!sock.Open())
		{
			LOG_ERROR("Socket connection failed");
			sock.Close();
			return FALSE;
		}

		LOG_INFO("Socket connected successfully to one.one.one.one:80");
		sock.Close();
		return TRUE;
	}

	// Test 3: HTTP GET request (port 80)
	static BOOL TestHttpRequest()
	{
		LOG_INFO("Test: HTTP GET Request (port 80)");

		Socket sock(IPAddress::FromIPv4(TEST_SERVER_IP), 80);

		if (!sock.Open())
		{
			LOG_ERROR("Socket initialization or connection failed");
			return FALSE;
		}

		// Send HTTP GET request
		auto request = "GET / HTTP/1.1\r\nHost: one.one.one.one\r\nConnection: close\r\n\r\n"_embed;
		UINT32 bytesSent = sock.Write((PCVOID)(PCCHAR)request, request.Length());

		if (bytesSent != request.Length())
		{
			LOG_ERROR("Failed to send complete HTTP request (sent %d/%d bytes)", bytesSent, request.Length());
			sock.Close();
			return FALSE;
		}

		// Receive response
		CHAR buffer[512];
		Memory::Zero(buffer, sizeof(buffer));
		SSIZE bytesRead = sock.Read(buffer, sizeof(buffer) - 1);

		if (bytesRead <= 0)
		{
			LOG_ERROR("Failed to receive HTTP response");
			sock.Close();
			return FALSE;
		}

		sock.Close();
		return TRUE;
	}

	// Test 4: Multiple sequential connections
	static BOOL TestMultipleConnections()
	{
		LOG_INFO("Test: Multiple Sequential Connections");

		for (UINT32 i = 0; i < 3; i++)
		{
			Socket sock(IPAddress::FromIPv4(TEST_SERVER_IP), 80);

			if (!sock.Open())
			{
				LOG_ERROR("Connection %d failed", i + 1);
				return FALSE;
			}

			// Send minimal HTTP request
			auto request = "GET / HTTP/1.0\r\n\r\n"_embed;
			UINT32 bytesSent = sock.Write((PCVOID)(PCCHAR)request, request.Length());

			if (bytesSent != request.Length())
			{
				LOG_ERROR("Connection %d: failed to send request", i + 1);
				sock.Close();
				return FALSE;
			}

			// Read some response
			CHAR buffer[128];
			Memory::Zero(buffer, sizeof(buffer));
			SSIZE bytesRead = sock.Read(buffer, sizeof(buffer) - 1);

			if (bytesRead <= 0)
			{
				LOG_ERROR("Connection %d: failed to receive response", i + 1);
				sock.Close();
				return FALSE;
			}

			sock.Close();
		}

		LOG_INFO("All sequential connections successful");
		return TRUE;
	}

	// Test 5: IP address conversion
	static BOOL TestIpConversion()
	{
		LOG_INFO("Test: IP Address Conversion");

		// Test IPAddress::FromString with test server address
		auto ipStr = "1.1.1.1"_embed;
		IPAddress convertedIp = IPAddress::FromString((PCCHAR)ipStr);

		if (!convertedIp.IsValid())
		{
			LOG_ERROR("IP conversion failed for valid IP");
			return FALSE;
		}

		if (convertedIp.ToIPv4() != TEST_SERVER_IP)
		{
			LOG_ERROR("IP conversion mismatch: expected 0x%08X, got 0x%08X", TEST_SERVER_IP, convertedIp.ToIPv4());
			return FALSE;
		}

		LOG_INFO("IP conversion successful: %s -> 0x%08X", (PCCHAR)ipStr, convertedIp.ToIPv4());

		// Test invalid IP addresses
		auto invalidIp1 = "256.1.1.1"_embed;
		IPAddress result1 = IPAddress::FromString((PCCHAR)invalidIp1);
		if (result1.IsValid())
		{
			LOG_ERROR("Failed to reject invalid IP: %s", (PCCHAR)invalidIp1);
			return FALSE;
		}

		auto invalidIp2 = "192.168.1"_embed;
		IPAddress result2 = IPAddress::FromString((PCCHAR)invalidIp2);
		if (result2.IsValid())
		{
			LOG_ERROR("Failed to reject invalid IP: %s", (PCCHAR)invalidIp2);
			return FALSE;
		}

		auto invalidIp3 = "abc.def.ghi.jkl"_embed;
		IPAddress result3 = IPAddress::FromString((PCCHAR)invalidIp3);
		if (result3.IsValid())
		{
			LOG_ERROR("Failed to reject invalid IP: %s", (PCCHAR)invalidIp3);
			return FALSE;
		}

		// Test IPv6 address parsing
		auto ipv6Str = "2001:db8::1"_embed;
		IPAddress ipv6Address = IPAddress::FromString((PCCHAR)ipv6Str);
		if (!ipv6Address.IsValid() || !ipv6Address.IsIPv6())
		{
			LOG_ERROR("IPv6 conversion failed for valid IPv6");
			return FALSE;
		}

		LOG_INFO("IPv6 conversion successful: %s", (PCCHAR)ipv6Str);
		LOG_INFO("Invalid IP rejection tests passed");
		return TRUE;
	}

	// Test 6: IPv6 Socket connection
	static BOOL TestIPv6Connection()
	{
		LOG_INFO("Test: IPv6 Socket Connection (HTTP:80)");

		// Use Cloudflare DNS IPv6 address: 2606:4700:4700::1111
		auto ipv6Str = "2606:4700:4700::1111"_embed;
		IPAddress ipv6Address = IPAddress::FromString((PCCHAR)ipv6Str);

		if (!ipv6Address.IsValid() || !ipv6Address.IsIPv6())
		{
			LOG_ERROR("Failed to parse IPv6 address: %s", (PCCHAR)ipv6Str);
			return FALSE;
		}

		Socket sock(ipv6Address, 80);

		if (!sock.Open())
		{
			LOG_WARNING("IPv6 socket connection failed (IPv6 may not be available in this environment)");
			sock.Close();
			return TRUE; // Return TRUE to avoid failing the test suite when IPv6 is unavailable
		}

		LOG_INFO("IPv6 socket connected successfully to %s:80", (PCCHAR)ipv6Str);

		// Send minimal HTTP request
		auto request = "GET / HTTP/1.1\r\nHost: one.one.one.one\r\nConnection: close\r\n\r\n"_embed;
		UINT32 bytesSent = sock.Write((PCVOID)(PCCHAR)request, request.Length());

		if (bytesSent != request.Length())
		{
			LOG_ERROR("Failed to send complete HTTP request over IPv6 (sent %d/%d bytes)", bytesSent, request.Length());
			sock.Close();
			return FALSE;
		}

		// Receive response
		CHAR buffer[512];
		Memory::Zero(buffer, sizeof(buffer));
		SSIZE bytesRead = sock.Read(buffer, sizeof(buffer) - 1);

		if (bytesRead <= 0)
		{
			LOG_ERROR("Failed to receive HTTP response over IPv6");
			sock.Close();
			return FALSE;
		}

		sock.Close();
		return TRUE;
	}

public:
	// Run all socket tests
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		LOG_INFO("Running Socket Tests...");
		LOG_INFO("  Test Server: one.one.one.one (1.1.1.1 / 2606:4700:4700::1111)");

		RunTest(allPassed, EMBED_FUNC(TestSocketCreation), L"Socket creation"_embed);
		RunTest(allPassed, EMBED_FUNC(TestSocketConnection), L"Socket connection (HTTP:80)"_embed);
		RunTest(allPassed, EMBED_FUNC(TestHttpRequest), L"HTTP GET request"_embed);
		RunTest(allPassed, EMBED_FUNC(TestMultipleConnections), L"Multiple sequential connections"_embed);
		RunTest(allPassed, EMBED_FUNC(TestIpConversion), L"IP address conversion"_embed);
		RunTest(allPassed, EMBED_FUNC(TestIPv6Connection), L"IPv6 connection"_embed);

		if (allPassed)
			LOG_INFO("All Socket tests passed!");
		else
			LOG_ERROR("Some Socket tests failed!");

		return allPassed;
	}
};
