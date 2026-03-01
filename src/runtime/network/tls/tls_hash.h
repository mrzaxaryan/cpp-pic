#pragma once

/**
 * @file tls_hash.h
 * @brief TLS 1.3 transcript hash for handshake verification
 *
 * @details Accumulates handshake messages and computes the transcript hash
 * used in TLS 1.3 key derivation and Finished message verification.
 * Supports SHA-256 and SHA-384 depending on the negotiated cipher suite.
 *
 * @see RFC 8446 Section 4.4.1 — Transcript Hash
 *      https://datatracker.ietf.org/doc/html/rfc8446#section-4.4.1
 */

#include "core/core.h"
#include "runtime/network/tls/tls_buffer.h"

/// TLS transcript hash accumulator for handshake verification
class TlsHash
{
private:
	TlsBuffer cache;
public:
	TlsHash() = default;
	~TlsHash() = default;

	// Non-copyable
	TlsHash(const TlsHash &) = delete;
	TlsHash &operator=(const TlsHash &) = delete;

	// Movable
	TlsHash(TlsHash &&) = default;
	TlsHash &operator=(TlsHash &&) = default;

	// Stack-only
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;

	// Reset the hash cache
	VOID Reset();
	// Append data to the hash cache
	VOID Append(Span<const CHAR> buffer);
	// Get the hash value from the cache; dispatch on out.Size() (32 = SHA-256, 48 = SHA-384)
	VOID GetHash(Span<CHAR> out);
};