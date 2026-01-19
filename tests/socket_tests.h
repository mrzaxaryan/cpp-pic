#pragma once

#include "runtime.h"
#include "socket.h"
#include "network.h"
#include "logger.h"

// =============================================================================
// Socket Tests - AFD Socket Implementation Validation
// Server: one.one.one.on (1.1.1.1)
// =============================================================================

class SocketTests
{
private:
	// Test server IP address: 1.1.1.1
	static constexpr UINT32 TEST_SERVER_IP = 0x01010101; // Network byte order

	// Test 1: Socket creation
	static BOOL TestSocketCreation()
	{
		LOG_INFO("Test: Socket Creation");

		// Create socket for connection to test server
		Socket sock(TEST_SERVER_IP, 80);

		if (!sock.IsValid())
		{
			LOG_ERROR("Socket creation failed");
			return FALSE;
		}

		LOG_INFO("Socket created successfully");
		sock.Close();
		return TRUE;
	}

	// Test 2: Socket connection to HTTP port
	static BOOL TestSocketConnection()
	{
		LOG_INFO("Test: Socket Connection (HTTP:80)");

		// Create socket and connect to test server port 80
		Socket sock(TEST_SERVER_IP, 80);

		if (!sock.IsValid())
		{
			LOG_ERROR("Socket creation failed");
			return FALSE;
		}

		if (!sock.Open())
		{
			LOG_ERROR("Socket connection failed");
			sock.Close();
			return FALSE;
		}

		LOG_INFO("Socket connected successfully to 0y.wtf:80");
		sock.Close();
		return TRUE;
	}

	// Test 3: HTTP GET request (port 80)
	static BOOL TestHttpRequest()
	{
		LOG_INFO("Test: HTTP GET Request (port 80)");

		// Connect to HTTP port
		Socket sock(TEST_SERVER_IP, 80);

		if (!sock.IsValid() || !sock.Open())
		{
			LOG_ERROR("Socket initialization or connection failed");
			return FALSE;
		}

		// Send HTTP GET request
		auto request = "GET / HTTP/1.1\r\nHost: 0y.wtf\r\nConnection: close\r\n\r\n"_embed;
		UINT32 bytesSent = sock.Write((PCVOID)(PCCHAR)request, request.Length);

		if (bytesSent != request.Length)
		{
			LOG_ERROR("Failed to send complete HTTP request (sent %d/%d bytes)", bytesSent, request.Length);
			sock.Close();
			return FALSE;
		}

		LOG_INFO("HTTP request sent successfully (%d bytes)", bytesSent);

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

		LOG_INFO("Received HTTP response (%d bytes)", bytesRead);
		LOG_DEBUG("Response preview: %.80s", buffer);

		sock.Close();
		return TRUE;
	}

	// Test 4: WebSocket connection (port 80)
	static BOOL TestWebSocketConnection()
	{
		LOG_INFO("Test: WebSocket Connection (WS:80)");

		// Connect to WebSocket port
		Socket sock(TEST_SERVER_IP, 80);

		if (!sock.IsValid() || !sock.Open())
		{
			LOG_ERROR("Socket initialization or connection failed");
			return FALSE;
		}

		// Send WebSocket upgrade request
		auto request = "GET / HTTP/1.1\r\n"
		               "Host: 0y.wtf\r\n"
		               "Upgrade: websocket\r\n"
		               "Connection: Upgrade\r\n"
		               "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
		               "Sec-WebSocket-Version: 13\r\n\r\n"_embed;
		UINT32 bytesSent = sock.Write((PCVOID)(PCCHAR)request, request.Length);

		if (bytesSent != request.Length)
		{
			LOG_ERROR("Failed to send WebSocket upgrade request");
			sock.Close();
			return FALSE;
		}

		LOG_INFO("WebSocket upgrade request sent (%d bytes)", bytesSent);

		// Receive upgrade response
		CHAR buffer[512];
		Memory::Zero(buffer, sizeof(buffer));
		SSIZE bytesRead = sock.Read(buffer, sizeof(buffer) - 1);

		if (bytesRead <= 0)
		{
			LOG_ERROR("Failed to receive WebSocket upgrade response");
			sock.Close();
			return FALSE;
		}

		LOG_INFO("Received WebSocket upgrade response (%d bytes)", bytesRead);
		sock.Close();
		return TRUE;
	}

	// Test 5: TCP Echo Server (port 8080)
	static BOOL TestTcpEcho()
	{
		LOG_INFO("Test: TCP Echo Server (port 8080)");

		// Connect to TCP echo port
		Socket sock(TEST_SERVER_IP, 8080);

		if (!sock.IsValid() || !sock.Open())
		{
			LOG_ERROR("Socket initialization or connection failed");
			return FALSE;
		}

		// Send test message
		auto message = "Hello from CPP-PIC TCP test!"_embed;
		UINT32 bytesSent = sock.Write((PCVOID)(PCCHAR)message, message.Length);

		if (bytesSent != message.Length)
		{
			LOG_ERROR("Failed to send complete message to echo server");
			sock.Close();
			return FALSE;
		}

		LOG_INFO("Sent %d bytes to echo server", bytesSent);

		// Receive echo response
		CHAR buffer[256];
		Memory::Zero(buffer, sizeof(buffer));
		SSIZE bytesRead = sock.Read(buffer, sizeof(buffer) - 1);

		if (bytesRead <= 0)
		{
			LOG_ERROR("Failed to receive echo response");
			sock.Close();
			return FALSE;
		}

		LOG_INFO("Received echo response (%d bytes): %s", bytesRead, buffer);

		// Verify echo matches sent message
		if (bytesRead != (SSIZE)message.Length)
		{
			LOG_ERROR("Echo length mismatch: sent %d, received %d", message.Length, bytesRead);
			sock.Close();
			return FALSE;
		}

		// Compare echoed data
		BOOL match = TRUE;
		for (UINT32 i = 0; i < message.Length; i++)
		{
			if (buffer[i] != message[i])
			{
				match = FALSE;
				break;
			}
		}

		if (!match)
		{
			LOG_ERROR("Echo data mismatch");
			sock.Close();
			return FALSE;
		}

		LOG_INFO("Echo test passed - data matches");
		sock.Close();
		return TRUE;
	}

