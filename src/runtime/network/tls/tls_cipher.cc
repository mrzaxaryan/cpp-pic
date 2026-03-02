#include "runtime/network/tls/tls_cipher.h"
#include "platform/io/logger.h"
#include "core/memory/memory.h"
#include "platform/system/random.h"
#include "runtime/crypto/sha2.h"
#include "runtime/network/tls/tls_hkdf.h"
#include "core/math/math.h"

/// ECC key size for secp256r1 (NIST P-256) in bytes
constexpr INT32 SECP256R1_KEY_SIZE = 32;
/// ECC key size for secp384r1 (NIST P-384) in bytes
constexpr INT32 SECP384R1_KEY_SIZE = 48;
/// Length of TLS 1.3 traffic label strings (e.g., "s hs traffic", "c ap traffic")
constexpr INT32 TRAFFIC_LABEL_LEN = 12;

/// @brief Reset the TlsCipher object to its initial state

VOID TlsCipher::Reset()
{
	for (INT32 i = 0; i < ECC_COUNT; i++)
	{
		if (privateEccKeys[i])
		{
			LOG_DEBUG("Freeing ECC key: %p", privateEccKeys[i]);
			delete privateEccKeys[i];
			privateEccKeys[i] = nullptr;
		}
	}
	Memory::Zero(privateEccKeys, sizeof(privateEccKeys));
	publicKey.Clear();
	decodeBuffer.Clear();
	LOG_DEBUG("Resetting tls_cipher structure for cipher: %p", this);
	Memory::Zero(&data12, Math::Max(sizeof(data12), sizeof(data13)));
	SetCipherCount(1);
	clientSeqNum = 0;
	serverSeqNum = 0;
	handshakeHash.Reset();
	cipherIndex = -1;
	isEncoding = false;
}

/// @brief Destroy the TlsCipher object and clean up resources

VOID TlsCipher::Destroy()
{
	Reset();
	publicKey.Clear();
	decodeBuffer.Clear();
}

/// @brief Generate and return the client random value for the ClientHello message
/// @return Span wrapping the generated client random data (RAND_SIZE bytes)
/// @see RFC 8446 Section 4.1.2 — Client Hello (client random field)
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4.1.2

Span<const UINT8> TlsCipher::CreateClientRand()
{
	Random random;

	LOG_DEBUG("Creating client random data for cipher: %p", this);
	for (USIZE i = 0; i < sizeof(data12.clientRandom); i++)
	{
		data12.clientRandom[i] = random.Get() & 0xff;
	}
	LOG_DEBUG("Client random data created: %p", data12.clientRandom);
	return Span<const UINT8>(data12.clientRandom, RAND_SIZE);
}

/// @brief Update server information for the TLS cipher
/// @return Result<void, Error>::Ok() if the update was successful

Result<void, Error> TlsCipher::UpdateServerInfo()
{
	cipherIndex = 0;

	return Result<void, Error>::Ok();
}

/// @brief Get the current handshake hash and store it in the provided output span
/// @param out Output span; size determines which hash algorithm is used
/// @see RFC 8446 Section 4.4.1 — Transcript Hash
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4.4.1

VOID TlsCipher::GetHash(Span<CHAR> out)
{
	handshakeHash.GetHash(out);
}

/// @brief Update the handshake hash with new input data
/// @param in Input data to be added to the handshake hash
/// @see RFC 8446 Section 4.4.1 — Transcript Hash
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4.4.1

VOID TlsCipher::UpdateHash(Span<const CHAR> in)
{
	handshakeHash.Append(in);
}

/// @brief Compute the public key for the specified ECC index and store it in the provided output buffer
/// @param eccIndex Index of the ECC key to use for public key computation
/// @param out Buffer where the computed public key will be appended
/// @return Result<void, Error>::Ok() if the public key was successfully computed
/// @see RFC 8446 Section 4.2.8 — Key Share
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4.2.8

