#pragma once

#include "ral.h"

class TlsTests
{
private:
	static constexpr UINT32 TEST_SERVER_IP = 0x01010101; // 1.1.1.1
	static constexpr UINT16 TLS_PORT = 443;

	// Test 1: TLS handshake and connection
	static BOOL TestTlsHandshake()
	{
		LOG_INFO("Test: TLS Handshake (ip: %x, port %d)", TEST_SERVER_IP, TLS_PORT);

		TLSClient tlsClient("one.one.one.one"_embed, IPAddress::FromIPv4(TEST_SERVER_IP), TLS_PORT);

		if (!tlsClient.Open())
		{
			LOG_ERROR("TLS handshake failed");
			return FALSE;
		}

		LOG_INFO("TLS handshake completed successfully");
		tlsClient.Close();
		return TRUE;
	}

	// Test 3: TLS echo test - single message
	static BOOL TestTlsEchoSingle()
	{
		LOG_INFO("Test: TLS Echo - Single Message (ip: %x, port %d)", TEST_SERVER_IP, TLS_PORT);

		TLSClient tlsClient("www.one.one.one.one"_embed, IPAddress::FromIPv4(TEST_SERVER_IP), TLS_PORT);

		if (!tlsClient.Open())
		{
			LOG_ERROR("TLS handshake failed");
			return FALSE;
		}

		// Send test message
		auto message = "GET / HTTP/1.1\r\n"_embed
					   "Host: one.one.one.one\r\n"_embed
					   "Connection: close\r\n"_embed
					   "\r\n"_embed;
		UINT32 bytesSent = tlsClient.Write((PCVOID)(PCCHAR)message, message.Length);

		if (bytesSent != message.Length)
		{
			LOG_ERROR("Failed to send complete message (sent %d/%d bytes)", bytesSent, message.Length);
			tlsClient.Close();
			return FALSE;
		}

		// Receive echo response
		CHAR buffer[128];
		Memory::Zero(buffer, sizeof(buffer));
		SSIZE bytesRead = tlsClient.Read(buffer, sizeof(buffer) - 1);

		if (bytesRead <= 0)
		{
			LOG_ERROR("Failed to receive echo response");
			tlsClient.Close();
			return FALSE;
		}

		LOG_INFO("TLS echo test passed");
		tlsClient.Close();
		return TRUE;
	}

	// Test 4: TLS echo test - multiple messages
	static BOOL TestTlsEchoMultiple()
	{
		LOG_INFO("Test: TLS Echo - Multiple Messages (port %d)", TLS_PORT);

		TLSClient tlsClient("www.one.one.one.one"_embed, IPAddress::FromIPv4(TEST_SERVER_IP), TLS_PORT);

		if (!tlsClient.Open())
		{
			LOG_ERROR("TLS handshake failed");
			return FALSE;
		}

		// Test messages
		auto msg1 = "GET / HTTP/1.1\r\n"_embed
					"Host: one.one.one.one\r\n"_embed
					"\r\n"_embed;
		auto msg2 = "GET / HTTP/1.1\r\n"_embed
					"Host: one.one.one.one\r\n"_embed
					"\r\n"_embed;
		auto msg3 = "GET / HTTP/1.1\r\n"_embed
					"Host: one.one.one.one\r\n"_embed
					"Connection: close\r\n"_embed
					"\r\n"_embed;

		// Send and receive message 1
		UINT32 sent1 = tlsClient.Write((PCVOID)(PCCHAR)msg1, msg1.Length);
		if (sent1 != msg1.Length)
		{
			LOG_ERROR("Failed to send message 1");
			tlsClient.Close();
			return FALSE;
		}

		CHAR buffer1[128];
		Memory::Zero(buffer1, sizeof(buffer1));
		SSIZE read1 = tlsClient.Read(buffer1, sizeof(buffer1) - 1);
		if (read1 <= 0)
		{
			LOG_ERROR("Failed to receive echo response for message 1");
			tlsClient.Close();
			return FALSE;
		}

		// Send and receive message 2
		UINT32 sent2 = tlsClient.Write((PCVOID)(PCCHAR)msg2, msg2.Length);
		if (sent2 != msg2.Length)
		{
			LOG_ERROR("Failed to send message 2");
			tlsClient.Close();
			return FALSE;
		}

		CHAR buffer2[128];
		Memory::Zero(buffer2, sizeof(buffer2));
		SSIZE read2 = tlsClient.Read(buffer2, sizeof(buffer2) - 1);
		if (read2 <= 0)
		{
			LOG_ERROR("Failed to receive echo response for message 2");
			tlsClient.Close();
			return FALSE;
		}

		// Send and receive message 3
		UINT32 sent3 = tlsClient.Write((PCVOID)(PCCHAR)msg3, msg3.Length);
		if (sent3 != msg3.Length)
		{
			LOG_ERROR("Failed to send message 3");
			tlsClient.Close();
			return FALSE;
		}

		CHAR buffer3[128];
		Memory::Zero(buffer3, sizeof(buffer3));
		SSIZE read3 = tlsClient.Read(buffer3, sizeof(buffer3) - 1);
		if (read3 <= 0)
		{
			LOG_ERROR("Failed to receive echo response for message 3");
			tlsClient.Close();
			return FALSE;
		}

		LOG_INFO("Multiple message echo test passed");
		tlsClient.Close();
		return TRUE;
	}

public:
	// Run all TLS tests
	static BOOL RunAll()
	{
		LOG_INFO("=== Starting TLS Tests ===");
		LOG_INFO("Test Server: one.one.one.one (1.1.1.1:443)");
		LOG_INFO("Protocol: TCP+TLS 1.3 Echo Server");

		UINT32 passed = 0;
		UINT32 total = 3;

		// Basic functionality tests
		if (TestTlsHandshake())
			passed++;
		if (TestTlsEchoSingle())
			passed++;
		if (TestTlsEchoMultiple())
			passed++;

		BOOL allPassed = (passed == total);
		LOG_INFO("=== TLS Tests Complete: %d/%d passed ===", passed, total);
		return allPassed;
	}
};
