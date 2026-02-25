#pragma once

#include "runtime.h"
#include "tests.h"

class TlsTests
{
private:
#define TEST_SERVER_IP 0x01010101
#define TLS_PORT 443

	// Test 1: TLS handshake and connection
	static BOOL TestTlsHandshake()
	{
		LOG_INFO("Test: TLS Handshake (ip: %x, port %d)", TEST_SERVER_IP, TLS_PORT);

		TLSClient tlsClient("one.one.one.one"_embed, IPAddress::FromIPv4(TEST_SERVER_IP), TLS_PORT);

		if (!tlsClient.Open())
		{
			LOG_ERROR("TLS handshake failed");
			return false;
		}

		LOG_INFO("TLS handshake completed successfully");
		(void)tlsClient.Close();
		return true;
	}

	// Test 3: TLS echo test - single message
	static BOOL TestTlsEchoSingle()
	{
		LOG_INFO("Test: TLS Echo - Single Message (ip: %x, port %d)", TEST_SERVER_IP, TLS_PORT);

		TLSClient tlsClient("www.one.one.one.one"_embed, IPAddress::FromIPv4(TEST_SERVER_IP), TLS_PORT);

		if (!tlsClient.Open())
		{
			LOG_ERROR("TLS handshake failed");
			return false;
		}

		// Send test message
		auto message = "GET / HTTP/1.1\r\n"_embed
					   "Host: one.one.one.one\r\n"_embed
					   "Connection: close\r\n"_embed
					   "\r\n"_embed;
		auto writeResult = tlsClient.Write((PCVOID)(PCCHAR)message, message.Length());

		if (!writeResult || writeResult.Value() != message.Length())
		{
			LOG_ERROR("Failed to send complete message (sent %d/%d bytes)",
			          writeResult ? writeResult.Value() : 0, message.Length());
			(void)tlsClient.Close();
			return false;
		}

		// Receive echo response
		CHAR buffer[128];
		Memory::Zero(buffer, sizeof(buffer));
		auto readResult = tlsClient.Read(buffer, sizeof(buffer) - 1);

		if (!readResult || readResult.Value() <= 0)
		{
			LOG_ERROR("Failed to receive echo response");
			(void)tlsClient.Close();
			return false;
		}

		LOG_INFO("TLS echo test passed");
		(void)tlsClient.Close();
		return true;
	}

	// Test 4: TLS echo test - multiple messages
	static BOOL TestTlsEchoMultiple()
	{
		LOG_INFO("Test: TLS Echo - Multiple Messages (port %d)", TLS_PORT);

		TLSClient tlsClient("www.one.one.one.one"_embed, IPAddress::FromIPv4(TEST_SERVER_IP), TLS_PORT);

		if (!tlsClient.Open())
		{
			LOG_ERROR("TLS handshake failed");
			return false;
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
		auto send1 = tlsClient.Write((PCVOID)(PCCHAR)msg1, msg1.Length());
		if (!send1 || send1.Value() != msg1.Length())
		{
			LOG_ERROR("Failed to send message 1");
			(void)tlsClient.Close();
			return false;
		}

		CHAR buffer1[128];
		Memory::Zero(buffer1, sizeof(buffer1));
		auto recv1 = tlsClient.Read(buffer1, sizeof(buffer1) - 1);
		if (!recv1 || recv1.Value() <= 0)
		{
			LOG_ERROR("Failed to receive echo response for message 1");
			(void)tlsClient.Close();
			return false;
		}

		// Send and receive message 2
		auto send2 = tlsClient.Write((PCVOID)(PCCHAR)msg2, msg2.Length());
		if (!send2 || send2.Value() != msg2.Length())
		{
			LOG_ERROR("Failed to send message 2");
			(void)tlsClient.Close();
			return false;
		}

		CHAR buffer2[128];
		Memory::Zero(buffer2, sizeof(buffer2));
		auto recv2 = tlsClient.Read(buffer2, sizeof(buffer2) - 1);
		if (!recv2 || recv2.Value() <= 0)
		{
			LOG_ERROR("Failed to receive echo response for message 2");
			(void)tlsClient.Close();
			return false;
		}

		// Send and receive message 3
		auto send3 = tlsClient.Write((PCVOID)(PCCHAR)msg3, msg3.Length());
		if (!send3 || send3.Value() != msg3.Length())
		{
			LOG_ERROR("Failed to send message 3");
			(void)tlsClient.Close();
			return false;
		}

		CHAR buffer3[128];
		Memory::Zero(buffer3, sizeof(buffer3));
		auto recv3 = tlsClient.Read(buffer3, sizeof(buffer3) - 1);
		if (!recv3 || recv3.Value() <= 0)
		{
			LOG_ERROR("Failed to receive echo response for message 3");
			(void)tlsClient.Close();
			return false;
		}

		LOG_INFO("Multiple message echo test passed");
		(void)tlsClient.Close();
		return true;
	}

public:
	// Run all TLS tests
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running TLS Tests...");
		LOG_INFO("  Test Server: one.one.one.one (1.1.1.1:443)");

		RunTest(allPassed, EMBED_FUNC(TestTlsHandshake), L"TLS handshake"_embed);
		RunTest(allPassed, EMBED_FUNC(TestTlsEchoSingle), L"TLS echo - single message"_embed);
		RunTest(allPassed, EMBED_FUNC(TestTlsEchoMultiple), L"TLS echo - multiple messages"_embed);

		if (allPassed)
			LOG_INFO("All TLS tests passed!");
		else
			LOG_ERROR("Some TLS tests failed!");

		return allPassed;
	}
};
