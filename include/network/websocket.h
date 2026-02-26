#pragma once
#include "platform.h"
#include "tls.h"

enum WebSocketOpcode : INT8
{
	OPCODE_CONTINUE = 0x0,
	OPCODE_TEXT     = 0x1,
	OPCODE_BINARY   = 0x2,
	OPCODE_CLOSE    = 0x8,
	OPCODE_PING     = 0x9,
	OPCODE_PONG     = 0xA
};

struct WebSocketFrame
{
	PCHAR data;
	UINT64 length;
	WebSocketOpcode opcode;
	UINT8 fin;
	UINT8 mask;
	UINT8 rsv1;
	UINT8 rsv2;
	UINT8 rsv3;
};

struct WebSocketMessage
{
	PCHAR data;
	USIZE length;
	WebSocketOpcode opcode;

	WebSocketMessage() : data(nullptr), length(0), opcode(OPCODE_BINARY) {}

	~WebSocketMessage()
	{
		if (data)
		{
			delete[] data;
			data = nullptr;
		}
	}

	WebSocketMessage(const WebSocketMessage &) = delete;
	WebSocketMessage &operator=(const WebSocketMessage &) = delete;

	WebSocketMessage(WebSocketMessage &&other) noexcept
		: data(other.data), length(other.length), opcode(other.opcode)
	{
		other.data = nullptr;
		other.length = 0;
	}

	WebSocketMessage &operator=(WebSocketMessage &&other) noexcept
	{
		if (this != &other)
		{
			if (data)
				delete[] data;
			data = other.data;
			length = other.length;
			opcode = other.opcode;
			other.data = nullptr;
			other.length = 0;
		}
		return *this;
	}
};

class WebSocketClient
{
private:
	CHAR hostName[254];  // RFC 1035: max 253 chars + null
	CHAR path[2048];     // De facto max URL path length
	IPAddress ipAddress;
	UINT16 port;
	TLSClient tlsContext;
	BOOL isConnected;

	[[nodiscard]] Result<void, Error> ReceiveRestrict(PVOID buffer, UINT32 size);
	[[nodiscard]] Result<void, Error> ReceiveFrame(WebSocketFrame &frame);
	static VOID MaskFrame(WebSocketFrame &frame, UINT32 maskKey);

public:
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	WebSocketClient(PCCHAR url);
	~WebSocketClient() { if (IsValid()) { [[maybe_unused]] auto _ = Close(); } }

	WebSocketClient(const WebSocketClient &) = delete;
	WebSocketClient &operator=(const WebSocketClient &) = delete;
	WebSocketClient(WebSocketClient &&) = delete;
	WebSocketClient &operator=(WebSocketClient &&) = delete;

	[[nodiscard]] BOOL IsValid() const { return tlsContext.IsValid(); }
	[[nodiscard]] BOOL IsSecure() const { return tlsContext.IsSecure(); }
	[[nodiscard]] BOOL IsConnected() const { return isConnected; }
	[[nodiscard]] Result<void, Error> Open();
	[[nodiscard]] Result<void, Error> Close();
	[[nodiscard]] Result<WebSocketMessage, Error> Read();
	[[nodiscard]] Result<UINT32, Error> Write(PCVOID buffer, UINT32 bufferLength, WebSocketOpcode opcode = OPCODE_BINARY);
};
