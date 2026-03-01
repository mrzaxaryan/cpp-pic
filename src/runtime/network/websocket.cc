#include "websocket.h"
#include "memory.h"
#include "string.h"
#include "random.h"
#include "logger.h"
#include "dns.h"
#include "http.h"
#include "embedded_string.h"

/**
 * @brief Performs the WebSocket opening handshake (RFC 6455 Section 4)
 * @details Sends the HTTP Upgrade request with Sec-WebSocket-Key (16 random bytes,
 * Base64-encoded per Section 4.1) and validates the server responds with HTTP 101.
 * Falls back to IPv4 if the initial IPv6 connection attempt fails.
 * @see https://datatracker.ietf.org/doc/html/rfc6455#section-4
 */
Result<void, Error> WebSocketClient::Open()
{
	BOOL isSecure = tlsContext.IsSecure();
	LOG_DEBUG("Opening WebSocket client to %s:%u%s (secure: %s)", hostName, port, path, isSecure ? "true"_embed : "false"_embed);

	auto openResult = tlsContext.Open();

	if (!openResult && ipAddress.IsIPv6())
	{
		LOG_DEBUG("Failed to open network transport for WebSocket client using IPv6 address, attempting IPv4 fallback");

		auto dnsResult = DNS::Resolve(hostName, A);
		if (!dnsResult)
		{
			LOG_ERROR("Failed to resolve IPv4 address for %s, cannot connect to WebSocket server", hostName);
			return Result<void, Error>::Err(Error::Ws_DnsFailed);
		}

		ipAddress = dnsResult.Value();

		(void)tlsContext.Close();
		auto tlsResult = TlsClient::Create(hostName, ipAddress, port, isSecure);
		if (!tlsResult)
		{
			LOG_ERROR("Failed to create TLS client for IPv4 fallback (error: %e)", tlsResult.Error());
			return Result<void, Error>::Err(Error::Ws_TransportFailed);
		}
		tlsContext = static_cast<TlsClient &&>(tlsResult.Value());
		openResult = tlsContext.Open();
	}

	if (!openResult)
	{
		LOG_DEBUG("Failed to open network transport for WebSocket client");
		return Result<void, Error>::Err(openResult, Error::Ws_TransportFailed);
	}

	// RFC 6455 Section 4.1: Sec-WebSocket-Key is 16 random bytes, Base64-encoded (24 chars)
	UINT32 key[4];
	Random random;
	for (INT32 i = 0; i < 4; i++)
		key[i] = (UINT32)random.Get();

	CHAR secureKey[25]; // Base64 of 16 bytes = 24 chars + null
	Base64::Encode(Span<const CHAR>((PCCHAR)key, 16), Span<CHAR>(secureKey));

	auto writeStr = [&](PCCHAR s) -> BOOL
	{
		UINT32 len = String::Length(s);
		auto r = tlsContext.Write(Span<const CHAR>(s, len));
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
		return Result<void, Error>::Err(Error::Ws_WriteFailed);
	}

	auto headerResult = HttpClient::ReadResponseHeaders(tlsContext, 101);
	if (!headerResult)
	{
		(void)Close();
		return Result<void, Error>::Err(headerResult, Error::Ws_HandshakeFailed);
	}

	isConnected = true;
	return Result<void, Error>::Ok();
}

/**
 * @brief Sends a Close frame with status 1000 (Normal Closure) and tears down the transport
 * @details Implements RFC 6455 Section 7.1.1 — the client initiates the closing handshake
 * by sending a Close frame whose payload is the 2-byte status code in network byte order.
 * @see https://datatracker.ietf.org/doc/html/rfc6455#section-7
 * @see https://datatracker.ietf.org/doc/html/rfc6455#section-7.4.1
 */
Result<void, Error> WebSocketClient::Close()
{
	if (isConnected)
	{
		// RFC 6455 Section 5.5.1: Send Close frame with status code 1000 (Normal Closure, big-endian)
		UINT16 statusCode = UINT16SwapByteOrder(1000);
		(void)Write(Span<const CHAR>((const CHAR *)&statusCode, sizeof(statusCode)), OPCODE_CLOSE);
	}

	isConnected = false;
	(void)tlsContext.Close();
	LOG_DEBUG("WebSocket client to %s:%u%s closed", hostName, port, path);
	return Result<void, Error>::Ok();
}

