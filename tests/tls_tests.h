#pragma once

#include "runtime/runtime.h"
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

		auto createResult = TlsClient::Create("one.one.one.one"_embed, IPAddress::FromIPv4(TEST_SERVER_IP), TLS_PORT);
		if (!createResult)
		{
			LOG_ERROR("TLS client creation failed (error: %e)", createResult.Error());
			return false;
		}
		TlsClient &tlsClient = createResult.Value();

		auto openResult = tlsClient.Open();
		if (!openResult)
		{
			LOG_ERROR("TLS handshake failed (error: %e)", openResult.Error());
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

		auto createResult = TlsClient::Create("www.one.one.one.one"_embed, IPAddress::FromIPv4(TEST_SERVER_IP), TLS_PORT);
		if (!createResult)
		{
			LOG_ERROR("TLS client creation failed (error: %e)", createResult.Error());
			return false;
		}
		TlsClient &tlsClient = createResult.Value();

		auto openResult = tlsClient.Open();
		if (!openResult)
		{
			LOG_ERROR("TLS handshake failed (error: %e)", openResult.Error());
			return false;
		}

		// Send test message
		auto message = "GET / HTTP/1.1\r\n"_embed
					   "Host: one.one.one.one\r\n"_embed
					   "Connection: close\r\n"_embed
					   "\r\n"_embed;
		auto writeResult = tlsClient.Write(Span<const CHAR>((PCCHAR)message, message.Length()));

		if (!writeResult)
		{
			LOG_ERROR("Failed to send message (error: %e)", writeResult.Error());
			(void)tlsClient.Close();
			return false;
		}
		if (writeResult.Value() != message.Length())
		{
			LOG_ERROR("Incomplete send (sent %d/%d bytes)", writeResult.Value(), message.Length());
			(void)tlsClient.Close();
			return false;
		}

		// Receive echo response
		CHAR buffer[128];
		Memory::Zero(buffer, sizeof(buffer));
		auto readResult = tlsClient.Read(Span<CHAR>(buffer, sizeof(buffer) - 1));

		if (!readResult)
		{
			LOG_ERROR("Failed to receive echo response (error: %e)", readResult.Error());
			(void)tlsClient.Close();
			return false;
		}
		if (readResult.Value() <= 0)
		{
			LOG_ERROR("Received zero bytes in echo response");
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

		auto createResult = TlsClient::Create("www.one.one.one.one"_embed, IPAddress::FromIPv4(TEST_SERVER_IP), TLS_PORT);
		if (!createResult)
		{
			LOG_ERROR("TLS client creation failed (error: %e)", createResult.Error());
			return false;
		}
		TlsClient &tlsClient = createResult.Value();

		auto openResult = tlsClient.Open();
		if (!openResult)
		{
			LOG_ERROR("TLS handshake failed (error: %e)", openResult.Error());
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
		auto send1 = tlsClient.Write(Span<const CHAR>((PCCHAR)msg1, msg1.Length()));
		if (!send1)
		{
			LOG_ERROR("Failed to send message 1 (error: %e)", send1.Error());
			(void)tlsClient.Close();
			return false;
		}
		if (send1.Value() != msg1.Length())
		{
			LOG_ERROR("Incomplete send for message 1 (%d/%d bytes)", send1.Value(), msg1.Length());
			(void)tlsClient.Close();
			return false;
		}

		CHAR buffer1[128];
		Memory::Zero(buffer1, sizeof(buffer1));
		auto recv1 = tlsClient.Read(Span<CHAR>(buffer1, sizeof(buffer1) - 1));
		if (!recv1)
		{
			LOG_ERROR("Failed to receive echo response for message 1 (error: %e)", recv1.Error());
			(void)tlsClient.Close();
			return false;
		}
		if (recv1.Value() <= 0)
		{
			LOG_ERROR("Received zero bytes for message 1");
			(void)tlsClient.Close();
			return false;
		}

		// Send and receive message 2
		auto send2 = tlsClient.Write(Span<const CHAR>((PCCHAR)msg2, msg2.Length()));
		if (!send2)
		{
			LOG_ERROR("Failed to send message 2 (error: %e)", send2.Error());
			(void)tlsClient.Close();
			return false;
		}
		if (send2.Value() != msg2.Length())
		{
			LOG_ERROR("Incomplete send for message 2 (%d/%d bytes)", send2.Value(), msg2.Length());
			(void)tlsClient.Close();
			return false;
		}

		CHAR buffer2[128];
		Memory::Zero(buffer2, sizeof(buffer2));
		auto recv2 = tlsClient.Read(Span<CHAR>(buffer2, sizeof(buffer2) - 1));
		if (!recv2)
		{
			LOG_ERROR("Failed to receive echo response for message 2 (error: %e)", recv2.Error());
			(void)tlsClient.Close();
			return false;
		}
		if (recv2.Value() <= 0)
		{
			LOG_ERROR("Received zero bytes for message 2");
			(void)tlsClient.Close();
			return false;
		}

		// Send and receive message 3
		auto send3 = tlsClient.Write(Span<const CHAR>((PCCHAR)msg3, msg3.Length()));
		if (!send3)
		{
			LOG_ERROR("Failed to send message 3 (error: %e)", send3.Error());
			(void)tlsClient.Close();
			return false;
		}
		if (send3.Value() != msg3.Length())
		{
			LOG_ERROR("Incomplete send for message 3 (%d/%d bytes)", send3.Value(), msg3.Length());
			(void)tlsClient.Close();
			return false;
		}

		CHAR buffer3[128];
		Memory::Zero(buffer3, sizeof(buffer3));
		auto recv3 = tlsClient.Read(Span<CHAR>(buffer3, sizeof(buffer3) - 1));
		if (!recv3)
		{
			LOG_ERROR("Failed to receive echo response for message 3 (error: %e)", recv3.Error());
			(void)tlsClient.Close();
			return false;
		}
		if (recv3.Value() <= 0)
		{
			LOG_ERROR("Received zero bytes for message 3");
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

		RunTest(allPassed, EMBED_FUNC(TestTlsHandshake), "TLS handshake"_embed);
		RunTest(allPassed, EMBED_FUNC(TestTlsEchoSingle), "TLS echo - single message"_embed);
		RunTest(allPassed, EMBED_FUNC(TestTlsEchoMultiple), "TLS echo - multiple messages"_embed);

		if (allPassed)
			LOG_INFO("All TLS tests passed!");
		else
			LOG_ERROR("Some TLS tests failed!");

		return allPassed;
	}
};
