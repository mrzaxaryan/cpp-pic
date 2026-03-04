#pragma once

#include "runtime/runtime.h"
#include "tests.h"

// =============================================================================
// WebSocket Tests - WebSocketClient Implementation Validation
// Test Server: echo.websocket.org - WebSocket Echo Service
// =============================================================================

class WebSocketTests
{
private:
	// Test 1: Secure WebSocket connection (wss://)
	static BOOL TestSecureWebSocketConnection()
	{
		LOG_INFO("Test: Secure WebSocket Connection (wss://)");

		auto wssUrl = "wss://echo.websocket.org/"_embed;
		auto createResult = WebSocketClient::Create(wssUrl);
		if (!createResult)
		{
			LOG_ERROR("Secure WebSocket connection failed (error: %e)", createResult.Error());
			return false;
		}

		LOG_INFO("Secure WebSocket connection established successfully");
		(void)createResult.Value().Close();
		return true;
	}

	// Test 2: WebSocket text message echo (WebSocketOpcode::Text)
	static BOOL TestWebSocketTextEcho()
	{
		LOG_INFO("Test: WebSocket Text Echo");

		auto wssUrl = "wss://echo.websocket.org/"_embed;
		auto createResult = WebSocketClient::Create(wssUrl);
		if (!createResult)
		{
			LOG_ERROR("WebSocket connection failed (error: %e)", createResult.Error());
			return false;
		}
		WebSocketClient &wsClient = createResult.Value();

		// Note: echo.websocket.org sends an initial "Request served by..." message
		// We need to read and discard it before sending our test data
		auto initialMsg = wsClient.Read();
		if (initialMsg)
			LOG_INFO("Received initial server message (%d bytes), discarding", initialMsg.Value().Length);

		// Send text message
		auto testMessage = "Hello, WebSocket!"_embed;
		auto writeResult = wsClient.Write(Span<const CHAR>((PCCHAR)testMessage, testMessage.Length()), WebSocketOpcode::Text);

		if (!writeResult)
		{
			LOG_ERROR("Failed to send message (error: %e)", writeResult.Error());
			(void)wsClient.Close();
			return false;
		}

		// Receive echo response
		auto readResult = wsClient.Read();

		if (!readResult)
		{
			LOG_ERROR("Failed to receive echo response (error: %e)", readResult.Error());
			(void)wsClient.Close();
			return false;
		}

		WebSocketMessage &response = readResult.Value();

		if (response.Opcode != WebSocketOpcode::Text)
		{
			LOG_ERROR("Unexpected opcode: expected %d (TEXT), got %d", (UINT8)WebSocketOpcode::Text, (UINT8)response.Opcode);
			(void)wsClient.Close();
			return false;
		}

		// Verify echo matches sent message
		BOOL matches = (response.Length == testMessage.Length()) &&
					   (Memory::Compare(response.Data, (PCVOID)(PCCHAR)testMessage, testMessage.Length()) == 0);

		(void)wsClient.Close();

		if (!matches)
		{
			LOG_ERROR("Echo response does not match sent message");
			return false;
		}

		LOG_INFO("Text echo test passed");
		return true;
	}

