#pragma once

#include "runtime.h"
#include "tests.h"

// =============================================================================
// WebSocket Tests - WebSocketClient Implementation Validation
// Test Server: echo.websocket.org - WebSocket Echo Service
// =============================================================================

class WebSocketTests
{
private:
	// Test 1: WebSocket client creation and URL parsing
	static BOOL TestWebSocketCreation()
	{
		LOG_INFO("Test: WebSocket Client Creation");

		auto wsUrl = "ws://echo.websocket.org/"_embed;
		WebSocketClient wsClient((PCCHAR)wsUrl);

		LOG_INFO("WebSocket client created successfully");
		return TRUE;
	}

	// Test 2: WebSocket connection with explicit DNS resolution
	static BOOL TestWebSocketConnectionWithDns()
	{
		LOG_INFO("Test: WebSocket Connection with Explicit DNS");

		// Force IPv4 resolution since CI environments may not have IPv6 connectivity
		auto domain = "echo.websocket.org"_embed;
		IPAddress resolvedIp = DNS::CloudflareResolve((PCCHAR)domain, A);
		if (resolvedIp .IsValid() == FALSE)
		{
			LOG_ERROR("DNS resolution failed for %s", (PCCHAR)domain);
			LOG_ERROR("WebSocket tests require network connectivity");
			return FALSE;
		}
		LOG_INFO("DNS resolved: %s successfully (IPv4)", (PCCHAR)domain);

		// Use hardcoded IPv4 to avoid IPv6 connectivity issues in CI
		auto wssUrl = "wss://echo.websocket.org/"_embed;
		auto ipv4 = "66.241.124.119"_embed;
		WebSocketClient wsClient((PCCHAR)wssUrl, (PCCHAR)ipv4);

		if (!wsClient.Open())
		{
			LOG_ERROR("WebSocket handshake failed - check if echo.websocket.org is accessible");
			return FALSE;
		}

		LOG_INFO("WebSocket connection established successfully");
		wsClient.Close();
		return TRUE;
	}

	// Test 3: Basic secure WebSocket connection and handshake (wss://)
	static BOOL TestSecureWebSocketConnection()
	{
		LOG_INFO("Test: Basic Secure WebSocket Connection (wss://)");

		auto wssUrl = "wss://echo.websocket.org/"_embed;
		auto ipv4 = "66.241.124.119"_embed;
		WebSocketClient wsClient((PCCHAR)wssUrl, (PCCHAR)ipv4);

		if (!wsClient.Open())
		{
			LOG_ERROR("Secure WebSocket handshake failed");
			return FALSE;
		}

		LOG_INFO("Secure WebSocket connection established successfully");
		wsClient.Close();
		return TRUE;
	}

	// Test 4: WebSocket text message echo (OPCODE_TEXT)
	static BOOL TestWebSocketTextEcho()
	{
		LOG_INFO("Test: WebSocket Text Echo");

		auto wssUrl = "wss://echo.websocket.org/"_embed;
		auto ipv4 = "66.241.124.119"_embed;
		WebSocketClient wsClient((PCCHAR)wssUrl, (PCCHAR)ipv4);

		if (!wsClient.Open())
		{
			LOG_ERROR("WebSocket connection failed");
			return FALSE;
		}

		// Note: echo.websocket.org sends an initial "Request served by..." message
		// We need to read and discard it before sending our test data
		USIZE initialLength = 0;
		INT8 initialOpcode = 0;
		PVOID initialMsg = wsClient.Read(&initialLength, &initialOpcode);
		if (initialMsg != NULL)
		{
			LOG_INFO("Received initial server message (%d bytes), discarding", initialLength);
			delete[] (PCHAR)initialMsg;
		}

		// Send text message
		auto testMessage = "Hello, WebSocket!"_embed;
		UINT32 bytesSent = wsClient.Write((PCVOID)(PCCHAR)testMessage, testMessage.Length(), OPCODE_TEXT);

		if (bytesSent != testMessage.Length())
		{
			LOG_ERROR("Failed to send complete message (sent %d/%d bytes)", bytesSent, testMessage.Length());
			wsClient.Close();
			return FALSE;
		}

		// Receive echo response
		USIZE responseLength = 0;
		INT8 opcode = 0;
		PVOID response = wsClient.Read(&responseLength, &opcode);

		if (response == NULL || responseLength == 0)
		{
			LOG_ERROR("Failed to receive echo response");
			wsClient.Close();
			return FALSE;
		}

		if (opcode != OPCODE_TEXT)
		{
			LOG_ERROR("Unexpected opcode: expected %d (TEXT), got %d", OPCODE_TEXT, opcode);
			delete[] (PCHAR)response;
			wsClient.Close();
			return FALSE;
		}

		// Verify echo matches sent message
		BOOL matches = (responseLength == testMessage.Length()) &&
		               (Memory::Compare(response, (PCVOID)(PCCHAR)testMessage, testMessage.Length()) == 0);

		delete[] (PCHAR)response;
		wsClient.Close();

		if (!matches)
		{
			LOG_ERROR("Echo response does not match sent message");
			return FALSE;
		}

		LOG_INFO("Text echo test passed");
		return TRUE;
	}

