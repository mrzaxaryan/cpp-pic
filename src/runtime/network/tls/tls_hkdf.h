#pragma once

/**
 * @file tls_hkdf.h
 * @brief HKDF (HMAC-based Key Derivation Function) for TLS 1.3 key schedule
 *
 * @details Implements HKDF-Extract and HKDF-Expand (RFC 5869) with the TLS 1.3
 * label construction (RFC 8446 Section 7.1) used to derive handshake and
 * application traffic keys from shared secrets.
 *
 * @see RFC 5869 — HMAC-based Extract-and-Expand Key Derivation Function (HKDF)
 *      https://datatracker.ietf.org/doc/html/rfc5869
 * @see RFC 8446 Section 7.1 — Key Schedule
 *      https://datatracker.ietf.org/doc/html/rfc8446#section-7.1
 */

#include "core/core.h"

/// HKDF key derivation for TLS 1.3 key schedule
class TlsHKDF
{
private:
	static INT32 Label(Span<const CHAR> label, Span<const UCHAR> data, Span<UCHAR> hkdflabel, UINT16 length);

public:
	static VOID Extract(Span<UCHAR> output, Span<const UCHAR> salt, Span<const UCHAR> ikm);
	static VOID Expand(Span<UCHAR> output, Span<const UCHAR> secret, Span<const UCHAR> info);
	static VOID ExpandLabel(Span<UCHAR> output, Span<const UCHAR> secret, Span<const CHAR> label, Span<const UCHAR> data);
};