Result<void, Error> TlsCipher::ComputePublicKey(INT32 eccIndex, TlsBuffer &out)
{
	if (privateEccKeys[eccIndex] == nullptr)
	{
		LOG_DEBUG("Allocating memory for private ECC key at index %d", eccIndex);
		privateEccKeys[eccIndex] = new ECC();
		INT32 eccSizeList[2];
		eccSizeList[0] = SECP256R1_KEY_SIZE;
		eccSizeList[1] = SECP384R1_KEY_SIZE;

		auto initResult = privateEccKeys[eccIndex]->Initialize(eccSizeList[eccIndex]);
		if (!initResult)
		{
			LOG_DEBUG("Failed to initialize ECC key at index %d", eccIndex);
			return Result<void, Error>::Err(initResult, Error::TlsCipher_ComputePublicKeyFailed);
		}
	}

	auto checkResult = out.CheckSize(MAX_PUBKEY_SIZE);
	if (!checkResult)
		return Result<void, Error>::Err(checkResult, Error::TlsCipher_ComputePublicKeyFailed);

	auto exportResult = privateEccKeys[eccIndex]->ExportPublicKey(Span<UINT8>((UINT8 *)out.GetBuffer() + out.GetSize(), MAX_PUBKEY_SIZE));
	if (!exportResult)
		return Result<void, Error>::Err(exportResult, Error::TlsCipher_ComputePublicKeyFailed);
	auto setSizeResult = out.SetSize(out.GetSize() + exportResult.Value());
	if (!setSizeResult)
		return Result<void, Error>::Err(setSizeResult, Error::TlsCipher_ComputePublicKeyFailed);

	return Result<void, Error>::Ok();
}

/// @brief Compute the pre-master key using the specified ECC group and server key
/// @param ecc Specified ECC group to use for key computation
/// @param serverKey Server's public key for pre-master key computation
/// @param premasterKey Buffer where the computed pre-master key will be stored
/// @return Result<void, Error>::Ok() if the pre-master key was successfully computed
/// @see RFC 8446 Section 7.4 — (EC)DHE Shared Secret
///      https://datatracker.ietf.org/doc/html/rfc8446#section-7.4

Result<void, Error> TlsCipher::ComputePreKey(EccGroup ecc, Span<const CHAR> serverKey, TlsBuffer &premasterKey)
{
	INT32 eccIndex;
	INT32 eccSize;

	// Replace loop with two if statements
	if (ecc == EccGroup::Secp256r1)
	{
		eccSize = SECP256R1_KEY_SIZE;
		eccIndex = 0;
	}
	else if (ecc == EccGroup::Secp384r1)
	{
		eccSize = SECP384R1_KEY_SIZE;
		eccIndex = 1;
	}
	else
	{
		return Result<void, Error>::Err(Error::TlsCipher_ComputePreKeyFailed);
	}
	auto pubKeyResult = ComputePublicKey(eccIndex, publicKey);
	if (!pubKeyResult)
	{
		LOG_DEBUG("Failed to compute public key for ECC group %d", (UINT16)ecc);
		return Result<void, Error>::Err(pubKeyResult, Error::TlsCipher_ComputePreKeyFailed);
	}

	auto setSizeResult = premasterKey.SetSize(eccSize);
	if (!setSizeResult)
		return Result<void, Error>::Err(setSizeResult, Error::TlsCipher_ComputePreKeyFailed);

	auto secretResult = privateEccKeys[eccIndex]->ComputeSharedSecret(Span<const UINT8>((UINT8 *)serverKey.Data(), serverKey.Size()), Span<UINT8>((UINT8 *)premasterKey.GetBuffer(), eccSize));
	if (!secretResult)
	{
		LOG_DEBUG("Failed to compute shared secret for ECC group %d", (UINT16)ecc);
		return Result<void, Error>::Err(secretResult, Error::TlsCipher_ComputePreKeyFailed);
	}

	return Result<void, Error>::Ok();
}

/// @brief Compute the TLS key using the specified ECC group and server key
/// @param ecc Specified ECC group to use for key computation
/// @param serverKey Server's public key for TLS key computation
/// @param finishedHash Buffer where the computed finished hash will be stored
/// @return Result<void, Error>::Ok() if the TLS key was successfully computed
/// @see RFC 8446 Section 7.1 — Key Schedule
///      https://datatracker.ietf.org/doc/html/rfc8446#section-7.1

