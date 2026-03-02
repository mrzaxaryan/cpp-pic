#pragma once

/**
 * @file tls_cipher.h
 * @brief TLS 1.3 cipher suite management and record-layer encryption
 *
 * @details Manages cipher suite negotiation, ECDHE key exchange, handshake hashing,
 * and record-layer encryption/decryption for TLS 1.3 connections. Supports
 * ChaCha20-Poly1305 (RFC 8439) as the AEAD cipher and secp256r1/secp384r1
 * (RFC 8422) for elliptic curve key exchange.
 *
 * @see RFC 8446 Section 5 — Record Protocol
 *      https://datatracker.ietf.org/doc/html/rfc8446#section-5
 * @see RFC 8439 — ChaCha20 and Poly1305 for IETF Protocols
 *      https://datatracker.ietf.org/doc/html/rfc8439
 * @see RFC 8422 — Elliptic Curve Cryptography (ECC) Cipher Suites for TLS
 *      https://datatracker.ietf.org/doc/html/rfc8422
 */

#include "core/core.h"
#include "runtime/network/tls/tls_buffer.h"
#include "runtime/crypto/ecc.h"
#include "runtime/network/tls/tls_hash.h"
#include "runtime/crypto/chacha20_encoder.h"

/// Number of supported ECC curves
constexpr INT32 ECC_COUNT = 2;
/// Size of random nonce in bytes (RFC 8446 Section 4.1.2)
constexpr INT32 RAND_SIZE = 32;
/// Maximum hash output length in bytes (SHA-384 = 48, with padding)
constexpr INT32 MAX_HASH_LEN = 64;
/// Maximum public key buffer size in bytes
constexpr INT32 MAX_PUBKEY_SIZE = 2048;
/// Maximum symmetric key size in bytes
constexpr INT32 MAX_KEY_SIZE = 32;
/// Maximum IV (nonce) size in bytes (RFC 8446 Section 5.3)
constexpr INT32 MAX_IV_SIZE = 12;
/// ChaCha20-Poly1305 key size in bytes (RFC 8439 Section 2.3)
constexpr INT32 CIPHER_KEY_SIZE = 32;
/// ChaCha20-Poly1305 authentication tag size in bytes (RFC 8439 Section 2.5)
constexpr INT32 CIPHER_HASH_SIZE = 32;
/// TLS content type for application data (RFC 8446 Section 5.1)
constexpr UINT8 CONTENT_APPLICATION_DATA = 0x17;

/// @brief Supported elliptic curve groups for TLS key exchange
/// @see RFC 8422 Section 5.1.1 — Supported Elliptic Curves Extension
///      https://datatracker.ietf.org/doc/html/rfc8422#section-5.1.1
/// @see RFC 7919 — Negotiated Finite Field Diffie-Hellman Ephemeral Parameters
///      https://datatracker.ietf.org/doc/html/rfc7919
enum class EccGroup : UINT16
{
	None = 0,           ///< No ECC support; implies RSA key exchange
	Secp256r1 = 0x0017, ///< secp256r1 (NIST P-256) curve (RFC 8422 Section 5.1.1)
	Secp384r1 = 0x0018, ///< secp384r1 (NIST P-384) curve (RFC 8422 Section 5.1.1)
};

/// TLS 1.3 cipher suite management and record-layer encryption/decryption
class TlsCipher
{
private:
	INT32 cipherCount;              ///< Number of supported ciphers
	UINT64 clientSeqNum;            ///< Client record sequence number (RFC 8446 Section 5.3)
	UINT64 serverSeqNum;            ///< Server record sequence number (RFC 8446 Section 5.3)
	ECC *privateEccKeys[ECC_COUNT]; ///< Ephemeral ECDH private keys per supported curve
	TlsBuffer publicKey;            ///< Serialized public key for key_share extension
	TlsBuffer decodeBuffer;         ///< Scratch buffer for record decryption
	TlsHash handshakeHash;          ///< Running transcript hash (RFC 8446 Section 4.4.1)

	union
	{
		struct
		{
			UINT8 mainSecret[MAX_HASH_LEN];      ///< TLS 1.3 master secret (RFC 8446 Section 7.1)
			UINT8 handshakeSecret[MAX_HASH_LEN]; ///< Handshake secret (RFC 8446 Section 7.1)
			UINT8 pseudoRandomKey[MAX_HASH_LEN]; ///< Early secret / PRK for key schedule
		} data13;
		struct
		{
			UINT8 clientRandom[RAND_SIZE]; ///< Client random value
			UINT8 serverRandom[RAND_SIZE]; ///< Server random value
			UINT8 masterKey[48];           ///< Master key
		} data12;
	};
	INT32 cipherIndex;               ///< Index of negotiated cipher suite
	ChaCha20Encoder chacha20Context; ///< ChaCha20-Poly1305 AEAD context (RFC 8439)
	BOOL isEncoding;                 ///< Whether record-layer encryption is active

public:
	// Constructor — trivial, call Reset() before use
	TlsCipher() : cipherCount(0), clientSeqNum(0), serverSeqNum(0), privateEccKeys{}, cipherIndex(-1), isEncoding(false) {}
	~TlsCipher() { Destroy(); }

