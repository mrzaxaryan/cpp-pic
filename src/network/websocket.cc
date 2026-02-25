#include "websocket.h"
#include "memory.h"
#include "string.h"
#include "random.h"
#include "logger.h"
#include "dns.h"
#include "http.h"
#include "embedded_string.h"

Result<void, WebSocketError> WebSocketClient::Open()
{
	BOOL isSecure = tlsContext.IsSecure();
	LOG_DEBUG("Opening WebSocket client to %s:%u%s (secure: %s)", hostName, port, path, isSecure ? "true"_embed : "false"_embed);

	BOOL result = tlsContext.Open();

	if (!result && ipAddress.IsIPv6())
	{
		LOG_DEBUG("Failed to open network transport for WebSocket client using IPv6 address, attempting IPv4 fallback");

		auto dnsResult = DNS::Resolve(hostName, A);
		if (!dnsResult)
		{
			LOG_ERROR("Failed to resolve IPv4 address for %s, cannot connect to WebSocket server", hostName);
			return Result<void, WebSocketError>::Err(WS_ERROR_DNS_FAILED);
		}

		ipAddress = dnsResult.Value();

		(void)tlsContext.Close();
		tlsContext = TLSClient(hostName, ipAddress, port, isSecure);
		result = tlsContext.Open();
	}

	if (!result)
	{
		LOG_DEBUG("Failed to open network transport for WebSocket client");
		return Result<void, WebSocketError>::Err(WS_ERROR_TRANSPORT_FAILED);
	}

	// Generate random 16-byte WebSocket key (RFC 6455: 16 random bytes, base64-encoded)
	UINT32 key[4];
	Random random;
	for (INT32 i = 0; i < 4; i++)
		key[i] = (UINT32)random.Get();

	CHAR secureKey[25]; // Base64 of 16 bytes = 24 chars + null
	Base64::Encode((PCHAR)key, 16, secureKey);

	auto writeStr = [&](PCCHAR s) -> BOOL
	{
		UINT32 len = String::Length(s);
		auto r = tlsContext.Write(s, len);
		return r && r.Value() == len;
	};

	if (!writeStr("GET "_embed) ||
		!writeStr(path) ||
		!writeStr(" HTTP/1.1\r\nHost: "_embed) ||
		!writeStr(hostName) ||
		!writeStr("\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: "_embed) ||
		!writeStr(secureKey) ||
		!writeStr("\r\nSec-WebSocket-Version: 13\r\nOrigin: "_embed) ||
		!writeStr(isSecure ? "https://"_embed : "http://"_embed) ||
		!writeStr(hostName) ||
		!writeStr("\r\n\r\n"_embed))
	{
		(void)Close();
		return Result<void, WebSocketError>::Err(WS_ERROR_WRITE_FAILED);
	}

	INT64 contentLength = -1;
	if (!HttpClient::ReadResponseHeaders(tlsContext, 101, contentLength))
	{
		(void)Close();
		return Result<void, WebSocketError>::Err(WS_ERROR_HANDSHAKE_FAILED);
	}

	isConnected = true;
	return Result<void, WebSocketError>::Ok();
}

Result<void, WebSocketError> WebSocketClient::Close()
{
	if (isConnected)
	{
		// Send a WebSocket CLOSE frame (status code 1000 = normal closure, big-endian)
		UINT16 statusCode = UINT16SwapByteOrder(1000);
		(void)Write(&statusCode, sizeof(statusCode), OPCODE_CLOSE);
	}

	isConnected = false;
	(void)tlsContext.Close();
	LOG_DEBUG("WebSocket client to %s:%u%s closed", hostName, port, path);
	return Result<void, WebSocketError>::Ok();
}

