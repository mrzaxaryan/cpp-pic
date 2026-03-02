#pragma once

/**
 * @file tls.h
 * @brief TLS 1.3 client implementation for secure transport
 *
 * @details Implements a TLS 1.3 client (RFC 8446) supporting the full handshake protocol
 * including ClientHello, ServerHello, key exchange (ECDHE with secp256r1/secp384r1),
 * and record-layer encryption (ChaCha20-Poly1305). The client can also operate in
 * plaintext TCP mode when TLS is not required.
 *
 * The handshake follows the TLS 1.3 state machine:
 *   1. Send ClientHello with supported groups and cipher suites
 *   2. Receive ServerHello and derive handshake keys
 *   3. Receive encrypted extensions, certificate, and Finished
 *   4. Send client Finished and derive application traffic keys
 *
 * @see RFC 8446 — The Transport Layer Security (TLS) Protocol Version 1.3
 *      https://datatracker.ietf.org/doc/html/rfc8446
 */

#include "platform/platform.h"
#include "runtime/network/tls/tls_buffer.h"
#include "runtime/network/tls/tls_cipher.h"

/// TLS connection state tracking content type and handshake type
/// @see RFC 8446 Section 5.1 — Record Layer
struct TlsState
{
	INT32 ContentType;   ///< TLS content type (RFC 8446 Section 5.1)
	INT32 HandshakeType; ///< TLS handshake type (RFC 8446 Section 4)
};

class TlsClient final
{
private:
	PCCHAR host;             ///< Server hostname (borrowed pointer, must outlive client)
	IPAddress ip;            ///< Resolved server IP address
	Socket context;          ///< Underlying TCP socket
	TlsCipher crypto;        ///< TLS cipher suite and key material
	BOOL secure;             ///< Whether to use TLS handshake or plain TCP
	INT32 stateIndex;        ///< Current handshake state index
	TlsBuffer recvBuffer;    ///< Incoming data buffer
	INT32 decryptedPos;      ///< Consumption cursor into current decrypted record
	INT32 decryptedSize;     ///< Size of current decrypted record (0 = none available)
	[[nodiscard]] Result<void, Error> ReadNextRecord();
	[[nodiscard]] Result<void, Error> ProcessReceive();
	[[nodiscard]] Result<void, Error> OnPacket(INT32 packetType, INT32 version, TlsBuffer &tlsReader);
	[[nodiscard]] NOINLINE Result<void, Error> HandleHandshakeMessage(INT32 handshakeType, TlsBuffer &reader);
	[[nodiscard]] NOINLINE Result<void, Error> HandleAlertMessage(TlsBuffer &reader);
	[[nodiscard]] NOINLINE Result<void, Error> HandleApplicationData(TlsBuffer &reader);
	[[nodiscard]] Result<void, Error> OnServerFinished();
	[[nodiscard]] Result<void, Error> VerifyFinished(TlsBuffer &tlsReader);
	[[nodiscard]] Result<void, Error> OnServerHelloDone();
	[[nodiscard]] Result<void, Error> OnServerHello(TlsBuffer &tlsReader);
	[[nodiscard]] Result<void, Error> SendChangeCipherSpec();
	[[nodiscard]] Result<void, Error> SendClientExchange();
	[[nodiscard]] Result<void, Error> SendClientFinished();
	[[nodiscard]] Result<void, Error> SendClientHello(PCCHAR host);
	[[nodiscard]] Result<void, Error> SendPacket(INT32 packetType, INT32 ver, Span<const CHAR> data);

	// Private trivial constructor — only used by Create()
	TlsClient(PCCHAR host, const IPAddress &ipAddress, Socket &&socket, BOOL secure)
		: host(host), ip(ipAddress), context(static_cast<Socket &&>(socket)), secure(secure), stateIndex(0), decryptedPos(0), decryptedSize(0) {}

public:
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	// Placement new required by Result<TlsClient, Error>
	VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }
	TlsClient() : host(nullptr), ip(), secure(true), stateIndex(0), decryptedPos(0), decryptedSize(0) {}

	// Factory — caller MUST check the result (enforced by [[nodiscard]])
	[[nodiscard]] static Result<TlsClient, Error> Create(PCCHAR host, const IPAddress &ipAddress, UINT16 port, BOOL secure = true);

	~TlsClient()
	{
		if (IsValid())
			(void)Close();
	}

	TlsClient(const TlsClient &) = delete;
	TlsClient &operator=(const TlsClient &) = delete;

	TlsClient(TlsClient &&other) noexcept
		: host(other.host), ip(other.ip), context(static_cast<Socket &&>(other.context)), crypto(static_cast<TlsCipher &&>(other.crypto)), secure(other.secure), stateIndex(other.stateIndex), recvBuffer(static_cast<TlsBuffer &&>(other.recvBuffer)), decryptedPos(other.decryptedPos), decryptedSize(other.decryptedSize)
	{
		other.host = nullptr;
		other.secure = true;
		other.stateIndex = 0;
		other.decryptedPos = 0;
		other.decryptedSize = 0;
	}

	TlsClient &operator=(TlsClient &&other) noexcept
	{
		if (this != &other)
		{
			(void)Close();
			host = other.host;
			ip = other.ip;
			context = static_cast<Socket &&>(other.context);
			crypto = static_cast<TlsCipher &&>(other.crypto);
			secure = other.secure;
			stateIndex = other.stateIndex;
			recvBuffer = static_cast<TlsBuffer &&>(other.recvBuffer);
			decryptedPos = other.decryptedPos;
			decryptedSize = other.decryptedSize;
			other.host = nullptr;
			other.secure = true;
			other.stateIndex = 0;
			other.decryptedPos = 0;
			other.decryptedSize = 0;
		}
		return *this;
	}

	constexpr BOOL IsValid() const { return context.IsValid(); }
	constexpr BOOL IsSecure() const { return secure; }
	[[nodiscard]] Result<void, Error> Open();
	[[nodiscard]] Result<void, Error> Close();
	[[nodiscard]] Result<SSIZE, Error> Read(Span<CHAR> buffer);
	[[nodiscard]] Result<UINT32, Error> Write(Span<const CHAR> buffer);
};
