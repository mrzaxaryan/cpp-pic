#pragma once

#include "runtime.h"
#include "socket.h"
#include "network.h"
#include "tls.h"
#include "logger.h"

class TlsTests
{
private:
	static constexpr UINT32 TEST_SERVER_IP = 0x6333854F;
	static constexpr UINT16 TLS_PORT = 8443;

	// Test 1: TLS handshake and connection
	static BOOL TestTlsHandshake()
	{
		LOG_INFO("Test: TLS Handshake (port 8443)");

		TLSClient tlsClient("0y.wtf"_embed, TEST_SERVER_IP, TLS_PORT);

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
		LOG_INFO("Test: TLS Echo - Single Message");

		TLSClient tlsClient("0y.wtf"_embed, TEST_SERVER_IP, TLS_PORT);

		if (!tlsClient.Open())
		{
			LOG_ERROR("TLS handshake failed");
			return FALSE;
		}

		// Send test message
		auto message = "Hello from CPP-PIC TLS test!"_embed;
		UINT32 bytesSent = tlsClient.Write((PCVOID)(PCCHAR)message, message.Length);

		if (bytesSent != message.Length)
		{
			LOG_ERROR("Failed to send complete message (sent %d/%d bytes)", bytesSent, message.Length);
			tlsClient.Close();
			return FALSE;
		}

		LOG_INFO("Sent %d bytes via TLS", bytesSent);

		// Receive echo response
		CHAR buffer[256];
		Memory::Zero(buffer, sizeof(buffer));
		SSIZE bytesRead = tlsClient.Read(buffer, sizeof(buffer) - 1);

		if (bytesRead <= 0)
		{
			LOG_ERROR("Failed to receive echo response");
			tlsClient.Close();
			return FALSE;
		}

		LOG_INFO("Received echo response (%d bytes): %s", bytesRead, buffer);

		// Verify echo matches sent message
		if (bytesRead != (SSIZE)message.Length)
		{
			LOG_ERROR("Echo length mismatch: sent %d, received %d", message.Length, bytesRead);
			tlsClient.Close();
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
			tlsClient.Close();
			return FALSE;
		}

		LOG_INFO("TLS echo test passed - data matches");
		tlsClient.Close();
		return TRUE;
	}

	// Test 4: TLS echo test - multiple messages
	static BOOL TestTlsEchoMultiple()
	{
		LOG_INFO("Test: TLS Echo - Multiple Messages");

		TLSClient tlsClient("0y.wtf"_embed, TEST_SERVER_IP, TLS_PORT);

		if (!tlsClient.Open())
		{
			LOG_ERROR("TLS handshake failed");
			return FALSE;
		}

		// Test messages
		auto msg1 = "First message"_embed;
		auto msg2 = "Second message with more data!"_embed;
		auto msg3 = "Third and final message for testing."_embed;

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
		if (read1 != (SSIZE)msg1.Length)
		{
			LOG_ERROR("Echo 1 length mismatch: sent %d, received %d", msg1.Length, read1);
			tlsClient.Close();
			return FALSE;
		}
		LOG_INFO("Message 1 echo: %s", buffer1);

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
		if (read2 != (SSIZE)msg2.Length)
		{
			LOG_ERROR("Echo 2 length mismatch: sent %d, received %d", msg2.Length, read2);
			tlsClient.Close();
			return FALSE;
		}
		LOG_INFO("Message 2 echo: %s", buffer2);

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
		if (read3 != (SSIZE)msg3.Length)
		{
			LOG_ERROR("Echo 3 length mismatch: sent %d, received %d", msg3.Length, read3);
			tlsClient.Close();
			return FALSE;
		}
		LOG_INFO("Message 3 echo: %s", buffer3);

		LOG_INFO("Multiple message echo test passed");
		tlsClient.Close();
		return TRUE;
	}