Result<UINT32, WebSocketError> WebSocketClient::Write(PCVOID buffer, UINT32 bufferLength, WebSocketOpcode opcode)
{
	if (!isConnected && opcode != OPCODE_CLOSE)
		return Result<UINT32, WebSocketError>::Err(WS_ERROR_NOT_CONNECTED);

	// Build frame header on stack (max 14 bytes: 2 base + 8 ext length + 4 mask key)
	UINT8 header[14];
	UINT32 headerLength;

	// FIN bit + opcode
	header[0] = (UINT8)(opcode | 0x80);

	// Generate masking key from a single random value
	Random random;
	UINT32 maskKeyVal = (UINT32)random.Get();
	PUINT8 maskKey = (PUINT8)&maskKeyVal;

	// Encode payload length + mask bit
	if (bufferLength <= 125)
	{
		header[1] = (UINT8)(bufferLength | 0x80);
		Memory::Copy(header + 2, maskKey, 4);
		headerLength = 6;
	}
	else if (bufferLength <= 0xFFFF)
	{
		header[1] = (126 | 0x80);
		UINT16 len16 = UINT16SwapByteOrder((UINT16)bufferLength);
		Memory::Copy(header + 2, &len16, 2);
		Memory::Copy(header + 4, maskKey, 4);
		headerLength = 8;
	}
	else
	{
		header[1] = (127 | 0x80);
		UINT64 len64 = UINT64SwapByteOrder((UINT64)bufferLength);
		Memory::Copy(header + 2, &len64, 8);
		Memory::Copy(header + 10, maskKey, 4);
		headerLength = 14;
	}

	// Chunk buffer for masking: small enough for the stack, multiple of 4 for mask alignment
	UINT8 chunk[256];

	// Small frames: combine header + masked payload into a single write
	if (bufferLength <= sizeof(chunk) - headerLength)
	{
		Memory::Copy(chunk, header, headerLength);
		PUINT8 dst = chunk + headerLength;
		PUINT8 src = (PUINT8)buffer;
		for (UINT32 i = 0; i < bufferLength; i++)
			dst[i] = src[i] ^ maskKey[i & 3];

		UINT32 frameLength = headerLength + bufferLength;
		auto smallWrite = tlsContext.Write(chunk, frameLength);
		if (!smallWrite || smallWrite.Value() != frameLength)
			return Result<UINT32, WebSocketError>::Err(WS_ERROR_WRITE_FAILED);

		return Result<UINT32, WebSocketError>::Ok(bufferLength);
	}

	// Large frames: write header, then mask and write payload in chunks
	auto headerWrite = tlsContext.Write(header, headerLength);
	if (!headerWrite || headerWrite.Value() != headerLength)
		return Result<UINT32, WebSocketError>::Err(WS_ERROR_WRITE_FAILED);

	PUINT8 src = (PUINT8)buffer;
	UINT32 offset = 0;
	UINT32 remaining = bufferLength;

	while (remaining > 0)
	{
		UINT32 chunkSize = (remaining < (UINT32)sizeof(chunk)) ? remaining : (UINT32)sizeof(chunk);
		for (UINT32 i = 0; i < chunkSize; i++)
			chunk[i] = src[offset + i] ^ maskKey[(offset + i) & 3];

		auto chunkWrite = tlsContext.Write(chunk, chunkSize);
		if (!chunkWrite || chunkWrite.Value() != chunkSize)
			return Result<UINT32, WebSocketError>::Err(WS_ERROR_WRITE_FAILED);

		offset += chunkSize;
		remaining -= chunkSize;
	}

	return Result<UINT32, WebSocketError>::Ok(bufferLength);
}

// Read exactly `size` bytes from the TLS transport
BOOL WebSocketClient::ReceiveRestrict(PVOID buffer, UINT32 size)
{
	UINT32 totalBytesRead = 0;
	while (totalBytesRead < size)
	{
		auto readResult = tlsContext.Read((PCHAR)buffer + totalBytesRead, size - totalBytesRead);
		if (!readResult || readResult.Value() <= 0)
			return false;
		totalBytesRead += (UINT32)readResult.Value();
	}
	return true;
}

VOID WebSocketClient::MaskFrame(WebSocketFrame &frame, UINT32 maskKey)
{
	PUINT8 mask = (PUINT8)&maskKey;
	PUINT8 d = (PUINT8)frame.data;
	UINT32 len = (UINT32)frame.length;

	// Process 4 bytes at a time (unrolled, no modulo in main loop)
	UINT32 i = 0;
	for (; i + 4 <= len; i += 4)
	{
		d[i] ^= mask[0];
		d[i + 1] ^= mask[1];
		d[i + 2] ^= mask[2];
		d[i + 3] ^= mask[3];
	}

	// Remaining 0-3 bytes
	for (; i < len; i++)
		d[i] ^= mask[i & 3];
}