Result<void, Error> TlsCipher::ComputeKey(EccGroup ecc, Span<const CHAR> serverKey, Span<CHAR> finishedHash)
{

	if (cipherIndex == -1)
	{
		LOG_DEBUG("Cipher index is -1, cannot compute TLS key");
		return Result<void, Error>::Err(Error::TlsCipher_ComputeKeyFailed);
	}
	LOG_DEBUG("Computing TLS key for cipher: %p, ECC group: %d", this, ecc);

	INT32 keyLen = CIPHER_KEY_SIZE;
	INT32 hashLen = CIPHER_HASH_SIZE;

	UINT8 hash[MAX_HASH_LEN];
	UINT8 earlysecret[MAX_HASH_LEN], salt[MAX_HASH_LEN];
	UINT8 localKeyBuffer[MAX_KEY_SIZE], remoteKeyBuffer[MAX_KEY_SIZE];
	UINT8 localIvBuffer[MAX_IV_SIZE], remoteIvBuffer[MAX_IV_SIZE];
	// Declare _embed strings separately to avoid type deduction issues with ternary
	auto serverKeyApp = "s ap traffic"_embed;
	auto serverKeyHs = "s hs traffic"_embed;
	auto clientKeyApp = "c ap traffic"_embed;
	auto clientKeyHs = "c hs traffic"_embed;
	PCCHAR serverLabel = ecc == EccGroup::None ? (PCCHAR)serverKeyApp : (PCCHAR)serverKeyHs;
	PCCHAR clientLabel = ecc == EccGroup::None ? (PCCHAR)clientKeyApp : (PCCHAR)clientKeyHs;
	auto derivedLabel = "derived"_embed;
	auto keyLabel = "key"_embed;
	auto ivLabel = "iv"_embed;
	TlsHash hash2;
	hash2.GetHash(Span<CHAR>((CHAR *)hash, hashLen));
	Memory::Zero(earlysecret, sizeof(earlysecret));

	if (ecc == EccGroup::None)
	{
		LOG_DEBUG("Using EccGroup::None for TLS key computation");

		TlsHkdf::ExpandLabel(Span<UCHAR>(salt, hashLen), Span<const UCHAR>((UINT8 *)data13.pseudoRandomKey, hashLen), Span<const CHAR>(derivedLabel, derivedLabel.Length()), Span<const UCHAR>(hash, hashLen));
		TlsHkdf::Extract(Span<UCHAR>(data13.pseudoRandomKey, hashLen), Span<const UCHAR>(salt, hashLen), Span<const UCHAR>(earlysecret, hashLen));

		if (finishedHash.Data())
		{
			LOG_DEBUG("Using finished hash for TLS key computation with size: %d bytes", (INT32)finishedHash.Size());
			Memory::Copy(hash, (VOID *)finishedHash.Data(), hashLen);
		}
	}
	else
	{
		TlsBuffer premasterKey;
		auto preKeyResult = ComputePreKey(ecc, serverKey, premasterKey);
		if (!preKeyResult)
		{
			LOG_DEBUG("Failed to compute pre-master key for ECC group %d", (UINT16)ecc);
			return Result<void, Error>::Err(preKeyResult, Error::TlsCipher_ComputeKeyFailed);
		}
		LOG_DEBUG("Computed pre-master key for ECC group %d, size: %d bytes", ecc, premasterKey.GetSize());

		// RFC 8446 §7.1: the initial Extract uses a salt of HashLen zero bytes
		UCHAR zeroSalt[MAX_HASH_LEN];
		Memory::Zero(zeroSalt, hashLen);

		TlsHkdf::Extract(Span<UCHAR>(data13.pseudoRandomKey, hashLen), Span<const UCHAR>(zeroSalt, hashLen), Span<const UCHAR>(earlysecret, hashLen));
		TlsHkdf::ExpandLabel(Span<UCHAR>(salt, hashLen), Span<const UCHAR>(data13.pseudoRandomKey, hashLen), Span<const CHAR>(derivedLabel, derivedLabel.Length()), Span<const UCHAR>(hash, hashLen));
		TlsHkdf::Extract(Span<UCHAR>(data13.pseudoRandomKey, hashLen), Span<const UCHAR>(salt, hashLen), Span<const UCHAR>((UINT8 *)premasterKey.GetBuffer(), premasterKey.GetSize()));

		GetHash(Span<CHAR>((CHAR *)hash, CIPHER_HASH_SIZE));
	}

	TlsHkdf::ExpandLabel(Span<UCHAR>(data13.handshakeSecret, hashLen), Span<const UCHAR>(data13.pseudoRandomKey, hashLen), Span<const CHAR>(clientLabel, TRAFFIC_LABEL_LEN), Span<const UCHAR>(hash, hashLen));

	TlsHkdf::ExpandLabel(Span<UCHAR>(localKeyBuffer, keyLen), Span<const UCHAR>(data13.handshakeSecret, hashLen), Span<const CHAR>(keyLabel, keyLabel.Length()), Span<const UCHAR>());
	TlsHkdf::ExpandLabel(Span<UCHAR>(localIvBuffer, chacha20Context.GetIvLength()), Span<const UCHAR>(data13.handshakeSecret, hashLen), Span<const CHAR>(ivLabel, ivLabel.Length()), Span<const UCHAR>());

	TlsHkdf::ExpandLabel(Span<UCHAR>(data13.mainSecret, hashLen), Span<const UCHAR>(data13.pseudoRandomKey, hashLen), Span<const CHAR>(serverLabel, TRAFFIC_LABEL_LEN), Span<const UCHAR>(hash, hashLen));

	TlsHkdf::ExpandLabel(Span<UCHAR>(remoteKeyBuffer, keyLen), Span<const UCHAR>(data13.mainSecret, hashLen), Span<const CHAR>(keyLabel, keyLabel.Length()), Span<const UCHAR>());
	TlsHkdf::ExpandLabel(Span<UCHAR>(remoteIvBuffer, chacha20Context.GetIvLength()), Span<const UCHAR>(data13.mainSecret, hashLen), Span<const CHAR>(ivLabel, ivLabel.Length()), Span<const UCHAR>());

	auto initResult = chacha20Context.Initialize(Span<const UINT8, POLY1305_KEYLEN>(localKeyBuffer), Span<const UINT8, POLY1305_KEYLEN>(remoteKeyBuffer), localIvBuffer, remoteIvBuffer);
	if (!initResult)
	{
		LOG_DEBUG("Failed to initialize encoder with local key: %p, remote key: %p", localKeyBuffer, remoteKeyBuffer);
		return Result<void, Error>::Err(initResult, Error::TlsCipher_ComputeKeyFailed);
	}

	LOG_DEBUG("Encoder initialized with local key: %p, remote key: %p", localKeyBuffer, remoteKeyBuffer);
	return Result<void, Error>::Ok();
}