	// Test 5: WebSocket binary message echo (OPCODE_BINARY)
	static BOOL TestWebSocketBinaryEcho()
	{
		LOG_INFO("Test: WebSocket Binary Echo");

		auto wssUrl = "wss://echo.websocket.org/"_embed;
		auto ipv4 = "66.241.124.119"_embed;
		WebSocketClient wsClient((PCCHAR)wssUrl, (PCCHAR)ipv4);

		if (!wsClient.Open())
		{
			LOG_ERROR("WebSocket connection failed");
			return FALSE;
		}

		// Discard initial server message
		USIZE initialLength = 0;
		INT8 initialOpcode = 0;
		PVOID initialMsg = wsClient.Read(&initialLength, &initialOpcode);
		if (initialMsg != NULL)
		{
			delete[] (PCHAR)initialMsg;
		}

		// Send binary message - generate data at runtime to avoid .rdata
		UINT8 binaryData[11];
		for (UINT32 i = 0; i < 5; i++)
		{
			binaryData[i] = (UINT8)(i + 1);
		}
		for (UINT32 i = 5; i < 11; i++)
		{
			binaryData[i] = (UINT8)(0xAA + ((i - 5) * 0x11));
		}
		UINT32 dataLength = sizeof(binaryData);

		UINT32 bytesSent = wsClient.Write((PCVOID)binaryData, dataLength, OPCODE_BINARY);

		if (bytesSent != dataLength)
		{
			LOG_ERROR("Failed to send complete binary message (sent %d/%d bytes)", bytesSent, dataLength);
			wsClient.Close();
			return FALSE;
		}

		LOG_INFO("Sent binary message (%d bytes)", bytesSent);

		// Receive echo response
		USIZE responseLength = 0;
		INT8 opcode = 0;
		PVOID response = wsClient.Read(&responseLength, &opcode);

		if (response == NULL || responseLength == 0)
		{
			LOG_ERROR("Failed to receive echo response");
			wsClient.Close();
			return FALSE;
		}

		if (opcode != OPCODE_BINARY)
		{
			LOG_ERROR("Unexpected opcode: expected %d (BINARY), got %d", OPCODE_BINARY, opcode);
			delete[] (PCHAR)response;
			wsClient.Close();
			return FALSE;
		}

		LOG_INFO("Received binary echo (opcode: %d, length: %d)", opcode, responseLength);

		// Verify echo matches sent data
		BOOL matches = (responseLength == dataLength) &&
		               (Memory::Compare(response, (PCVOID)binaryData, dataLength) == 0);

		delete[] (PCHAR)response;
		wsClient.Close();

		if (!matches)
		{
			LOG_ERROR("Binary echo response does not match sent data");
			return FALSE;
		}

		LOG_INFO("Binary echo test passed");
		return TRUE;
	}