/**
 * @brief Constructs and sends a masked WebSocket frame (RFC 6455 Section 5.2, 5.3)
 * @details Builds the frame header with FIN=1 and the appropriate payload length encoding
 * (7-bit / 16-bit / 64-bit). Generates a random 32-bit masking key and XOR-masks the
 * entire payload — client-to-server frames MUST be masked per Section 5.1. Small frames
 * are coalesced into a single TLS write; large frames stream in 256-byte masked chunks.
 * @see https://datatracker.ietf.org/doc/html/rfc6455#section-5.2
 * @see https://datatracker.ietf.org/doc/html/rfc6455#section-5.3
 */
Result<UINT32, Error> WebSocketClient::Write(Span<const CHAR> buffer, WebSocketOpcode opcode)
{
	UINT32 bufferLength = (UINT32)buffer.Size();
	if (!isConnected && opcode != OPCODE_CLOSE)
	{
		return Result<UINT32, Error>::Err(Error::Ws_NotConnected);
	}

	// Build frame header on stack (max 14 bytes: 2 base + 8 ext length + 4 mask key)
	UINT8 header[14];
	UINT32 headerLength;

	// RFC 6455 Section 5.2: byte 0 = FIN (bit 7) | opcode (bits 0-3)
	header[0] = (UINT8)(opcode | 0x80);

	// Generate masking key from a single random value
	Random random;
	UINT32 maskKeyVal = (UINT32)random.Get();
	PUINT8 maskKey = (PUINT8)&maskKeyVal;

	// RFC 6455 Section 5.2: byte 1 = MASK (bit 7) | payload length (bits 0-6)
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
		PUINT8 src = (PUINT8)buffer.Data();
		for (UINT32 i = 0; i < bufferLength; i++)
			dst[i] = src[i] ^ maskKey[i & 3];

		UINT32 frameLength = headerLength + bufferLength;
		auto smallWrite = tlsContext.Write(Span<const CHAR>((PCHAR)chunk, frameLength));
		if (!smallWrite || smallWrite.Value() != frameLength)
		{
			return Result<UINT32, Error>::Err(Error::Ws_WriteFailed);
		}

		return Result<UINT32, Error>::Ok(bufferLength);
	}

	// Large frames: write header, then mask and write payload in chunks
	auto headerWrite = tlsContext.Write(Span<const CHAR>((PCHAR)header, headerLength));
	if (!headerWrite || headerWrite.Value() != headerLength)
	{
		return Result<UINT32, Error>::Err(Error::Ws_WriteFailed);
	}

	PUINT8 src = (PUINT8)buffer.Data();
	UINT32 offset = 0;
	UINT32 remaining = bufferLength;

	while (remaining > 0)
	{
		UINT32 chunkSize = (remaining < (UINT32)sizeof(chunk)) ? remaining : (UINT32)sizeof(chunk);
		for (UINT32 i = 0; i < chunkSize; i++)
			chunk[i] = src[offset + i] ^ maskKey[(offset + i) & 3];

		auto chunkWrite = tlsContext.Write(Span<const CHAR>((PCHAR)chunk, chunkSize));
		if (!chunkWrite || chunkWrite.Value() != chunkSize)
		{
			return Result<UINT32, Error>::Err(Error::Ws_WriteFailed);
		}

		offset += chunkSize;
		remaining -= chunkSize;
	}

	return Result<UINT32, Error>::Ok(bufferLength);
}

/**
 * @brief Reads exactly buffer.Size() bytes from the TLS transport
 * @details Loops over TlsClient::Read until all requested bytes are received.
 * Returns Err immediately if any individual read returns an error or zero bytes.
 * Used by ReceiveFrame to read fixed-size frame header fields and payload data.
 */
Result<void, Error> WebSocketClient::ReceiveRestrict(Span<CHAR> buffer)
{
	UINT32 size = (UINT32)buffer.Size();
	UINT32 totalBytesRead = 0;
	while (totalBytesRead < size)
	{
		auto readResult = tlsContext.Read(Span<CHAR>(buffer.Data() + totalBytesRead, size - totalBytesRead));
		if (!readResult || readResult.Value() <= 0)
			return Result<void, Error>::Err(readResult, Error::Ws_ReceiveFailed);
		totalBytesRead += (UINT32)readResult.Value();
	}
	return Result<void, Error>::Ok();
}

/**
 * @brief Applies the RFC 6455 Section 5.3 XOR masking transformation in-place
 * @details Iterates over frame.data applying: data[i] ^= maskKey[i % 4].
 * Processes 4 bytes per iteration in the main loop, then handles 0–3 trailing bytes.
 * The same function both masks and unmasks since XOR is self-inverse.
 * @see https://datatracker.ietf.org/doc/html/rfc6455#section-5.3
 */
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