	TlsCipher(const TlsCipher &) = delete;
	TlsCipher &operator=(const TlsCipher &) = delete;

	// Stack-only
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;

	TlsCipher(TlsCipher &&other) noexcept
		: cipherCount(other.cipherCount)
		, clientSeqNum(other.clientSeqNum)
		, serverSeqNum(other.serverSeqNum)
		, publicKey(static_cast<TlsBuffer &&>(other.publicKey))
		, decodeBuffer(static_cast<TlsBuffer &&>(other.decodeBuffer))
		, handshakeHash(static_cast<TlsHash &&>(other.handshakeHash))
		, cipherIndex(other.cipherIndex)
		, chacha20Context(static_cast<ChaCha20Encoder &&>(other.chacha20Context))
		, isEncoding(other.isEncoding)
	{
		for (INT32 i = 0; i < ECC_COUNT; i++)
		{
			privateEccKeys[i] = other.privateEccKeys[i];
			other.privateEccKeys[i] = nullptr;
		}
		Memory::Copy(&data13, &other.data13, Math::Max(sizeof(data13), sizeof(data12)));
		// Zero sensitive key material in the moved-from object immediately after copying
		Memory::Zero(&other.data13, Math::Max(sizeof(other.data13), sizeof(other.data12)));
	}

	TlsCipher &operator=(TlsCipher &&other) noexcept
	{
		if (this != &other)
		{
			Destroy();
			cipherCount = other.cipherCount;
			clientSeqNum = other.clientSeqNum;
			serverSeqNum = other.serverSeqNum;
			publicKey = static_cast<TlsBuffer &&>(other.publicKey);
			decodeBuffer = static_cast<TlsBuffer &&>(other.decodeBuffer);
			handshakeHash = static_cast<TlsHash &&>(other.handshakeHash);
			cipherIndex = other.cipherIndex;
			chacha20Context = static_cast<ChaCha20Encoder &&>(other.chacha20Context);
			isEncoding = other.isEncoding;
			for (INT32 i = 0; i < ECC_COUNT; i++)
			{
				privateEccKeys[i] = other.privateEccKeys[i];
				other.privateEccKeys[i] = nullptr;
			}
			Memory::Copy(&data13, &other.data13, Math::Max(sizeof(data13), sizeof(data12)));
			// Zero sensitive key material in the moved-from object immediately after copying
			Memory::Zero(&other.data13, Math::Max(sizeof(other.data13), sizeof(other.data12)));
		}
		return *this;
	}

	/**
	 * @brief Resets the cipher to its initial state, freeing ECC keys and zeroing secrets
	 */
	VOID Reset();

	/**
	 * @brief Destroys the cipher, releasing all resources and zeroing key material
	 */
	VOID Destroy();

	/**
	 * @brief Generates and returns the client random value for the ClientHello message
	 * @return Span wrapping the generated client random data (RAND_SIZE bytes)
	 *
	 * @see RFC 8446 Section 4.1.2 — Client Hello (client random field)
	 *      https://datatracker.ietf.org/doc/html/rfc8446#section-4.1.2
	 */
	Span<const UINT8> CreateClientRand();

	/**
	 * @brief Updates server information after receiving the ServerHello cipher suite
	 * @return Result<void, Error>::Ok() if the update was successful
	 */
	[[nodiscard]] Result<void, Error> UpdateServerInfo();

	/**
	 * @brief Gets the current transcript hash and stores it in the provided output span
	 * @param out Output span; size determines which hash algorithm is used
	 *            (32 = SHA-256, 48 = SHA-384)
	 *
	 * @see RFC 8446 Section 4.4.1 — Transcript Hash
	 *      https://datatracker.ietf.org/doc/html/rfc8446#section-4.4.1
	 */
	VOID GetHash(Span<CHAR> out);

	/**
	 * @brief Updates the running transcript hash with new handshake data
	 * @param in Input data to be added to the transcript hash
	 *
	 * @see RFC 8446 Section 4.4.1 — Transcript Hash
	 *      https://datatracker.ietf.org/doc/html/rfc8446#section-4.4.1
	 */
	VOID UpdateHash(Span<const CHAR> in);

	/**
	 * @brief Computes the ephemeral ECDH public key for the key_share extension
	 * @param eccIndex Index of the ECC key to use (0 = secp256r1, 1 = secp384r1)
	 * @param out Buffer where the computed public key will be appended
	 * @return Result<void, Error>::Ok() if the public key was successfully computed
	 *
	 * @see RFC 8446 Section 4.2.8 — Key Share
	 *      https://datatracker.ietf.org/doc/html/rfc8446#section-4.2.8
	 */
	[[nodiscard]] Result<void, Error> ComputePublicKey(INT32 eccIndex, TlsBuffer &out);