BOOL WebSocketClient::ReceiveFrame(WebSocketFrame &frame)
{
	UINT8 header[2] = {0};
	if (!ReceiveRestrict(&header, 2))
		return false;

	UINT8 b1 = header[0];
	UINT8 b2 = header[1];

	frame.fin = (b1 >> 7) & 1;
	frame.rsv1 = (b1 >> 6) & 1;
	frame.rsv2 = (b1 >> 5) & 1;
	frame.rsv3 = (b1 >> 4) & 1;
	frame.opcode = (WebSocketOpcode)(b1 & 0x0F);
	frame.mask = (b2 >> 7) & 1;

	// RFC 6455 Section 5.2: RSV1-3 must be 0 unless extensions are negotiated
	if (frame.rsv1 || frame.rsv2 || frame.rsv3)
		return false;

	UINT8 lengthBits = b2 & 0x7F;

	if (lengthBits == 126)
	{
		UINT16 len16 = 0;
		if (!ReceiveRestrict(&len16, 2))
			return false;
		frame.length = UINT16SwapByteOrder(len16);
	}
	else if (lengthBits == 127)
	{
		UINT64 len64 = 0;
		if (!ReceiveRestrict(&len64, 8))
			return false;
		frame.length = UINT64SwapByteOrder(len64);
	}
	else
	{
		frame.length = lengthBits;
	}

	// Reject frames that would require an absurd allocation (>64 MB)
	if (frame.length > 0x4000000)
		return false;

	UINT32 frameMask = 0;
	if (frame.mask)
	{
		if (!ReceiveRestrict(&frameMask, 4))
			return false;
	}

	frame.data = nullptr;
	if (frame.length > 0)
	{
		frame.data = new CHAR[(UINT32)frame.length];
		if (!frame.data)
			return false;

		if (!ReceiveRestrict(frame.data, (UINT32)frame.length))
		{
			delete[] frame.data;
			frame.data = nullptr;
			return false;
		}
	}

	if (frame.mask && frame.data)
		MaskFrame(frame, frameMask);

	return true;
}

Result<WebSocketMessage, WebSocketError> WebSocketClient::Read()
{
	if (!isConnected)
		return Result<WebSocketMessage, WebSocketError>::Err(WS_ERROR_NOT_CONNECTED);

	WebSocketFrame frame;
	WebSocketMessage message;
	BOOL messageComplete = false;

	while (isConnected)
	{
		Memory::Zero(&frame, sizeof(frame));
		if (!ReceiveFrame(frame))
			break;

		if (frame.opcode == OPCODE_TEXT || frame.opcode == OPCODE_BINARY || frame.opcode == OPCODE_CONTINUE)
		{
			if (frame.opcode == OPCODE_CONTINUE && message.data == nullptr)
			{
				delete[] frame.data;
				break;
			}

			// Capture opcode from the initial (non-continuation) frame
			if (frame.opcode != OPCODE_CONTINUE)
				message.opcode = frame.opcode;

			if (frame.length > 0)
			{
				if (message.data)
				{
					PCHAR tempBuffer = new CHAR[message.length + (UINT32)frame.length];
					if (!tempBuffer)
					{
						delete[] frame.data;
						delete[] message.data;
						message.data = nullptr;
						break;
					}
					Memory::Copy(tempBuffer, message.data, message.length);
					Memory::Copy(tempBuffer + message.length, frame.data, (UINT32)frame.length);
					delete[] message.data;
					message.data = tempBuffer;
					message.length += (UINT32)frame.length;
					delete[] frame.data;
				}
				else
				{
					message.data = frame.data;
					message.length = (UINT32)frame.length;
				}
			}

			if (frame.fin)
			{
				messageComplete = true;
				break;
			}
		}
		else if (frame.opcode == OPCODE_CLOSE)
		{
			// Send close response per RFC 6455 Section 5.5.1
			(void)Write(frame.data, (frame.length >= 2) ? 2 : 0, OPCODE_CLOSE);
			delete[] frame.data;
			isConnected = false;
			return Result<WebSocketMessage, WebSocketError>::Err(WS_ERROR_CONNECTION_CLOSED);
		}
		else if (frame.opcode == OPCODE_PING)
		{
			(void)Write(frame.data, (UINT32)frame.length, OPCODE_PONG);
			delete[] frame.data;
		}
		else if (frame.opcode == OPCODE_PONG)
		{
			delete[] frame.data;
		}
		else
		{
			delete[] frame.data;
			break;
		}
	}

	if (!messageComplete)
		return Result<WebSocketMessage, WebSocketError>::Err(WS_ERROR_RECEIVE_FAILED);

	return Result<WebSocketMessage, WebSocketError>::Ok(static_cast<WebSocketMessage &&>(message));
}

WebSocketClient::WebSocketClient(PCCHAR url)
{
	Memory::Zero(hostName, sizeof(hostName));
	Memory::Zero(path, sizeof(path));
	port = 0;
	isConnected = false;

	BOOL isSecure = false;
	if (!HttpClient::ParseUrl(url, hostName, path, port, isSecure))
		return;

	auto dnsResult = DNS::Resolve(hostName);
	if (!dnsResult)
	{
		LOG_ERROR("Failed to resolve hostname %s", hostName);
		return;
	}
	ipAddress = dnsResult.Value();

	tlsContext = TLSClient(hostName, ipAddress, port, isSecure);
}