	// Test 6: Multiple sequential connections
	static BOOL TestMultipleConnections()
	{
		LOG_INFO("Test: Multiple Sequential Connections");

		for (UINT32 i = 0; i < 3; i++)
		{
			Socket sock(TEST_SERVER_IP, 80);

			if (!sock.IsValid() || !sock.Open())
			{
				LOG_ERROR("Connection %d failed", i + 1);
				return FALSE;
			}

			// Send minimal HTTP request
			auto request = "GET / HTTP/1.0\r\n\r\n"_embed;
			UINT32 bytesSent = sock.Write((PCVOID)(PCCHAR)request, request.Length);

			if (bytesSent != request.Length)
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

			LOG_INFO("Connection %d successful (%d bytes received)", i + 1, bytesRead);
			sock.Close();
		}

		LOG_INFO("All sequential connections successful");
		return TRUE;
	}

	// Test 7: Large HTTP response handling
	static BOOL TestLargeResponse()
	{
		LOG_INFO("Test: Large HTTP Response Handling");

		Socket sock(TEST_SERVER_IP, 80);

		if (!sock.IsValid() || !sock.Open())
		{
			LOG_ERROR("Socket initialization or connection failed");
			return FALSE;
		}

		// Send HTTP request
		auto request = "GET / HTTP/1.1\r\nHost: 0y.wtf\r\nConnection: close\r\n\r\n"_embed;
		UINT32 bytesSent = sock.Write((PCVOID)(PCCHAR)request, request.Length);

		if (bytesSent != request.Length)
		{
			LOG_ERROR("Failed to send HTTP request");
			sock.Close();
			return FALSE;
		}

		// Read response in chunks
		CHAR buffer[1024];
		UINT32 totalBytesRead = 0;
		UINT32 chunkCount = 0;

		while (TRUE)
		{
			Memory::Zero(buffer, sizeof(buffer));
			SSIZE bytesRead = sock.Read(buffer, sizeof(buffer) - 1);

			if (bytesRead <= 0)
			{
				// End of stream or error
				break;
			}

			totalBytesRead += bytesRead;
			chunkCount++;

			if (chunkCount == 1)
			{
				LOG_DEBUG("First chunk (80 chars): %.80s", buffer);
			}
		}

		LOG_INFO("Received %d bytes in %d chunks", totalBytesRead, chunkCount);

		if (totalBytesRead == 0)
		{
			LOG_ERROR("No data received");
			sock.Close();
			return FALSE;
		}

		sock.Close();
		return TRUE;
	}

	// Test 8: IP address conversion
	static BOOL TestIpConversion()
	{
		LOG_INFO("Test: IP Address Conversion");

		// Test ConvertIP function with test server address
		auto ipStr = "79.133.51.99"_embed;
		UINT32 convertedIp = ConvertIP((PCCHAR)ipStr);

		if (convertedIp == INVALID_IPV4)
		{
			LOG_ERROR("IP conversion failed for valid IP");
			return FALSE;
		}

		if (convertedIp != TEST_SERVER_IP)
		{
			LOG_ERROR("IP conversion mismatch: expected 0x%08X, got 0x%08X", TEST_SERVER_IP, convertedIp);
			return FALSE;
		}

		LOG_INFO("IP conversion successful: %s -> 0x%08X", (PCCHAR)ipStr, convertedIp);

		// Test invalid IP addresses
		auto invalidIp1 = "256.1.1.1"_embed;
		UINT32 result1 = ConvertIP((PCCHAR)invalidIp1);
		if (result1 != INVALID_IPV4)
		{
			LOG_ERROR("Failed to reject invalid IP: %s", (PCCHAR)invalidIp1);
			return FALSE;
		}

		auto invalidIp2 = "192.168.1"_embed;
		UINT32 result2 = ConvertIP((PCCHAR)invalidIp2);
		if (result2 != INVALID_IPV4)
		{
			LOG_ERROR("Failed to reject invalid IP: %s", (PCCHAR)invalidIp2);
			return FALSE;
		}

		auto invalidIp3 = "abc.def.ghi.jkl"_embed;
		UINT32 result3 = ConvertIP((PCCHAR)invalidIp3);
		if (result3 != INVALID_IPV4)
		{
			LOG_ERROR("Failed to reject invalid IP: %s", (PCCHAR)invalidIp3);
			return FALSE;
		}

		LOG_INFO("Invalid IP rejection tests passed");
		return TRUE;
	}

public:
	// Run all socket tests
	static BOOL RunAll()
	{
		LOG_INFO("=== Starting Socket Tests ===");
		LOG_INFO("Test Server: 0y.wtf (79.133.51.99)");

		UINT32 passed = 0;
		UINT32 total = 8;

		if (TestSocketCreation())
			passed++;
		if (TestSocketConnection())
			passed++;
		if (TestHttpRequest())
			passed++;
		if (TestWebSocketConnection())
			passed++;
		if (TestTcpEcho())
			passed++;
		if (TestMultipleConnections())
			passed++;
		if (TestLargeResponse())
			passed++;
		if (TestIpConversion())
			passed++;

		LOG_INFO("=== Socket Tests Complete: %d/%d passed ===", passed, total);
		return (passed == total);
	}
};
