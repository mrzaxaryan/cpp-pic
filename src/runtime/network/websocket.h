#pragma once
#include "platform.h"
#include "tls.h"

/**
 * @brief WebSocket frame opcodes
 * @details Defines the opcode values carried in the first byte of every WebSocket frame.
 * @see RFC 6455 Section 5.2 — Base Framing Protocol (opcode field definition)
 *      https://datatracker.ietf.org/doc/html/rfc6455#section-5.2
 * @see RFC 6455 Section 11.8 — WebSocket Opcode Registry
 *      https://datatracker.ietf.org/doc/html/rfc6455#section-11.8
 */
enum WebSocketOpcode : INT8
{
	OPCODE_CONTINUE = 0x0, ///< Continuation frame (RFC 6455 Section 5.4)
	OPCODE_TEXT     = 0x1, ///< Text data frame — payload is UTF-8 (RFC 6455 Section 5.6)
	OPCODE_BINARY   = 0x2, ///< Binary data frame (RFC 6455 Section 5.6)
	OPCODE_CLOSE    = 0x8, ///< Connection close control frame (RFC 6455 Section 5.5.1)
	OPCODE_PING     = 0x9, ///< Ping control frame (RFC 6455 Section 5.5.2)
	OPCODE_PONG     = 0xA  ///< Pong control frame (RFC 6455 Section 5.5.3)
};

/**
 * @brief Represents a single parsed WebSocket frame
 * @details Maps directly to the wire format defined by the base framing protocol.
 * Each frame carries a FIN bit, RSV flags, an opcode, an optional masking key,
 * a variable-length payload length, and the payload data itself.
 * @see RFC 6455 Section 5.2 — Base Framing Protocol
 *      https://datatracker.ietf.org/doc/html/rfc6455#section-5.2
 */
struct WebSocketFrame
{
	PCHAR data;            ///< Pointer to the payload data (heap-allocated by ReceiveFrame)
	UINT64 length;         ///< Payload length in bytes (decoded from 7-bit, 16-bit, or 64-bit encoding)
	WebSocketOpcode opcode;///< Frame opcode (RFC 6455 Section 5.2, bits [4:7] of byte 0)
	UINT8 fin;             ///< FIN bit — 1 if this is the final fragment of a message (bit 0 of byte 0)
	UINT8 mask;            ///< MASK bit — 1 if payload is masked with a 32-bit key (bit 0 of byte 1)
	UINT8 rsv1;            ///< RSV1 extension bit — must be 0 unless an extension is negotiated
	UINT8 rsv2;            ///< RSV2 extension bit — must be 0 unless an extension is negotiated
	UINT8 rsv3;            ///< RSV3 extension bit — must be 0 unless an extension is negotiated
};

/**
 * @brief Represents a fully reassembled WebSocket message
 * @details A message may span multiple frames when fragmentation is used. This struct
 * holds the concatenated payload from all fragments, along with the opcode from
 * the initial frame. Owns its data buffer and frees it on destruction.
 * @see RFC 6455 Section 5.4 — Fragmentation
 *      https://datatracker.ietf.org/doc/html/rfc6455#section-5.4
 */
struct WebSocketMessage
{
	PCHAR data;            ///< Pointer to the reassembled message payload (heap-allocated)
	USIZE length;          ///< Total message payload length in bytes
	WebSocketOpcode opcode;///< Message type captured from the first (non-continuation) frame

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

/**
 * @brief WebSocket client implementing the WebSocket Protocol (RFC 6455)
 * @details Provides a full WebSocket client over TLS (wss://) or plaintext (ws://) transport.
 * Implements the opening handshake (Section 4), base framing protocol (Section 5.2),
 * client-to-server masking (Section 5.3), message fragmentation/reassembly (Section 5.4),
 * control frame handling — Close, Ping, Pong (Section 5.5), and closing handshake (Section 7).
 *
 * Stack-only type — heap allocation is deleted; placement new is provided for Result.
 *
 * @see RFC 6455 — The WebSocket Protocol
 *      https://datatracker.ietf.org/doc/html/rfc6455
 */
class WebSocketClient
{
private:
	CHAR hostName[254];  ///< Server hostname (RFC 1035: max 253 chars + null terminator)
	CHAR path[2048];     ///< Request-URI path component
	IPAddress ipAddress; ///< Resolved server IP address
	UINT16 port;         ///< Server port number
	TlsClient tlsContext;///< Underlying TLS/plaintext transport
	BOOL isConnected;    ///< Whether the WebSocket connection is in the OPEN state