	// Test 3: WebSocket binary message echo (WebSocketOpcode::Binary)
	static BOOL TestWebSocketBinaryEcho()
	{
		LOG_INFO("Test: WebSocket Binary Echo");

		auto wssUrl = "wss://echo.websocket.org/"_embed;
		auto createResult = WebSocketClient::Create(wssUrl);
		if (!createResult)
		{
			LOG_ERROR("WebSocket connection failed (error: %e)", createResult.Error());
			return false;
		}
		WebSocketClient &wsClient = createResult.Value();

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

		auto writeResult = wsClient.Write(Span<const CHAR>((PCHAR)binaryData, dataLength), WebSocketOpcode::Binary);

		if (!writeResult)
		{
			LOG_ERROR("Failed to send binary message (error: %e)", writeResult.Error());
			(void)wsClient.Close();
			return false;
		}

		LOG_INFO("Sent binary message (%d bytes)", writeResult.Value());

		// Receive echo response
		auto readResult = wsClient.Read();

		if (!readResult)
		{
			LOG_ERROR("Failed to receive echo response (error: %e)", readResult.Error());
			(void)wsClient.Close();
			return false;
		}

		WebSocketMessage &response = readResult.Value();

		if (response.Opcode != WebSocketOpcode::Binary)
		{
			LOG_ERROR("Unexpected opcode: expected %d (BINARY), got %d", (UINT8)WebSocketOpcode::Binary, (UINT8)response.Opcode);
			(void)wsClient.Close();
			return false;
		}

		LOG_INFO("Received binary echo (opcode: %d, length: %d)", (UINT8)response.Opcode, response.Length);

		// Verify echo matches sent data
		BOOL matches = (response.Length == dataLength) &&
					   (Memory::Compare(response.Data, (PCVOID)binaryData, dataLength) == 0);

		(void)wsClient.Close();

		if (!matches)
		{
			LOG_ERROR("Binary echo response does not match sent data");
			return false;
		}

		LOG_INFO("Binary echo test passed");
		return true;
	}

	// Test 4: Multiple sequential messages
	static BOOL TestMultipleMessages()
	{
		LOG_INFO("Test: Multiple Sequential Messages");

		auto wssUrl = "wss://echo.websocket.org/"_embed;
		auto createResult = WebSocketClient::Create(wssUrl);
		if (!createResult)
		{
			LOG_ERROR("WebSocket connection failed (error: %e)", createResult.Error());
			return false;
		}
		WebSocketClient &wsClient = createResult.Value();

		// Discard initial server message
		(void)wsClient.Read();

		// Test messages
		auto msg1 = "First message"_embed;
		auto msg2 = "Second message"_embed;
		auto msg3 = "Third message"_embed;

		// Send and receive message 1
		auto write1 = wsClient.Write(Span<const CHAR>((PCCHAR)msg1, msg1.Length()), WebSocketOpcode::Text);
		if (!write1)
		{
			LOG_ERROR("Failed to send message 1 (error: %e)", write1.Error());
			(void)wsClient.Close();
			return false;
		}

		auto read1 = wsClient.Read();
		if (!read1)
		{
			LOG_ERROR("Failed to receive echo for message 1 (error: %e)", read1.Error());
			(void)wsClient.Close();
			return false;
		}
		if (read1.Value().Length != msg1.Length())
		{
			LOG_ERROR("Echo length mismatch for message 1: expected %d, got %d", msg1.Length(), read1.Value().Length);
			(void)wsClient.Close();
			return false;
		}

		// Send and receive message 2
		auto write2 = wsClient.Write(Span<const CHAR>((PCCHAR)msg2, msg2.Length()), WebSocketOpcode::Text);
		if (!write2)
		{
			LOG_ERROR("Failed to send message 2 (error: %e)", write2.Error());
			(void)wsClient.Close();
			return false;
		}

		auto read2 = wsClient.Read();
		if (!read2)
		{
			LOG_ERROR("Failed to receive echo for message 2 (error: %e)", read2.Error());
			(void)wsClient.Close();
			return false;
		}
		if (read2.Value().Length != msg2.Length())
		{
			LOG_ERROR("Echo length mismatch for message 2: expected %d, got %d", msg2.Length(), read2.Value().Length);
			(void)wsClient.Close();
			return false;
		}

		// Send and receive message 3
		auto write3 = wsClient.Write(Span<const CHAR>((PCCHAR)msg3, msg3.Length()), WebSocketOpcode::Text);
		if (!write3)
		{
			LOG_ERROR("Failed to send message 3 (error: %e)", write3.Error());
			(void)wsClient.Close();
			return false;
		}

		auto read3 = wsClient.Read();
		if (!read3)
		{
			LOG_ERROR("Failed to receive echo for message 3 (error: %e)", read3.Error());
			(void)wsClient.Close();
			return false;
		}
		if (read3.Value().Length != msg3.Length())
		{
			LOG_ERROR("Echo length mismatch for message 3: expected %d, got %d", msg3.Length(), read3.Value().Length);
			(void)wsClient.Close();
			return false;
		}

		LOG_INFO("Multiple message test passed");
		(void)wsClient.Close();
		return true;
	}

