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
	TlsClient tlsContext;
	BOOL isConnected;

	[[nodiscard]] Result<void, Error> ReceiveRestrict(Span<CHAR> buffer);
	[[nodiscard]] Result<void, Error> ReceiveFrame(WebSocketFrame &frame);
	static VOID MaskFrame(WebSocketFrame &frame, UINT32 maskKey);

	// Private constructor — only used by Create()
	WebSocketClient(const CHAR (&host)[254], const CHAR (&parsedPath)[2048], const IPAddress &ip, UINT16 portNum, TlsClient &&tls)
		: ipAddress(ip), port(portNum), tlsContext(static_cast<TlsClient &&>(tls)), isConnected(false)
	{
		Memory::Copy(hostName, host, sizeof(hostName));
		Memory::Copy(path, parsedPath, sizeof(path));
	}

public:
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	// Placement new required by Result<WebSocketClient, Error>
	VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }

	~WebSocketClient() { if (IsValid()) { [[maybe_unused]] auto _ = Close(); } }

	WebSocketClient(const WebSocketClient &) = delete;
	WebSocketClient &operator=(const WebSocketClient &) = delete;

	WebSocketClient(WebSocketClient &&other) noexcept
		: ipAddress(other.ipAddress), port(other.port),
		  tlsContext(static_cast<TlsClient &&>(other.tlsContext)),
		  isConnected(other.isConnected)
	{
		Memory::Copy(hostName, other.hostName, sizeof(hostName));
		Memory::Copy(path, other.path, sizeof(path));
		other.port = 0;
		other.isConnected = false;
	}

	WebSocketClient &operator=(WebSocketClient &&other) noexcept
	{
		if (this != &other)
		{
			if (IsValid())
				(void)Close();
			Memory::Copy(hostName, other.hostName, sizeof(hostName));
			Memory::Copy(path, other.path, sizeof(path));
			ipAddress = other.ipAddress;
			port = other.port;
			tlsContext = static_cast<TlsClient &&>(other.tlsContext);
			isConnected = other.isConnected;
			other.port = 0;
			other.isConnected = false;
		}
		return *this;
	}

	// Factory — caller MUST check the result (enforced by [[nodiscard]])
	[[nodiscard]] static Result<WebSocketClient, Error> Create(PCCHAR url);

	BOOL IsValid() const { return tlsContext.IsValid(); }
	BOOL IsSecure() const { return tlsContext.IsSecure(); }
	BOOL IsConnected() const { return isConnected; }
	[[nodiscard]] Result<void, Error> Open();
	[[nodiscard]] Result<void, Error> Close();
	[[nodiscard]] Result<WebSocketMessage, Error> Read();
	[[nodiscard]] Result<UINT32, Error> Write(Span<const CHAR> buffer, WebSocketOpcode opcode = OPCODE_BINARY);
};
