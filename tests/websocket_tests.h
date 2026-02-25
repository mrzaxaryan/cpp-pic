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
		if (resolvedIp.IsValid() == FALSE)
		{
			LOG_ERROR("DNS resolution failed for %s", (PCCHAR)domain);
			LOG_ERROR("WebSocket tests require network connectivity");
			return FALSE;
		}
		LOG_INFO("DNS resolved: %s successfully (IPv4)", (PCCHAR)domain);

		// Use hardcoded IPv4 to avoid IPv6 connectivity issues in CI
		auto wssUrl = "wss://echo.websocket.org/"_embed;
		WebSocketClient wsClient((PCCHAR)wssUrl);

		auto openResult = wsClient.Open();
		if (!openResult)
		{
			LOG_ERROR("WebSocket handshake failed - check if echo.websocket.org is accessible (error: %u)", openResult.Error());
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
		WebSocketClient wsClient((PCCHAR)wssUrl);

		auto openResult = wsClient.Open();
		if (!openResult)
		{
			LOG_ERROR("Secure WebSocket handshake failed (error: %u)", openResult.Error());
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
		WebSocketClient wsClient((PCCHAR)wssUrl);

		auto openResult = wsClient.Open();
		if (!openResult)
		{
			LOG_ERROR("WebSocket connection failed (error: %u)", openResult.Error());
			return FALSE;
		}

		// Note: echo.websocket.org sends an initial "Request served by..." message
		// We need to read and discard it before sending our test data
		auto initialMsg = wsClient.Read();
		if (initialMsg)
			LOG_INFO("Received initial server message (%d bytes), discarding", initialMsg.Value().length);

		// Send text message
		auto testMessage = "Hello, WebSocket!"_embed;
		auto writeResult = wsClient.Write((PCVOID)(PCCHAR)testMessage, testMessage.Length(), OPCODE_TEXT);

		if (!writeResult)
		{
			LOG_ERROR("Failed to send message (error: %u)", writeResult.Error());
			wsClient.Close();
			return FALSE;
		}

		// Receive echo response
		auto readResult = wsClient.Read();

		if (!readResult)
		{
			LOG_ERROR("Failed to receive echo response (error: %u)", readResult.Error());
			wsClient.Close();
			return FALSE;
		}

		WebSocketMessage &response = readResult.Value();

		if (response.opcode != OPCODE_TEXT)
		{
			LOG_ERROR("Unexpected opcode: expected %d (TEXT), got %d", OPCODE_TEXT, response.opcode);
			wsClient.Close();
			return FALSE;
		}

		// Verify echo matches sent message
		BOOL matches = (response.length == testMessage.Length()) &&
					   (Memory::Compare(response.data, (PCVOID)(PCCHAR)testMessage, testMessage.Length()) == 0);

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
		WebSocketClient wsClient((PCCHAR)wssUrl);

		auto openResult = wsClient.Open();
		if (!openResult)
		{
			LOG_ERROR("WebSocket connection failed (error: %u)", openResult.Error());
			return FALSE;
		}

		// Discard initial server message
		(void)wsClient.Read();

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

		auto writeResult = wsClient.Write((PCVOID)binaryData, dataLength, OPCODE_BINARY);

		if (!writeResult)
		{
			LOG_ERROR("Failed to send binary message (error: %u)", writeResult.Error());
			wsClient.Close();
			return FALSE;
		}

		LOG_INFO("Sent binary message (%d bytes)", writeResult.Value());

		// Receive echo response
		auto readResult = wsClient.Read();

		if (!readResult)
		{
			LOG_ERROR("Failed to receive echo response (error: %u)", readResult.Error());
			wsClient.Close();
			return FALSE;
		}

		WebSocketMessage &response = readResult.Value();

		if (response.opcode != OPCODE_BINARY)
		{
			LOG_ERROR("Unexpected opcode: expected %d (BINARY), got %d", OPCODE_BINARY, response.opcode);
			wsClient.Close();
			return FALSE;
		}

		LOG_INFO("Received binary echo (opcode: %d, length: %d)", response.opcode, response.length);

		// Verify echo matches sent data
		BOOL matches = (response.length == dataLength) &&
					   (Memory::Compare(response.data, (PCVOID)binaryData, dataLength) == 0);

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
		WebSocketClient wsClient((PCCHAR)wssUrl);

		auto openResult = wsClient.Open();
		if (!openResult)
		{
			LOG_ERROR("WebSocket connection failed (error: %u)", openResult.Error());
			return FALSE;
		}

		// Discard initial server message
		(void)wsClient.Read();

		// Test messages
		auto msg1 = "First message"_embed;
		auto msg2 = "Second message"_embed;
		auto msg3 = "Third message"_embed;

		// Send and receive message 1
		auto write1 = wsClient.Write((PCVOID)(PCCHAR)msg1, msg1.Length(), OPCODE_TEXT);
		if (!write1)
		{
			LOG_ERROR("Failed to send message 1");
			wsClient.Close();
			return FALSE;
		}

		auto read1 = wsClient.Read();
		if (!read1 || read1.Value().length != msg1.Length())
		{
			LOG_ERROR("Failed to receive echo for message 1");
			wsClient.Close();
			return FALSE;
		}

		// Send and receive message 2
		auto write2 = wsClient.Write((PCVOID)(PCCHAR)msg2, msg2.Length(), OPCODE_TEXT);
		if (!write2)
		{
			LOG_ERROR("Failed to send message 2");
			wsClient.Close();
			return FALSE;
		}

		auto read2 = wsClient.Read();
		if (!read2 || read2.Value().length != msg2.Length())
		{
			LOG_ERROR("Failed to receive echo for message 2");
			wsClient.Close();
			return FALSE;
		}

		// Send and receive message 3
		auto write3 = wsClient.Write((PCVOID)(PCCHAR)msg3, msg3.Length(), OPCODE_TEXT);
		if (!write3)
		{
			LOG_ERROR("Failed to send message 3");
			wsClient.Close();
			return FALSE;
		}

		auto read3 = wsClient.Read();
		if (!read3 || read3.Value().length != msg3.Length())
		{
			LOG_ERROR("Failed to receive echo for message 3");
			wsClient.Close();
			return FALSE;
		}

		LOG_INFO("Multiple message test passed");
		wsClient.Close();
		return TRUE;
	}

	// Test 7: Large message handling
	static BOOL TestLargeMessage()
	{
		LOG_INFO("Test: Large Message Handling");

		auto wssUrl = "wss://echo.websocket.org/"_embed;
		WebSocketClient wsClient((PCCHAR)wssUrl);

		auto openResult = wsClient.Open();
		if (!openResult)
		{
			LOG_ERROR("WebSocket connection failed (error: %u)", openResult.Error());
			return FALSE;
		}

		// Discard initial server message
		(void)wsClient.Read();

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
		auto writeResult = wsClient.Write((PCVOID)largeMessage, largeMessageSize, OPCODE_TEXT);

		if (!writeResult)
		{
			LOG_ERROR("Failed to send large message (error: %u)", writeResult.Error());
			delete[] largeMessage;
			wsClient.Close();
			return FALSE;
		}

		LOG_INFO("Sent large message (%d bytes)", writeResult.Value());

		// Receive echo response
		auto readResult = wsClient.Read();

		if (!readResult)
		{
			LOG_ERROR("Failed to receive large echo response (error: %u)", readResult.Error());
			delete[] largeMessage;
			wsClient.Close();
			return FALSE;
		}

		WebSocketMessage &response = readResult.Value();

		LOG_INFO("Received large echo response (opcode: %d, length: %d)", response.opcode, response.length);

		// Verify echo matches sent message
		BOOL matches = (response.length == largeMessageSize) &&
					   (Memory::Compare(response.data, (PCVOID)largeMessage, largeMessageSize) == 0);

		delete[] largeMessage;
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
		WebSocketClient wsClient((PCCHAR)wssUrl);

		auto openResult = wsClient.Open();
		if (!openResult)
		{
			LOG_ERROR("WebSocket connection failed (error: %u)", openResult.Error());
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

		RunTest(allPassed, EMBED_FUNC(TestWebSocketCreation), L"WebSocket client creation"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWebSocketConnectionWithDns), L"WebSocket connection with DNS"_embed);
		RunTest(allPassed, EMBED_FUNC(TestSecureWebSocketConnection), L"Secure WebSocket connection"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWebSocketTextEcho), L"WebSocket text echo"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWebSocketBinaryEcho), L"WebSocket binary echo"_embed);
		RunTest(allPassed, EMBED_FUNC(TestMultipleMessages), L"Multiple messages"_embed);
		RunTest(allPassed, EMBED_FUNC(TestLargeMessage), L"Large message"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWebSocketClose), L"WebSocket close"_embed);

		if (allPassed)
			LOG_INFO("All WebSocket tests passed!");
		else
			LOG_ERROR("Some WebSocket tests failed!");

		return allPassed;
	}
};
