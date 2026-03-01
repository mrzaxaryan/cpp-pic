#include "runtime/network/tls/tls_cipher.h"
#include "platform/io/logger.h"
#include "core/memory/memory.h"
#include "platform/system/random.h"
#include "runtime/crypto/sha2.h"
#include "runtime/network/tls/tls_hkdf.h"
#include "core/math/math.h"

/// @brief Reset the TlsCipher object to its initial state
/// @return void

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
/// @return void

VOID TlsCipher::Destroy()
{
	Reset();
	publicKey.Clear();
	decodeBuffer.Clear();
}

/// @brief Create client random data
/// @return Pointer to the client random data

PINT8 TlsCipher::CreateClientRand()
{
	Random random;

	LOG_DEBUG("Creating client random data for cipher: %p", this);
	for (UINT64 i = 0; i < (UINT64)sizeof(data12.clientRandom); i++)
	{
		data12.clientRandom[i] = random.Get() & 0xff;
	}
	LOG_DEBUG("Client random data created: %p", data12.clientRandom);
	return (PINT8)data12.clientRandom;
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
/// @return void

VOID TlsCipher::GetHash(Span<CHAR> out)
{
	handshakeHash.GetHash(out);
}

/// @brief Update the handshake hash with new input data
/// @param in Pointer to the input data to be added to the handshake hash
/// @param len Length of the input data
/// @return void

VOID TlsCipher::UpdateHash(Span<const CHAR> in)
{
	handshakeHash.Append(in);
}

/// @brief Compute the public key for the specified ECC index and store it in the provided output buffer
/// @param eccIndex Index of the ECC key to use for public key computation
/// @param out Pointer to the buffer where the computed public key will be stored
/// @return Result<void, Error>::Ok() if the public key was successfully computed

Result<void, Error> TlsCipher::ComputePublicKey(INT32 eccIndex, TlsBuffer &out)
{
	if (privateEccKeys[eccIndex] == nullptr)
	{
		LOG_DEBUG("Allocating memory for private ECC key at index %d", eccIndex);
		privateEccKeys[eccIndex] = new ECC();
		INT32 ecc_size_list[2];
		ecc_size_list[0] = 32;
		ecc_size_list[1] = 48;

		auto initResult = privateEccKeys[eccIndex]->Initialize(ecc_size_list[eccIndex]);
		if (!initResult)
		{
			LOG_DEBUG("Failed to initialize ECC key at index %d", eccIndex);
			return Result<void, Error>::Err(initResult, Error::TlsCipher_ComputePublicKeyFailed);
		}
	}

	out.CheckSize(MAX_PUBKEY_SIZE);

	auto exportResult = privateEccKeys[eccIndex]->ExportPublicKey(Span<UINT8>((UINT8 *)out.GetBuffer() + out.GetSize(), MAX_PUBKEY_SIZE));
	if (!exportResult)
		return Result<void, Error>::Err(exportResult, Error::TlsCipher_ComputePublicKeyFailed);
	out.SetSize(out.GetSize() + exportResult.Value());

	return Result<void, Error>::Ok();
}

/// @brief Compute the pre-master key using the specified ECC group and server key, and store it in the provided output buffer
/// @param ecc Specified ECC group to use for key computation
/// @param serverKey Server's public key to use for pre-master key computation
/// @param serverKeyLen Server key length in bytes
/// @param premasterKey Pointer to the buffer where the computed pre-master key will be stored
/// @return Result<void, Error>::Ok() if the pre-master key was successfully computed

Result<void, Error> TlsCipher::ComputePreKey(EccGroup ecc, Span<const CHAR> serverKey, TlsBuffer &premasterKey)
{
	INT32 eccIndex;
	INT32 eccSize;

	// Replace loop with two if statements
	if (ecc == EccGroup::Secp256r1)
	{
		eccSize = 32;
		eccIndex = 0;
	}
	else if (ecc == EccGroup::Secp384r1)
	{
		eccSize = 48;
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

	premasterKey.SetSize(eccSize);

	auto secretResult = privateEccKeys[eccIndex]->ComputeSharedSecret(Span<const UINT8>((UINT8 *)serverKey.Data(), serverKey.Size()), Span<UINT8>((UINT8 *)premasterKey.GetBuffer(), eccSize));
	if (!secretResult)
	{
		LOG_DEBUG("Failed to compute shared secret for ECC group %d", (UINT16)ecc);
		return Result<void, Error>::Err(secretResult, Error::TlsCipher_ComputePreKeyFailed);
	}

	return Result<void, Error>::Ok();
}

/// @brief Compute the TLS key using the specified ECC group and server key, and store it in the provided finished hash
/// @param ecc Specified ECC group to use for key computation
/// @param serverKey Server's public key to use for TLS key computation
/// @param serverKeyLen Server key length in bytes
/// @param finishedHash Pointer to the buffer where the computed finished hash will be stored
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
	auto server_key_app = "s ap traffic"_embed;
	auto server_key_hs = "s hs traffic"_embed;
	auto client_key_app = "c ap traffic"_embed;
	auto client_key_hs = "c hs traffic"_embed;
	const CHAR *server_key = ecc == EccGroup::None ? (const CHAR *)server_key_app : (const CHAR *)server_key_hs;
	const CHAR *client_key = ecc == EccGroup::None ? (const CHAR *)client_key_app : (const CHAR *)client_key_hs;
	TlsHash hash2;
	hash2.GetHash(Span<CHAR>((CHAR *)hash, hashLen));
	Memory::Zero(earlysecret, sizeof(earlysecret));

	if (ecc == EccGroup::None)
	{
		LOG_DEBUG("Using EccGroup::None for TLS key computation");

		TlsHkdf::ExpandLabel(Span<UCHAR>(salt, hashLen), Span<const UCHAR>((UINT8 *)data13.pseudoRandomKey, hashLen), Span<const CHAR>("derived"_embed, 7), Span<const UCHAR>(hash, hashLen));
		TlsHkdf::Extract(Span<UCHAR>(data13.pseudoRandomKey, hashLen), Span<const UCHAR>(salt, hashLen), Span<const UCHAR>(earlysecret, hashLen));

		if (finishedHash.Data())
		{
			LOG_DEBUG("Using finished hash for TLS key computation with size: %d bytes", (INT32)finishedHash.Size());
			Memory::Copy(hash, (VOID *)finishedHash.Data(), hashLen);
		}
	}
	else
	{
		TlsBuffer premaster_key;
		auto preKeyResult = ComputePreKey(ecc, serverKey, premaster_key);
		if (!preKeyResult)
		{
			LOG_DEBUG("Failed to compute pre-master key for ECC group %d", (UINT16)ecc);
			return Result<void, Error>::Err(preKeyResult, Error::TlsCipher_ComputeKeyFailed);
		}
		LOG_DEBUG("Computed pre-master key for ECC group %d, size: %d bytes", ecc, premaster_key.GetSize());

		// RFC 8446 §7.1: the initial Extract uses a salt of HashLen zero bytes
		UCHAR zeroSalt[MAX_HASH_LEN];
		Memory::Zero(zeroSalt, hashLen);

		TlsHkdf::Extract(Span<UCHAR>(data13.pseudoRandomKey, hashLen), Span<const UCHAR>(zeroSalt, hashLen), Span<const UCHAR>(earlysecret, hashLen));
		TlsHkdf::ExpandLabel(Span<UCHAR>(salt, hashLen), Span<const UCHAR>(data13.pseudoRandomKey, hashLen), Span<const CHAR>("derived"_embed, 7), Span<const UCHAR>(hash, hashLen));
		TlsHkdf::Extract(Span<UCHAR>(data13.pseudoRandomKey, hashLen), Span<const UCHAR>(salt, hashLen), Span<const UCHAR>((UINT8 *)premaster_key.GetBuffer(), premaster_key.GetSize()));

		GetHash(Span<CHAR>((CHAR *)hash, CIPHER_HASH_SIZE));
	}

	TlsHkdf::ExpandLabel(Span<UCHAR>(data13.handshakeSecret, hashLen), Span<const UCHAR>(data13.pseudoRandomKey, hashLen), Span<const CHAR>(client_key, 12), Span<const UCHAR>(hash, hashLen));

	TlsHkdf::ExpandLabel(Span<UCHAR>(localKeyBuffer, keyLen), Span<const UCHAR>(data13.handshakeSecret, hashLen), Span<const CHAR>("key"_embed, 3), Span<const UCHAR>());
	TlsHkdf::ExpandLabel(Span<UCHAR>(localIvBuffer, chacha20Context.GetIvLength()), Span<const UCHAR>(data13.handshakeSecret, hashLen), Span<const CHAR>("iv"_embed, 2), Span<const UCHAR>());

	TlsHkdf::ExpandLabel(Span<UCHAR>(data13.mainSecret, hashLen), Span<const UCHAR>(data13.pseudoRandomKey, hashLen), Span<const CHAR>(server_key, 12), Span<const UCHAR>(hash, hashLen));

	TlsHkdf::ExpandLabel(Span<UCHAR>(remoteKeyBuffer, keyLen), Span<const UCHAR>(data13.mainSecret, hashLen), Span<const CHAR>("key"_embed, 3), Span<const UCHAR>());
	TlsHkdf::ExpandLabel(Span<UCHAR>(remoteIvBuffer, chacha20Context.GetIvLength()), Span<const UCHAR>(data13.mainSecret, hashLen), Span<const CHAR>("iv"_embed, 2), Span<const UCHAR>());

	auto initResult = chacha20Context.Initialize(Span<const UINT8, POLY1305_KEYLEN>(localKeyBuffer), Span<const UINT8, POLY1305_KEYLEN>(remoteKeyBuffer), localIvBuffer, remoteIvBuffer);
	if (!initResult)
	{
		LOG_DEBUG("Failed to initialize encoder with local key: %p, remote key: %p", localKeyBuffer, remoteKeyBuffer);
		return Result<void, Error>::Err(initResult, Error::TlsCipher_ComputeKeyFailed);
	}

	LOG_DEBUG("Encoder initialized with local key: %p, remote key: %p", localKeyBuffer, remoteKeyBuffer);
	return Result<void, Error>::Ok();
}

/// @brief Compute the verify data for the TLS handshake and store it in the provided output buffer
/// @param out Pointer to the buffer where the computed verify data will be stored
/// @param verifySize Size of the verify data to compute
/// @param localOrRemote Indicates whether to use the local or remote finished key
/// @return void
/// @see RFC 8446 Section 4.4.4 — Finished
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4.4.4

VOID TlsCipher::ComputeVerify(TlsBuffer &out, INT32 verifySize, INT32 localOrRemote)
{
	if (cipherIndex == -1)
	{
		LOG_DEBUG("tls_cipher_compute_verify: cipher_index is -1, cannot compute verify data");
		return;
	}
	CHAR hash[MAX_HASH_LEN];
	INT32 hashLen = CIPHER_HASH_SIZE;
	LOG_DEBUG("tls_cipher_compute_verify: Getting handshake hash, hash_len=%d", hashLen);
	GetHash(Span<CHAR>(hash, hashLen));

	UINT8 finished_key[MAX_HASH_LEN];
	auto finishedLabel = "finished"_embed;
	if (localOrRemote)
	{
		LOG_DEBUG("tls_cipher_compute_verify: Using server finished key");
		TlsHkdf::ExpandLabel(Span<UCHAR>(finished_key, hashLen), Span<const UCHAR>(data13.mainSecret, hashLen), Span<const CHAR>(finishedLabel, 8), Span<const UCHAR>());
	}
	else
	{
		LOG_DEBUG("tls_cipher_compute_verify: Using client finished key");
		TlsHkdf::ExpandLabel(Span<UCHAR>(finished_key, hashLen), Span<const UCHAR>(data13.handshakeSecret, hashLen), Span<const CHAR>(finishedLabel, 8), Span<const UCHAR>());
	}
	out.SetSize(verifySize);
	LOG_DEBUG("tls_cipher_compute_verify: Calculating HMAC for verify, verify_size=%d", verifySize);
	HMAC_SHA256 hmac;
	hmac.Init(Span<const UCHAR>(finished_key, hashLen));
	hmac.Update(Span<const UCHAR>((UINT8 *)hash, hashLen));

	hmac.Final(Span<UCHAR>((UINT8 *)out.GetBuffer(), out.GetSize()));
	LOG_DEBUG("tls_cipher_compute_verify: Finished verify computation");
}

/// @brief Encode a TLS record using the ChaCha20 encoder and append it to the send buffer
/// @param sendbuf Pointer to the buffer where the encoded TLS record will be appended
/// @param packet Pointer to the TLS record to encode
/// @param packetSize Size of the TLS record to encode
/// @param keepOriginal Indicates whether to keep the original TLS record without encoding
/// @return void

VOID TlsCipher::Encode(TlsBuffer &sendbuf, Span<const CHAR> packet, BOOL keepOriginal)
{
	if (!isEncoding || !chacha20Context.IsValid() || keepOriginal)
	{
		LOG_DEBUG("Encoding not enabled or encoder is nullptr, appending packet directly to sendbuf");
		sendbuf.Append(packet);
		return;
	}
	LOG_DEBUG("Encoding packet with size: %d bytes", (INT32)packet.Size());

	UCHAR aad[13];

	aad[0] = CONTENT_APPLICATION_DATA;
	aad[1] = sendbuf.GetBuffer()[1];
	aad[2] = sendbuf.GetBuffer()[2];
	*((UINT16 *)(aad + 3)) = UINT16SwapByteOrder(ChaCha20Encoder::ComputeSize((INT32)packet.Size(), CipherDirection::Encode)); //-header_size
	UINT64 clientSeq = UINT64SwapByteOrder(clientSeqNum++);
	Memory::Copy(aad + 5, &clientSeq, sizeof(UINT64));

	chacha20Context.Encode(sendbuf, packet, Span<const UCHAR>(aad));
}

/// @brief Decode a TLS record using the ChaCha20 encoder and store the result in the provided buffer
/// @param inout Pointer to the buffer containing the TLS record to decode, and also where the decoded data will be stored
/// @param version TLS version of the record to decode
/// @return Result<void, Error>::Ok() if the TLS record was successfully decoded

Result<void, Error> TlsCipher::Decode(TlsBuffer &inout, INT32 version)
{
	if (!isEncoding || !chacha20Context.IsValid())
	{
		LOG_DEBUG("Encoding not enabled or encoder is nullptr, cannot Decode packet");
		return Result<void, Error>::Ok();
	}
	UCHAR aad[13];

	aad[0] = CONTENT_APPLICATION_DATA;
	aad[1] = UINT16SwapByteOrder(version) >> 8;
	aad[2] = UINT16SwapByteOrder(version) & 0xff;
	*((UINT16 *)(aad + 3)) = UINT16SwapByteOrder(inout.GetSize()); //-header_size
	UINT64 serverSeq = UINT64SwapByteOrder(serverSeqNum++);
	Memory::Copy(aad + 5, &serverSeq, sizeof(UINT64));

	auto decodeResult = chacha20Context.Decode(inout, decodeBuffer, Span<const UCHAR>(aad));
	if (!decodeResult)
	{
		LOG_ERROR("Decoding failed, returning error");
		return Result<void, Error>::Err(decodeResult, Error::TlsCipher_DecodeFailed);
	}
	inout.SetBuffer(decodeBuffer.GetBuffer());
	inout.SetSize(decodeBuffer.GetSize());

	return Result<void, Error>::Ok();
}