	// Test 5: Large message handling
	static BOOL TestLargeMessage()
	{
		LOG_INFO("Test: Large Message Handling");

		auto wssUrl = "wss://echo.websocket.org/"_embed;
		auto createResult = WebSocketClient::Create(wssUrl);
		if (!createResult)
		{
			LOG_ERROR("WebSocket connection failed (error: %e)", createResult.Error());
			return false;
		}
		WebSocketClient &wsClient = createResult.Value();

		// Discard initial server message
		(void)wsClient.Read();

		// Create a large message (1KB)
		UINT32 largeMessageSize = 1024;
		PCHAR largeMessage = new CHAR[largeMessageSize + 1];
		if (!largeMessage)
		{
			LOG_ERROR("Failed to allocate memory for large message");
			(void)wsClient.Close();
			return false;
		}

		// Fill with pattern
		for (UINT32 i = 0; i < largeMessageSize; i++)
		{
			largeMessage[i] = 'A' + (i % 26);
		}
		largeMessage[largeMessageSize] = '\0';

		// Send large message
		auto writeResult = wsClient.Write(Span<const CHAR>(largeMessage, largeMessageSize), WebSocketOpcode::Text);

		if (!writeResult)
		{
			LOG_ERROR("Failed to send large message (error: %e)", writeResult.Error());
			delete[] largeMessage;
			(void)wsClient.Close();
			return false;
		}

		LOG_INFO("Sent large message (%d bytes)", writeResult.Value());

		// Receive echo response
		auto readResult = wsClient.Read();

		if (!readResult)
		{
			LOG_ERROR("Failed to receive large echo response (error: %e)", readResult.Error());
			delete[] largeMessage;
			(void)wsClient.Close();
			return false;
		}

		WebSocketMessage &response = readResult.Value();

		LOG_INFO("Received large echo response (opcode: %d, length: %d)", (UINT8)response.Opcode, response.Length);

		// Verify echo matches sent message
		BOOL matches = (response.Length == largeMessageSize) &&
					   (Memory::Compare(response.Data, (PCVOID)largeMessage, largeMessageSize) == 0);

		delete[] largeMessage;
		(void)wsClient.Close();

		if (!matches)
		{
			LOG_ERROR("Large echo response does not match sent message");
			return false;
		}

		LOG_INFO("Large message test passed");
		return true;
	}

	// Test 6: WebSocket close handshake
	static BOOL TestWebSocketClose()
	{
		LOG_INFO("Test: WebSocket Close Handshake");

		auto wssUrl = "wss://echo.websocket.org/"_embed;
		auto createResult = WebSocketClient::Create(wssUrl);
		if (!createResult)
		{
			LOG_ERROR("WebSocket connection failed (error: %e)", createResult.Error());
			return false;
		}
		WebSocketClient &wsClient = createResult.Value();

		LOG_INFO("WebSocket connected, initiating close handshake");

		if (!wsClient.Close())
		{
			LOG_ERROR("WebSocket close handshake failed");
			return false;
		}

		LOG_INFO("WebSocket closed successfully");
		return true;
	}

public:
	// Run all WebSocket tests
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running WebSocket Tests...");
		LOG_INFO("  Test Server: echo.websocket.org (wss://)");

		RunTest(allPassed, EMBED_FUNC(TestSecureWebSocketConnection), "Secure WebSocket connection (wss://)"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWebSocketTextEcho), "WebSocket text echo"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWebSocketBinaryEcho), "WebSocket binary echo"_embed);
		RunTest(allPassed, EMBED_FUNC(TestMultipleMessages), "Multiple messages"_embed);
		RunTest(allPassed, EMBED_FUNC(TestLargeMessage), "Large message"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWebSocketClose), "WebSocket close"_embed);

		if (allPassed)
			LOG_INFO("All WebSocket tests passed!");
		else
			LOG_ERROR("Some WebSocket tests failed!");

		return allPassed;
	}
};