	// Test 5: TLS echo test - varying data sizes
	static BOOL TestTlsEchoVariableSizes()
	{
		LOG_INFO("Test: TLS Echo - Variable Data Sizes");

		TLSClient tlsClient("0y.wtf"_embed, TEST_SERVER_IP, TLS_PORT);

		if (!tlsClient.Open())
		{
			LOG_ERROR("TLS handshake failed");
			return FALSE;
		}

		// Test case 1: Single byte
		auto msg1 = "A"_embed;
		UINT32 sent1 = tlsClient.Write((PCVOID)(PCCHAR)msg1, 1);
		if (sent1 != 1)
		{
			LOG_ERROR("Test case 1: Failed to send 1 byte");
			tlsClient.Close();
			return FALSE;
		}
		CHAR buffer1[256];
		Memory::Zero(buffer1, sizeof(buffer1));
		SSIZE received1 = tlsClient.Read(buffer1, sizeof(buffer1) - 1);
		if (received1 != 1 || buffer1[0] != 'A')
		{
			LOG_ERROR("Test case 1: Length or data mismatch");
			tlsClient.Close();
			return FALSE;
		}
		LOG_INFO("Test case 1: 1 byte echoed correctly");

		// Test case 2: Short message
		auto msg2 = "Hello"_embed;
		UINT32 sent2 = tlsClient.Write((PCVOID)(PCCHAR)msg2, 5);
		if (sent2 != 5)
		{
			LOG_ERROR("Test case 2: Failed to send 5 bytes");
			tlsClient.Close();
			return FALSE;
		}
		CHAR buffer2[256];
		Memory::Zero(buffer2, sizeof(buffer2));
		SSIZE received2 = tlsClient.Read(buffer2, sizeof(buffer2) - 1);
		if (received2 != 5)
		{
			LOG_ERROR("Test case 2: Length mismatch");
			tlsClient.Close();
			return FALSE;
		}
		BOOL match2 = TRUE;
		for (UINT32 j = 0; j < 5; j++)
		{
			if (buffer2[j] != msg2[j])
			{
				match2 = FALSE;
				break;
			}
		}
		if (!match2)
		{
			LOG_ERROR("Test case 2: Data mismatch");
			tlsClient.Close();
			return FALSE;
		}
		LOG_INFO("Test case 2: 5 bytes echoed correctly");

		// Test case 3: Medium message
		auto msg3 = "This is a medium length message that tests TLS echo functionality."_embed;
		UINT32 sent3 = tlsClient.Write((PCVOID)(PCCHAR)msg3, 68);
		if (sent3 != 68)
		{
			LOG_ERROR("Test case 3: Failed to send 68 bytes");
			tlsClient.Close();
			return FALSE;
		}
		CHAR buffer3[256];
		Memory::Zero(buffer3, sizeof(buffer3));
		SSIZE received3 = tlsClient.Read(buffer3, sizeof(buffer3) - 1);
		if (received3 != 68)
		{
			LOG_ERROR("Test case 3: Length mismatch");
			tlsClient.Close();
			return FALSE;
		}
		BOOL match3 = TRUE;
		for (UINT32 j = 0; j < 68; j++)
		{
			if (buffer3[j] != msg3[j])
			{
				match3 = FALSE;
				break;
			}
		}
		if (!match3)
		{
			LOG_ERROR("Test case 3: Data mismatch");
			tlsClient.Close();
			return FALSE;
		}
		LOG_INFO("Test case 3: 68 bytes echoed correctly");

		// Test case 4: Longer message
		auto msg4 = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"_embed;
		UINT32 sent4 = tlsClient.Write((PCVOID)(PCCHAR)msg4, 134);
		if (sent4 != 134)
		{
			LOG_ERROR("Test case 4: Failed to send 134 bytes");
			tlsClient.Close();
			return FALSE;
		}
		CHAR buffer4[256];
		Memory::Zero(buffer4, sizeof(buffer4));
		SSIZE received4 = tlsClient.Read(buffer4, sizeof(buffer4) - 1);
		if (received4 != 134)
		{
			LOG_ERROR("Test case 4: Length mismatch");
			tlsClient.Close();
			return FALSE;
		}
		BOOL match4 = TRUE;
		for (UINT32 j = 0; j < 134; j++)
		{
			if (buffer4[j] != msg4[j])
			{
				match4 = FALSE;
				break;
			}
		}
		if (!match4)
		{
			LOG_ERROR("Test case 4: Data mismatch");
			tlsClient.Close();
			return FALSE;
		}
		LOG_INFO("Test case 4: 134 bytes echoed correctly");

		LOG_INFO("Variable size echo test passed");
		tlsClient.Close();
		return TRUE;
	}

	// Test 6: TLS echo test - binary data
	static BOOL TestTlsEchoBinaryData()
	{
		LOG_INFO("Test: TLS Echo - Binary Data");

		TLSClient tlsClient("0y.wtf"_embed, TEST_SERVER_IP, TLS_PORT);

		if (!tlsClient.Open())
		{
			LOG_ERROR("TLS handshake failed");
			return FALSE;
		}

		// Create binary test data with all byte values
		UINT8 binaryData[256];
		for (UINT32 i = 0; i < 256; i++)
		{
			binaryData[i] = (UINT8)i;
		}

		UINT32 sent = tlsClient.Write((PCVOID)binaryData, 256);
		if (sent != 256)
		{
			LOG_ERROR("Failed to send binary data (sent %d/256 bytes)", sent);
			tlsClient.Close();
			return FALSE;
		}

		UINT8 recvBuffer[256];
		Memory::Zero(recvBuffer, sizeof(recvBuffer));
		SSIZE received = tlsClient.Read(recvBuffer, 256);

		if (received != 256)
		{
			LOG_ERROR("Binary data length mismatch: sent 256, received %d", received);
			tlsClient.Close();
			return FALSE;
		}

		// Verify binary data
		BOOL match = TRUE;
		for (UINT32 i = 0; i < 256; i++)
		{
			if (recvBuffer[i] != binaryData[i])
			{
				LOG_ERROR("Binary data mismatch at byte %d: expected 0x%02X, got 0x%02X", i, binaryData[i], recvBuffer[i]);
				match = FALSE;
				break;
			}
		}

		if (!match)
		{
			LOG_ERROR("Binary data verification failed");
			tlsClient.Close();
			return FALSE;
		}

		LOG_INFO("Binary data echo test passed - all 256 bytes matched");
		tlsClient.Close();
		return TRUE;
	}