/// @brief Compute the verify data for the TLS handshake
/// @param out Buffer where the computed verify data will be stored
/// @param verifySize Size of the verify data to compute
/// @param localOrRemote Indicates whether to use the local (0) or remote (1) finished key
/// @return Result<void, Error> Success or error if cipher is not initialized
/// @see RFC 8446 Section 4.4.4 — Finished
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4.4.4

Result<void, Error> TlsCipher::ComputeVerify(TlsBuffer &out, INT32 verifySize, INT32 localOrRemote)
{
	if (cipherIndex == -1)
	{
		LOG_DEBUG("Cipher index is -1, cannot compute verify data");
		return Result<void, Error>::Err(Error::TlsCipher_ComputeVerifyFailed);
	}
	CHAR hash[MAX_HASH_LEN];
	INT32 hashLen = CIPHER_HASH_SIZE;
	LOG_DEBUG("Computing verify data, hashLen=%d", hashLen);
	GetHash(Span<CHAR>(hash, hashLen));

	UINT8 finishedKey[MAX_HASH_LEN];
	auto finishedLabel = "finished"_embed;
	if (localOrRemote)
	{
		LOG_DEBUG("Using server finished key");
		TlsHkdf::ExpandLabel(Span<UCHAR>(finishedKey, hashLen), Span<const UCHAR>(data13.mainSecret, hashLen), Span<const CHAR>(finishedLabel, finishedLabel.Length()), Span<const UCHAR>());
	}
	else
	{
		LOG_DEBUG("Using client finished key");
		TlsHkdf::ExpandLabel(Span<UCHAR>(finishedKey, hashLen), Span<const UCHAR>(data13.handshakeSecret, hashLen), Span<const CHAR>(finishedLabel, finishedLabel.Length()), Span<const UCHAR>());
	}
	auto setSizeResult = out.SetSize(verifySize);
	if (!setSizeResult)
		return Result<void, Error>::Err(setSizeResult, Error::TlsCipher_ComputeVerifyFailed);
	LOG_DEBUG("Calculating HMAC for verify, verifySize=%d", verifySize);
	HMAC_SHA256 hmac;
	hmac.Init(Span<const UCHAR>(finishedKey, hashLen));
	hmac.Update(Span<const UCHAR>((UINT8 *)hash, hashLen));

	hmac.Final(Span<UCHAR>((UINT8 *)out.GetBuffer(), out.GetSize()));
	LOG_DEBUG("Finished verify computation");
	return Result<void, Error>::Ok();
}