	// Test 6: Multiple sequential messages
	static BOOL TestMultipleMessages()
	{
		LOG_INFO("Test: Multiple Sequential Messages");

		auto wssUrl = "wss://echo.websocket.org/"_embed;
		auto ipv4 = "66.241.124.119"_embed;
		WebSocketClient wsClient((PCCHAR)wssUrl, (PCCHAR)ipv4);

		if (!wsClient.Open())
		{
			LOG_ERROR("WebSocket connection failed");
			return FALSE;
		}

		// Discard initial server message
		USIZE initialLength = 0;
		INT8 initialOpcode = 0;
		PVOID initialMsg = wsClient.Read(&initialLength, &initialOpcode);
		if (initialMsg != NULL)
		{
			delete[] (PCHAR)initialMsg;
		}

		// Test messages
		auto msg1 = "First message"_embed;
		auto msg2 = "Second message"_embed;
		auto msg3 = "Third message"_embed;

		// Send and receive message 1
		UINT32 sent1 = wsClient.Write((PCVOID)(PCCHAR)msg1, msg1.Length(), OPCODE_TEXT);
		if (sent1 != msg1.Length())
		{
			LOG_ERROR("Failed to send message 1");
			wsClient.Close();
			return FALSE;
		}

		USIZE len1 = 0;
		INT8 op1 = 0;
		PVOID resp1 = wsClient.Read(&len1, &op1);
		if (resp1 == NULL || len1 != msg1.Length())
		{
			LOG_ERROR("Failed to receive echo for message 1");
			if (resp1) delete[] (PCHAR)resp1;
			wsClient.Close();
			return FALSE;
		}
		delete[] (PCHAR)resp1;

		// Send and receive message 2
		UINT32 sent2 = wsClient.Write((PCVOID)(PCCHAR)msg2, msg2.Length(), OPCODE_TEXT);
		if (sent2 != msg2.Length())
		{
			LOG_ERROR("Failed to send message 2");
			wsClient.Close();
			return FALSE;
		}

		USIZE len2 = 0;
		INT8 op2 = 0;
		PVOID resp2 = wsClient.Read(&len2, &op2);
		if (resp2 == NULL || len2 != msg2.Length())
		{
			LOG_ERROR("Failed to receive echo for message 2");
			if (resp2) delete[] (PCHAR)resp2;
			wsClient.Close();
			return FALSE;
		}
		delete[] (PCHAR)resp2;

		// Send and receive message 3
		UINT32 sent3 = wsClient.Write((PCVOID)(PCCHAR)msg3, msg3.Length(), OPCODE_TEXT);
		if (sent3 != msg3.Length())
		{
			LOG_ERROR("Failed to send message 3");
			wsClient.Close();
			return FALSE;
		}

		USIZE len3 = 0;
		INT8 op3 = 0;
		PVOID resp3 = wsClient.Read(&len3, &op3);
		if (resp3 == NULL || len3 != msg3.Length())
		{
			LOG_ERROR("Failed to receive echo for message 3");
			if (resp3) delete[] (PCHAR)resp3;
			wsClient.Close();
			return FALSE;
		}
		delete[] (PCHAR)resp3;

		LOG_INFO("Multiple message test passed");
		wsClient.Close();
		return TRUE;
	}