	// Test 7: TLS multiple sequential connections
	static BOOL TestTlsMultipleConnections()
	{
		LOG_INFO("Test: TLS Multiple Sequential Connections");

		for (UINT32 i = 0; i < 3; i++)
		{
			TLSClient tlsClient("0y.wtf"_embed, TEST_SERVER_IP, TLS_PORT);

			if (!tlsClient.Open())
			{
				LOG_ERROR("Connection %d: TLS handshake failed", i + 1);
				return FALSE;
			}

			// Send test message
			auto message = "Connection test"_embed;
			UINT32 sent = tlsClient.Write((PCVOID)(PCCHAR)message, message.Length);

			if (sent != message.Length)
			{
				LOG_ERROR("Connection %d: Failed to send message", i + 1);
				tlsClient.Close();
				return FALSE;
			}

			// Receive echo
			CHAR buffer[64];
			Memory::Zero(buffer, sizeof(buffer));
			SSIZE received = tlsClient.Read(buffer, sizeof(buffer) - 1);

			if (received != (SSIZE)message.Length)
			{
				LOG_ERROR("Connection %d: Echo length mismatch", i + 1);
				tlsClient.Close();
				return FALSE;
			}

			LOG_INFO("Connection %d: TLS handshake and echo successful", i + 1);
			tlsClient.Close();
		}

		LOG_INFO("All sequential TLS connections successful");
		return TRUE;
	}

	// Test 8: TLS large data transfer
	static BOOL TestTlsLargeDataTransfer()
	{
		LOG_INFO("Test: TLS Large Data Transfer");

		TLSClient tlsClient("0y.wtf"_embed, TEST_SERVER_IP, TLS_PORT);

		if (!tlsClient.Open())
		{
			LOG_ERROR("TLS handshake failed");
			return FALSE;
		}

		// Create large message (1KB)
		CHAR largeMessage[1024];
		for (UINT32 i = 0; i < 1024; i++)
		{
			largeMessage[i] = 'A' + (i % 26); // Repeating alphabet pattern
		}

		UINT32 sent = tlsClient.Write((PCVOID)largeMessage, 1024);
		if (sent != 1024)
		{
			LOG_ERROR("Failed to send large message (sent %d/1024 bytes)", sent);
			tlsClient.Close();
			return FALSE;
		}

		LOG_INFO("Sent 1024 bytes via TLS");

		// Receive echo in chunks
		CHAR recvBuffer[1024];
		Memory::Zero(recvBuffer, sizeof(recvBuffer));
		UINT32 totalReceived = 0;

		while (totalReceived < 1024)
		{
			SSIZE received = tlsClient.Read(recvBuffer + totalReceived, 1024 - totalReceived);
			if (received <= 0)
			{
				LOG_ERROR("Read failed or connection closed (total received: %d/1024)", totalReceived);
				tlsClient.Close();
				return FALSE;
			}
			totalReceived += received;
			LOG_DEBUG("Received chunk: %d bytes (total: %d/1024)", received, totalReceived);
		}

		// Verify data
		BOOL match = TRUE;
		for (UINT32 i = 0; i < 1024; i++)
		{
			if (recvBuffer[i] != largeMessage[i])
			{
				LOG_ERROR("Data mismatch at byte %d", i);
				match = FALSE;
				break;
			}
		}

		if (!match)
		{
			LOG_ERROR("Large data transfer verification failed");
			tlsClient.Close();
			return FALSE;
		}

		LOG_INFO("Large data transfer test passed - 1024 bytes matched");
		tlsClient.Close();
		return TRUE;
	}

public:
	// Run all TLS tests
	static BOOL RunAll()
	{
		LOG_INFO("=== Starting TLS Tests ===");
		LOG_INFO("Test Server: 0y.wtf (79.133.51.99:443)");
		LOG_INFO("Protocol: TCP+TLS 1.3 Echo Server");

		UINT32 passed = 0;
		UINT32 total = 7;

		// Basic functionality tests
		if (TestTlsHandshake())
			passed++;
		if (TestTlsEchoSingle())
			passed++;
		if (TestTlsEchoMultiple())
			passed++;
		if (TestTlsEchoVariableSizes())
			passed++;
		if (TestTlsEchoBinaryData())
			passed++;
		if (TestTlsMultipleConnections())
			passed++;
		if (TestTlsLargeDataTransfer())
			passed++;


		BOOL allPassed = (passed == total);
		LOG_INFO("=== TLS Tests Complete: %d/%d passed ===", passed, total);
		return allPassed;
	}
};