/// @brief Encode a TLS record using the ChaCha20 encoder and append it to the send buffer
/// @param sendbuf Buffer where the encoded TLS record will be appended
/// @param packet TLS record data to encode
/// @param keepOriginal Indicates whether to keep the original TLS record without encoding
/// @see RFC 8446 Section 5.2 — Record Payload Protection
///      https://datatracker.ietf.org/doc/html/rfc8446#section-5.2

VOID TlsCipher::Encode(TlsBuffer &sendbuf, Span<const CHAR> packet, BOOL keepOriginal, INT32 innerContentType)
{
	if (!isEncoding || keepOriginal)
	{
		LOG_DEBUG("Encoding not enabled or encoder is nullptr, appending packet directly to sendbuf");
		sendbuf.Append(packet);
		return;
	}

	Span<const CHAR> plaintext = packet;
	TlsBuffer concatenated;

	if (innerContentType >= 0)
	{
		concatenated.Append(packet);
		concatenated.Append<CHAR>((CHAR)innerContentType);
		plaintext = concatenated.AsSpan();
	}

	LOG_DEBUG("Encoding packet with size: %d bytes", (INT32)plaintext.Size());

	UCHAR aad[13];

	aad[0] = CONTENT_APPLICATION_DATA;
	aad[1] = sendbuf.GetBuffer()[1];
	aad[2] = sendbuf.GetBuffer()[2];
	UINT16 encSize = (UINT16)ChaCha20Encoder::ComputeSize((INT32)plaintext.Size(), CipherDirection::Encode);
	aad[3] = (UINT8)(encSize >> 8);
	aad[4] = (UINT8)(encSize & 0xFF);
	UINT64 clientSeq = UINT64SwapByteOrder(clientSeqNum++);
	Memory::Copy(aad + 5, &clientSeq, sizeof(UINT64));

	chacha20Context.Encode(sendbuf, plaintext, Span<const UCHAR>(aad));
}

/// @brief Decode a TLS record using the ChaCha20 encoder
/// @param inout Buffer containing the TLS record to decode; updated with decoded data
/// @param version TLS version of the record to decode
/// @return Result<void, Error>::Ok() if the TLS record was successfully decoded
/// @see RFC 8446 Section 5.2 — Record Payload Protection
///      https://datatracker.ietf.org/doc/html/rfc8446#section-5.2

Result<void, Error> TlsCipher::Decode(TlsBuffer &inout, INT32 version)
{
	if (!isEncoding)
	{
		LOG_DEBUG("Encoding not enabled or encoder is nullptr, cannot Decode packet");
		return Result<void, Error>::Ok();
	}
	UCHAR aad[13];

	aad[0] = CONTENT_APPLICATION_DATA;
	aad[1] = UINT16SwapByteOrder(version) >> 8;
	aad[2] = UINT16SwapByteOrder(version) & 0xff;
	UINT16 recSize = (UINT16)inout.GetSize();
	aad[3] = (UINT8)(recSize >> 8);
	aad[4] = (UINT8)(recSize & 0xFF);
	UINT64 serverSeq = UINT64SwapByteOrder(serverSeqNum++);
	Memory::Copy(aad + 5, &serverSeq, sizeof(UINT64));

	auto decodeResult = chacha20Context.Decode(inout, decodeBuffer, Span<const UCHAR>(aad));
	if (!decodeResult)
	{
		LOG_ERROR("Decoding failed, returning error");
		return Result<void, Error>::Err(decodeResult, Error::TlsCipher_DecodeFailed);
	}
	inout.SetBuffer(decodeBuffer.GetBuffer());
	auto setSizeResult = inout.SetSize(decodeBuffer.GetSize());
	if (!setSizeResult)
		return Result<void, Error>::Err(setSizeResult, Error::TlsCipher_DecodeFailed);

	return Result<void, Error>::Ok();
}