	// Test 7: Large message handling
	static BOOL TestLargeMessage()
	{
		LOG_INFO("Test: Large Message Handling");

		auto wssUrl = "wss://echo.websocket.org/"_embed;
		auto ipv4 = "66.241.124.119"_embed;
		WebSocketClient wsClient((PCCHAR)wssUrl, (PCCHAR)ipv4);

		if (!wsClient.Open())
		{
			LOG_ERROR("WebSocket connection failed");
			return FALSE;
		}

		// Discard initial server message
		USIZE initialLength = 0;
		INT8 initialOpcode = 0;
		PVOID initialMsg = wsClient.Read(&initialLength, &initialOpcode);
		if (initialMsg != NULL)
		{
			delete[] (PCHAR)initialMsg;
		}

		// Create a large message (1KB)
		UINT32 largeMessageSize = 1024;
		PCHAR largeMessage = new CHAR[largeMessageSize + 1];
		if (!largeMessage)
		{
			LOG_ERROR("Failed to allocate memory for large message");
			wsClient.Close();
			return FALSE;
		}

		// Fill with pattern
		for (UINT32 i = 0; i < largeMessageSize; i++)
		{
			largeMessage[i] = 'A' + (i % 26);
		}
		largeMessage[largeMessageSize] = '\0';

		// Send large message
		UINT32 bytesSent = wsClient.Write((PCVOID)largeMessage, largeMessageSize, OPCODE_TEXT);

		if (bytesSent != largeMessageSize)
		{
			LOG_ERROR("Failed to send complete large message (sent %d/%d bytes)", bytesSent, largeMessageSize);
			delete[] largeMessage;
			wsClient.Close();
			return FALSE;
		}

		LOG_INFO("Sent large message (%d bytes)", bytesSent);

		// Receive echo response
		USIZE responseLength = 0;
		INT8 opcode = 0;
		PVOID response = wsClient.Read(&responseLength, &opcode);

		if (response == NULL || responseLength == 0)
		{
			LOG_ERROR("Failed to receive large echo response");
			delete[] largeMessage;
			wsClient.Close();
			return FALSE;
		}

		LOG_INFO("Received large echo response (opcode: %d, length: %d)", opcode, responseLength);

		// Verify echo matches sent message
		BOOL matches = (responseLength == largeMessageSize) &&
		               (Memory::Compare(response, (PCVOID)largeMessage, largeMessageSize) == 0);

		delete[] largeMessage;
		delete[] (PCHAR)response;
		wsClient.Close();

		if (!matches)
		{
			LOG_ERROR("Large echo response does not match sent message");
			return FALSE;
		}

		LOG_INFO("Large message test passed");
		return TRUE;
	}

	// Test 8: WebSocket close handshake
	static BOOL TestWebSocketClose()
	{
		LOG_INFO("Test: WebSocket Close Handshake");

		auto wssUrl = "wss://echo.websocket.org/"_embed;
		auto ipv4 = "66.241.124.119"_embed;
		WebSocketClient wsClient((PCCHAR)wssUrl, (PCCHAR)ipv4);

		if (!wsClient.Open())
		{
			LOG_ERROR("WebSocket connection failed");
			return FALSE;
		}

		LOG_INFO("WebSocket connected, initiating close handshake");

		if (!wsClient.Close())
		{
			LOG_ERROR("WebSocket close handshake failed");
			return FALSE;
		}

		LOG_INFO("WebSocket closed successfully");
		return TRUE;
	}

public:
	// Run all WebSocket tests
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		LOG_INFO("Running WebSocket Tests...");
		LOG_INFO("  Test Server: echo.websocket.org (wss://)");

		RUN_TEST(allPassed, TestWebSocketCreation, "WebSocket client creation");
		RUN_TEST(allPassed, TestWebSocketConnectionWithDns, "WebSocket connection with DNS");
		RUN_TEST(allPassed, TestSecureWebSocketConnection, "Secure WebSocket connection");
		RUN_TEST(allPassed, TestWebSocketTextEcho, "WebSocket text echo");
		RUN_TEST(allPassed, TestWebSocketBinaryEcho, "WebSocket binary echo");
		RUN_TEST(allPassed, TestMultipleMessages, "Multiple messages");
		RUN_TEST(allPassed, TestLargeMessage, "Large message");
		RUN_TEST(allPassed, TestWebSocketClose, "WebSocket close");

		if (allPassed)
			LOG_INFO("All WebSocket tests passed!");
		else
			LOG_ERROR("Some WebSocket tests failed!");

		return allPassed;
	}
};