/**
 * @brief Reads and parses a single WebSocket frame from the transport (RFC 6455 Section 5.2)
 * @details Parses the wire format:
 *   Byte 0: [FIN:1][RSV1:1][RSV2:1][RSV3:1][opcode:4]
 *   Byte 1: [MASK:1][payload_len:7]
 *   If payload_len == 126: next 2 bytes are the 16-bit length (network byte order)
 *   If payload_len == 127: next 8 bytes are the 64-bit length (network byte order)
 *   If MASK == 1: next 4 bytes are the masking key
 *   Remaining bytes: payload data (unmasked after reading if MASK was set)
 *
 * Rejects frames with non-zero RSV bits (Section 5.2) and payloads > 64 MB.
 * @see https://datatracker.ietf.org/doc/html/rfc6455#section-5.2
 */
Result<void, Error> WebSocketClient::ReceiveFrame(WebSocketFrame &frame)
{
	UINT8 header[2] = {0};
	auto headerResult = ReceiveRestrict(Span<CHAR>((PCHAR)header, sizeof(header)));
	if (!headerResult)
		return Result<void, Error>::Err(headerResult, Error::Ws_ReceiveFailed);

	UINT8 b1 = header[0];
	UINT8 b2 = header[1];

	frame.fin = (b1 >> 7) & 1;
	frame.rsv1 = (b1 >> 6) & 1;
	frame.rsv2 = (b1 >> 5) & 1;
	frame.rsv3 = (b1 >> 4) & 1;
	frame.opcode = (WebSocketOpcode)(b1 & 0x0F);
	frame.mask = (b2 >> 7) & 1;

	// RFC 6455 Section 5.2: RSV1-3 MUST be 0 unless an extension defining their meaning is negotiated
	if (frame.rsv1 || frame.rsv2 || frame.rsv3)
		return Result<void, Error>::Err(Error::Ws_InvalidFrame);

	UINT8 lengthBits = b2 & 0x7F;

	if (lengthBits == 126)
	{
		UINT16 len16 = 0;
		auto lenResult = ReceiveRestrict(Span<CHAR>((PCHAR)&len16, sizeof(len16)));
		if (!lenResult)
			return Result<void, Error>::Err(lenResult, Error::Ws_ReceiveFailed);
		frame.length = UINT16SwapByteOrder(len16);
	}
	else if (lengthBits == 127)
	{
		UINT64 len64 = 0;
		auto lenResult = ReceiveRestrict(Span<CHAR>((PCHAR)&len64, sizeof(len64)));
		if (!lenResult)
			return Result<void, Error>::Err(lenResult, Error::Ws_ReceiveFailed);
		frame.length = UINT64SwapByteOrder(len64);
	}
	else
	{
		frame.length = lengthBits;
	}

	// Reject frames that would require an absurd allocation (>64 MB)
	if (frame.length > 0x4000000)
		return Result<void, Error>::Err(Error::Ws_FrameTooLarge);

	UINT32 frameMask = 0;
	if (frame.mask)
	{
		auto maskResult = ReceiveRestrict(Span<CHAR>((PCHAR)&frameMask, sizeof(frameMask)));
		if (!maskResult)
			return Result<void, Error>::Err(maskResult, Error::Ws_ReceiveFailed);
	}

	frame.data = nullptr;
	if (frame.length > 0)
	{
		frame.data = new CHAR[(USIZE)frame.length];
		if (!frame.data)
			return Result<void, Error>::Err(Error::Ws_AllocFailed);

		auto dataResult = ReceiveRestrict(Span<CHAR>(frame.data, (USIZE)frame.length));
		if (!dataResult)
		{
			delete[] frame.data;
			frame.data = nullptr;
			return Result<void, Error>::Err(dataResult, Error::Ws_ReceiveFailed);
		}
	}

	if (frame.mask && frame.data)
		MaskFrame(frame, frameMask);

	return Result<void, Error>::Ok();
}