	/**
	 * @brief Reads exactly buffer.Size() bytes from the TLS transport
	 * @param buffer Destination buffer — all bytes must be filled before returning
	 * @return Ok on success, Err(Ws_ReceiveFailed) if the transport returns an error or EOF
	 * @details Loops over TlsClient::Read until the requested byte count is satisfied.
	 * Used internally by ReceiveFrame to read fixed-size header fields and payload data.
	 */
	[[nodiscard]] Result<void, Error> ReceiveRestrict(Span<CHAR> buffer);

	/**
	 * @brief Reads and parses a single WebSocket frame from the transport
	 * @param frame Output — populated with header fields and heap-allocated payload data
	 * @return Ok on success, Err on transport error, invalid frame, or allocation failure
	 *
	 * @details Implements the frame parsing logic defined in RFC 6455 Section 5.2:
	 *   1. Reads the 2-byte frame header (FIN, RSV1-3, opcode, MASK, payload length)
	 *   2. Reads the extended payload length (16-bit or 64-bit) if the 7-bit length is 126 or 127
	 *   3. Reads the 4-byte masking key if the MASK bit is set
	 *   4. Allocates and reads the payload data
	 *   5. Applies the masking key to unmask the payload if MASK was set
	 *
	 * Rejects frames with non-zero RSV bits (no extensions negotiated) per Section 5.2.
	 * Rejects frames with payload length > 64 MB to prevent excessive allocation.
	 *
	 * @warning The caller is responsible for freeing frame.data with delete[].
	 *
	 * @see RFC 6455 Section 5.2 — Base Framing Protocol
	 *      https://datatracker.ietf.org/doc/html/rfc6455#section-5.2
	 */
	[[nodiscard]] Result<void, Error> ReceiveFrame(WebSocketFrame &frame);

	/**
	 * @brief Applies or removes the XOR masking transformation on a frame's payload
	 * @param frame The frame whose data buffer will be masked/unmasked in-place
	 * @param maskKey The 32-bit masking key (4 bytes applied cyclically)
	 *
	 * @details Implements the masking algorithm from RFC 6455 Section 5.3:
	 *   j = i MOD 4
	 *   transformed-octet-i = original-octet-i XOR masking-key-octet-j
	 *
	 * The same operation both masks and unmasks since XOR is its own inverse.
	 * Processes 4 bytes at a time in the main loop for efficiency.
	 *
	 * @see RFC 6455 Section 5.3 — Client-to-Server Masking
	 *      https://datatracker.ietf.org/doc/html/rfc6455#section-5.3
	 */
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

	/**
	 * @brief Factory method — creates a WebSocketClient from a ws:// or wss:// URL
	 * @param url Null-terminated WebSocket URL (e.g., "wss://example.com/path")
	 * @return Ok(WebSocketClient) ready for Open(), or Err(Ws_CreateFailed) on parse/DNS/TLS failure
	 *
	 * @details Performs URL parsing via HttpClient::ParseUrl, DNS resolution via DNS::Resolve,
	 * and TLS transport creation via TlsClient::Create. Falls back to IPv4 if IPv6 socket
	 * creation fails (e.g., on UEFI platforms without IPv6 support).
	 *
	 * The returned client is in the CLOSED state — call Open() to perform the handshake.
	 */
	[[nodiscard]] static Result<WebSocketClient, Error> Create(PCCHAR url);

	/** @brief Returns true if the underlying TLS transport is valid */
	BOOL IsValid() const { return tlsContext.IsValid(); }
	/** @brief Returns true if the connection uses TLS (wss://) */
	BOOL IsSecure() const { return tlsContext.IsSecure(); }
	/** @brief Returns true if the WebSocket connection is in the OPEN state */
	BOOL IsConnected() const { return isConnected; }

