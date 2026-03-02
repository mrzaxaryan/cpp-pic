#pragma once

/**
 * @file tls_hash.h
 * @brief TLS 1.3 transcript hash for handshake verification
 *
 * @details Maintains a running SHA-256 hash context that is updated incrementally
 * as handshake messages arrive. GetHash() snapshots the current state without
 * consuming it, allowing the hash to continue accumulating further messages.
 *
 * This replaces the previous buffer-based approach (which cached all handshake
 * bytes and re-hashed from scratch on every GetHash() call) with O(n) incremental
 * hashing and zero heap allocation.
 *
 * @see RFC 8446 Section 4.4.1 — Transcript Hash
 *      https://datatracker.ietf.org/doc/html/rfc8446#section-4.4.1
 */

#include "core/core.h"
#include "runtime/crypto/sha2.h"

/// TLS transcript hash accumulator for handshake verification
class TlsHash
{
private:
	SHA256 ctx;
public:
	TlsHash() = default;
	~TlsHash() = default;

	// Non-copyable
	TlsHash(const TlsHash &) = delete;
	TlsHash &operator=(const TlsHash &) = delete;

	// Movable — copies running SHA state, source remains valid
	TlsHash(TlsHash &&other) noexcept { ctx.CopyStateFrom(other.ctx); }
	TlsHash &operator=(TlsHash &&other) noexcept
	{
		if (this != &other)
		{
			ctx.~SHA256();
			new (&ctx) SHA256();
			ctx.CopyStateFrom(other.ctx);
		}
		return *this;
	}

	// Stack-only
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;

	/**
	 * @brief Resets the transcript hash to its initial state
	 */
	VOID Reset();

	/**
	 * @brief Updates the running transcript hash with new handshake data
	 * @param buffer Data to append
	 *
	 * @see RFC 8446 Section 4.4.1 — Transcript Hash
	 *      https://datatracker.ietf.org/doc/html/rfc8446#section-4.4.1
	 */
	VOID Append(Span<const CHAR> buffer);

	/**
	 * @brief Computes the transcript hash from accumulated data
	 * @param out Output span for the hash digest (must be SHA256_DIGEST_SIZE bytes)
	 *
	 * @details Snapshots the running SHA-256 context and finalizes the copy,
	 * leaving the original context intact for further Append() calls.
	 *
	 * @see RFC 8446 Section 4.4.1 — Transcript Hash
	 *      https://datatracker.ietf.org/doc/html/rfc8446#section-4.4.1
	 */
	VOID GetHash(Span<CHAR> out);
};