/**
 * @brief Reads the next complete WebSocket message, reassembling fragmented frames
 * @details Implements message reception per RFC 6455 Section 5.4 (Fragmentation):
 *   - An unfragmented message is a single frame with FIN=1 and opcode != 0
 *   - A fragmented message starts with opcode != 0 and FIN=0, followed by zero or more
 *     continuation frames (opcode=0, FIN=0), ending with a continuation frame with FIN=1
 *   - Payloads from all fragments are concatenated into a single WebSocketMessage
 *
 * Control frames (Close, Ping, Pong) may be interleaved between data fragments:
 *   - Close (Section 5.5.1): echoes the status code and returns Err(Ws_ConnectionClosed)
 *   - Ping (Section 5.5.2): responds with Pong carrying the same Application Data
 *   - Pong (Section 5.5.3): silently discarded (unsolicited pongs are allowed)
 *
 * @see https://datatracker.ietf.org/doc/html/rfc6455#section-5.4
 * @see https://datatracker.ietf.org/doc/html/rfc6455#section-5.5
 */
Result<WebSocketMessage, Error> WebSocketClient::Read()
{
	if (!isConnected)
	{
		return Result<WebSocketMessage, Error>::Err(Error::Ws_NotConnected);
	}

	WebSocketFrame frame;
	WebSocketMessage message;
	BOOL messageComplete = false;

	while (isConnected)
	{
		Memory::Zero(&frame, sizeof(frame));
		auto frameResult = ReceiveFrame(frame);
		if (!frameResult)
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
					UINT32 newLength = message.length + (UINT32)frame.length;
					if (newLength < message.length)
					{
						delete[] frame.data;
						delete[] message.data;
						message.data = nullptr;
						break;
					}
					PCHAR tempBuffer = new CHAR[newLength];
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
					message.length = newLength;
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
			// RFC 6455 Section 5.5.1: echo the 2-byte status code back in the Close response
			(void)Write(Span<const CHAR>(frame.data, (frame.length >= 2) ? 2 : 0), OPCODE_CLOSE);
			delete[] frame.data;
			isConnected = false;
			return Result<WebSocketMessage, Error>::Err(Error::Ws_ConnectionClosed);
		}
		else if (frame.opcode == OPCODE_PING)
		{
			(void)Write(Span<const CHAR>(frame.data, (UINT32)frame.length), OPCODE_PONG);
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
	{
		return Result<WebSocketMessage, Error>::Err(Error::Ws_ReceiveFailed);
	}

	return Result<WebSocketMessage, Error>::Ok(static_cast<WebSocketMessage &&>(message));
}

/**
 * @brief Factory method — creates a WebSocketClient from a ws:// or wss:// URL
 * @param url Null-terminated WebSocket URL (ws:// or wss://)
 * @return Ok(WebSocketClient) ready for Open(), or Err(Ws_CreateFailed) on failure
 *
 * @details Performs three setup steps:
 *   1. Parses the URL into host, path, port, and secure flag via HttpClient::ParseUrl
 *   2. Resolves the hostname to an IP address via DNS::Resolve (AAAA first, A fallback)
 *   3. Creates the TLS transport via TlsClient::Create (with IPv4 fallback on IPv6 failure)
 *
 * The returned client is in the CLOSED state — call Open() to initiate the
 * opening handshake defined in RFC 6455 Section 4.
 * @see https://datatracker.ietf.org/doc/html/rfc6455#section-3
 */
Result<WebSocketClient, Error> WebSocketClient::Create(PCCHAR url)
{
	CHAR host[254];
	CHAR parsedPath[2048];
	UINT16 port;
	BOOL isSecure = false;
	auto parseResult = HttpClient::ParseUrl(url, host, parsedPath, port, isSecure);
	if (!parseResult)
		return Result<WebSocketClient, Error>::Err(Error::Ws_CreateFailed);

	auto dnsResult = DNS::Resolve(host);
	if (!dnsResult)
	{
		LOG_ERROR("Failed to resolve hostname %s", host);
		return Result<WebSocketClient, Error>::Err(Error::Ws_CreateFailed);
	}
	IPAddress ip = dnsResult.Value();

	auto tlsResult = TlsClient::Create(host, ip, port, isSecure);

	// IPv6 socket creation can fail on platforms without IPv6 support (e.g. UEFI)
	if (!tlsResult && ip.IsIPv6())
	{
		auto dnsResultV4 = DNS::Resolve(host, A);
		if (dnsResultV4)
		{
			ip = dnsResultV4.Value();
			tlsResult = TlsClient::Create(host, ip, port, isSecure);
		}
	}

	if (!tlsResult)
		return Result<WebSocketClient, Error>::Err(Error::Ws_CreateFailed);

	WebSocketClient client(host, parsedPath, ip, port, static_cast<TlsClient &&>(tlsResult.Value()));
	return Result<WebSocketClient, Error>::Ok(static_cast<WebSocketClient &&>(client));
}