	/**
	 * @brief Computes the pre-master key using ECDH shared secret derivation
	 * @param ecc Specified ECC group to use for key computation
	 * @param serverKey Server's public key for pre-master key computation
	 * @param premasterKey Buffer where the computed pre-master key will be stored
	 * @return Result<void, Error>::Ok() if the pre-master key was successfully computed
	 *
	 * @see RFC 8446 Section 7.4 — (EC)DHE Shared Secret
	 *      https://datatracker.ietf.org/doc/html/rfc8446#section-7.4
	 */
	[[nodiscard]] Result<void, Error> ComputePreKey(EccGroup ecc, Span<const CHAR> serverKey, TlsBuffer &premasterKey);

	/**
	 * @brief Derives handshake or application traffic keys from the key schedule
	 * @param ecc Specified ECC group (None for application keys derivation)
	 * @param serverKey Server's public key for TLS key computation
	 * @param finishedHash Transcript hash at the point of Finished message
	 * @return Result<void, Error>::Ok() if the TLS key was successfully computed
	 *
	 * @see RFC 8446 Section 7.1 — Key Schedule
	 *      https://datatracker.ietf.org/doc/html/rfc8446#section-7.1
	 */
	[[nodiscard]] Result<void, Error> ComputeKey(EccGroup ecc, Span<const CHAR> serverKey, Span<CHAR> finishedHash);

	/**
	 * @brief Computes the verify data for the TLS Finished message
	 * @param out Buffer where the computed verify data will be stored
	 * @param verifySize Size of the verify data to compute
	 * @param localOrRemote 0 for client finished key, 1 for server finished key
	 * @return Result<void, Error>::Ok() if the verify data was successfully computed
	 *
	 * @see RFC 8446 Section 4.4.4 — Finished
	 *      https://datatracker.ietf.org/doc/html/rfc8446#section-4.4.4
	 */
	[[nodiscard]] Result<void, Error> ComputeVerify(TlsBuffer &out, INT32 verifySize, INT32 localOrRemote);

	/**
	 * @brief Encodes a TLS record using ChaCha20-Poly1305 AEAD encryption
	 * @param sendbuf Buffer where the encoded TLS record will be appended
	 * @param packet TLS record data to encode
	 * @param keepOriginal If true, appends data without encryption
	 * @param innerContentType When >= 0 and encryption is active, appended to
	 *        the plaintext before encryption (RFC 8446 Section 5.2 inner content type).
	 *        Pass -1 to omit.
	 *
	 * @see RFC 8446 Section 5.2 — Record Payload Protection
	 *      https://datatracker.ietf.org/doc/html/rfc8446#section-5.2
	 */
	VOID Encode(TlsBuffer &sendbuf, Span<const CHAR> packet, BOOL keepOriginal, INT32 innerContentType = -1);

	/**
	 * @brief Decodes a TLS record using ChaCha20-Poly1305 AEAD decryption
	 * @param inout Buffer containing the TLS record; updated with decoded data
	 * @param version TLS version of the record to decode
	 * @return Result<void, Error>::Ok() if the TLS record was successfully decoded
	 *
	 * @see RFC 8446 Section 5.2 — Record Payload Protection
	 *      https://datatracker.ietf.org/doc/html/rfc8446#section-5.2
	 */
	[[nodiscard]] Result<void, Error> Decode(TlsBuffer &inout, INT32 version);

	/**
	 * @brief Sets whether record-layer encryption is active
	 * @param encoding true to enable encryption, false to disable
	 */
	constexpr VOID SetEncoding(BOOL encoding) { isEncoding = encoding; }

	/**
	 * @brief Resets both client and server record sequence numbers to zero
	 *
	 * @see RFC 8446 Section 5.3 — Per-Record Nonce
	 *      https://datatracker.ietf.org/doc/html/rfc8446#section-5.3
	 */
	constexpr VOID ResetSequenceNumber() { clientSeqNum = 0; serverSeqNum = 0; }

	/** @brief Returns true if the cipher has been initialized with at least one cipher suite */
	constexpr BOOL IsValid() const { return cipherCount > 0; }

	/** @brief Returns true if record-layer encryption is currently active */
	constexpr BOOL GetEncoding() const { return isEncoding; }

	/** @brief Returns the number of supported cipher suites */
	constexpr INT32 GetCipherCount() const { return cipherCount; }

	/** @brief Returns the current decoded application data as a read-only span */
	Span<const CHAR> GetDecodedData() const { return decodeBuffer.AsSpan(); }

	/** @brief Returns a reference to the serialized public key buffer */
	TlsBuffer &GetPubKey() { return publicKey; }

	/**
	 * @brief Sets the number of supported cipher suites
	 * @param count Number of cipher suites
	 */
	constexpr VOID SetCipherCount(INT32 count) { cipherCount = count; }
};