	/**
	 * @brief Performs the WebSocket opening handshake
	 * @return Ok on successful handshake, Err on transport/write/handshake failure
	 *
	 * @details Implements the client-side opening handshake defined in RFC 6455 Section 4:
	 *   1. Opens the underlying TLS/TCP transport (with IPv4 fallback on IPv6 failure)
	 *   2. Generates a random 16-byte Sec-WebSocket-Key and Base64-encodes it (Section 4.1)
	 *   3. Sends the HTTP Upgrade request with required headers:
	 *      - GET <path> HTTP/1.1
	 *      - Host, Upgrade: websocket, Connection: Upgrade
	 *      - Sec-WebSocket-Key, Sec-WebSocket-Version: 13, Origin
	 *   4. Validates the server returns HTTP 101 Switching Protocols (Section 4.2.2)
	 *
	 * @see RFC 6455 Section 4 — Opening Handshake
	 *      https://datatracker.ietf.org/doc/html/rfc6455#section-4
	 * @see RFC 6455 Section 4.1 — Client Requirements
	 *      https://datatracker.ietf.org/doc/html/rfc6455#section-4.1
	 */
	[[nodiscard]] Result<void, Error> Open();

	/**
	 * @brief Performs the WebSocket closing handshake and shuts down the transport
	 * @return Ok always (close is best-effort)
	 *
	 * @details Implements the client-initiated close per RFC 6455 Section 7:
	 *   1. Sends a Close control frame with status code 1000 (Normal Closure) in big-endian
	 *   2. Marks the connection as closed
	 *   3. Closes the underlying TLS transport
	 *
	 * @see RFC 6455 Section 7 — Closing the Connection
	 *      https://datatracker.ietf.org/doc/html/rfc6455#section-7
	 * @see RFC 6455 Section 7.4.1 — Defined Status Codes (1000 = Normal Closure)
	 *      https://datatracker.ietf.org/doc/html/rfc6455#section-7.4.1
	 */
	[[nodiscard]] Result<void, Error> Close();

	/**
	 * @brief Reads the next complete WebSocket message (reassembling fragments)
	 * @return Ok(WebSocketMessage) containing the full payload and opcode, or Err on failure
	 *
	 * @details Implements message reception with fragmentation support per RFC 6455 Section 5.4:
	 *   - Reads frames in a loop until a frame with FIN=1 is received
	 *   - Concatenates payloads from continuation frames (opcode 0x0)
	 *   - Captures the message opcode from the first non-continuation frame
	 *   - Handles interleaved control frames transparently:
	 *     - CLOSE (0x8): echoes the status code back and returns Err(Ws_ConnectionClosed)
	 *       per RFC 6455 Section 5.5.1
	 *     - PING (0x9): responds with PONG carrying the same payload per RFC 6455 Section 5.5.2
	 *     - PONG (0xA): silently discarded per RFC 6455 Section 5.5.3
	 *
	 * @see RFC 6455 Section 5.4 — Fragmentation
	 *      https://datatracker.ietf.org/doc/html/rfc6455#section-5.4
	 * @see RFC 6455 Section 5.5 — Control Frames
	 *      https://datatracker.ietf.org/doc/html/rfc6455#section-5.5
	 */
	[[nodiscard]] Result<WebSocketMessage, Error> Read();

	/**
	 * @brief Sends a WebSocket data or control frame with client-to-server masking
	 * @param buffer Payload data to send
	 * @param opcode Frame opcode (default: OPCODE_BINARY)
	 * @return Ok(bytesSent) on success, Err(Ws_WriteFailed | Ws_NotConnected) on failure
	 *
	 * @details Implements frame construction and masking per RFC 6455 Section 5.2 and 5.3:
	 *   1. Sets FIN=1 (no fragmentation on send — each Write produces a single complete frame)
	 *   2. Encodes the payload length using the variable-length encoding:
	 *      - 0–125: 7-bit length in byte 1
	 *      - 126–65535: 16-bit extended length (network byte order)
	 *      - 65536+: 64-bit extended length (network byte order)
	 *   3. Generates a random 32-bit masking key (all client frames MUST be masked per Section 5.3)
	 *   4. XOR-masks the payload with the masking key
	 *   5. For small payloads (<=242 bytes): combines header + masked payload into one TLS write
	 *      For large payloads: writes header first, then streams masked chunks of 256 bytes
	 *
	 * @see RFC 6455 Section 5.2 — Base Framing Protocol
	 *      https://datatracker.ietf.org/doc/html/rfc6455#section-5.2
	 * @see RFC 6455 Section 5.3 — Client-to-Server Masking
	 *      https://datatracker.ietf.org/doc/html/rfc6455#section-5.3
	 */
	[[nodiscard]] Result<UINT32, Error> Write(Span<const CHAR> buffer, WebSocketOpcode opcode = OPCODE_BINARY);
};
