#include "runtime/network/tls/tls.h"
#include "core/io/binary_reader.h"
#include "core/memory/memory.h"
#include "core/string/string.h"
#include "platform/io/logger.h"
#include "core/math/math.h"

/// TLS_CHACHA20_POLY1305_SHA256 cipher suite identifier (RFC 8446 Section B.4)
constexpr UINT16 TLS_CHACHA20_POLY1305_SHA256 = 0x1303;

/// Number of expected handshake states in TLS 1.3 (ServerHello through Finished)
constexpr INT32 HANDSHAKE_STATE_COUNT = 6;

/// Receive buffer size for incoming TLS records (16 KiB)
constexpr INT32 RECV_BUFFER_SIZE = 4096 * 4;
/// Maximum TLS record payload size for outgoing data (16 KiB, RFC 8446 Section 5.1)
constexpr INT32 MAX_TLS_SEND_CHUNK = 1024 * 16;
/// TLS 1.2 protocol version (RFC 5246 Section 1.2)
constexpr UINT16 TLS_VERSION_1_2 = 0x0303;
/// TLS 1.3 protocol version (RFC 8446 Section 4.2.1)
constexpr UINT16 TLS_VERSION_1_3 = 0x0304;

/// ChangeCipherSpec content type (RFC 5246 Section 6.2.1 — legacy, used in TLS 1.3 middlebox compat)
constexpr UINT8 CONTENT_CHANGECIPHERSPEC = 0x14;
/// Alert content type (RFC 8446 Section 5.1)
constexpr UINT8 CONTENT_ALERT = 0x15;
/// Handshake content type (RFC 8446 Section 5.1)
constexpr UINT8 CONTENT_HANDSHAKE = 0x16;

/// ClientHello handshake message type (RFC 8446 Section 4.1.2)
constexpr UINT8 MSG_CLIENT_HELLO = 0x01;
/// ServerHello handshake message type (RFC 8446 Section 4.1.3)
constexpr UINT8 MSG_SERVER_HELLO = 0x02;
/// EncryptedExtensions handshake message type (RFC 8446 Section 4.3.1)
constexpr UINT8 MSG_ENCRYPTED_EXTENSIONS = 0x08;
/// Certificate handshake message type (RFC 8446 Section 4.4.2)
constexpr UINT8 MSG_CERTIFICATE = 0x0B;
/// ServerHelloDone handshake message type (legacy TLS 1.2, RFC 5246 Section 7.4.5)
constexpr UINT8 MSG_SERVER_HELLO_DONE = 0x0E;
/// CertificateVerify handshake message type (RFC 8446 Section 4.4.3)
constexpr UINT8 MSG_CERTIFICATE_VERIFY = 0x0F;
/// ClientKeyExchange handshake message type (legacy TLS 1.2, RFC 5246 Section 7.4.7)
constexpr UINT8 MSG_CLIENT_KEY_EXCHANGE = 0x10;
/// Finished handshake message type (RFC 8446 Section 4.4.4)
constexpr UINT8 MSG_FINISHED = 0x14;

/// ChangeCipherSpec message value (RFC 5246 Section 7.1 — legacy compatibility)
constexpr UINT8 MSG_CHANGE_CIPHER_SPEC = 0x01;

/// @brief TLS 1.3 signature algorithm identifiers for the signature_algorithms extension
/// @see RFC 8446 Section 4.2.3 — Signature Algorithms
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4.2.3
constexpr UINT16 SIG_ECDSA_SECP256R1_SHA256 = 0x0403; ///< ecdsa_secp256r1_sha256
constexpr UINT16 SIG_ECDSA_SECP384R1_SHA384 = 0x0503; ///< ecdsa_secp384r1_sha384
constexpr UINT16 SIG_ECDSA_SECP521R1_SHA512 = 0x0603; ///< ecdsa_secp521r1_sha512
constexpr UINT16 SIG_RSA_PSS_RSAE_SHA256    = 0x0804; ///< rsa_pss_rsae_sha256
constexpr UINT16 SIG_RSA_PSS_RSAE_SHA384    = 0x0805; ///< rsa_pss_rsae_sha384
constexpr UINT16 SIG_RSA_PSS_RSAE_SHA512    = 0x0806; ///< rsa_pss_rsae_sha512
constexpr UINT16 SIG_RSA_PKCS1_SHA256       = 0x0401; ///< rsa_pkcs1_sha256
constexpr UINT16 SIG_RSA_PKCS1_SHA384       = 0x0501; ///< rsa_pkcs1_sha384
constexpr UINT16 SIG_RSA_PKCS1_SHA512       = 0x0601; ///< rsa_pkcs1_sha512
constexpr UINT16 SIG_ECDSA_SHA1             = 0x0203; ///< ecdsa_sha1 (legacy)
constexpr UINT16 SIG_RSA_PKCS1_SHA1         = 0x0201; ///< rsa_pkcs1_sha1 (legacy)

/// Number of signature algorithms advertised in ClientHello
constexpr UINT16 SIG_ALGORITHM_COUNT = 11;
/// Total bytes for signature_algorithms extension data (count * 2)
constexpr UINT16 SIG_ALGORITHM_LIST_BYTES = SIG_ALGORITHM_COUNT * 2;

/// @brief TLS extension type identifiers
/// @see https://www.iana.org/assignments/tls-extensiontype-values/tls-extensiontype-values.xhtml
/// @see RFC 8446 Section 4.2 — Extensions
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4.2
enum class TlsExtension : UINT16
{
	ServerName = 0x0000,            ///< server_name (RFC 6066 Section 3)
	SupportedGroups = 0x000A,       ///< supported_groups (RFC 8422 Section 5.1.1)
	EcPointFormats = 0x000B,        ///< ec_point_formats (RFC 8422 Section 5.1.2)
	SignatureAlgorithms = 0x000D,   ///< signature_algorithms (RFC 8446 Section 4.2.3)
	EncryptThenMac = 0x0016,        ///< encrypt_then_mac (RFC 7366)
	ExtendedMasterSecret = 0x0017,  ///< extended_master_secret (RFC 7627)
	RecordSizeLimit = 0x001C,       ///< record_size_limit (RFC 8449)
	SessionTicketTls = 0x0023,      ///< session_ticket (RFC 5077)
	PresharedKey = 0x0029,          ///< pre_shared_key (RFC 8446 Section 4.2.11)
	SupportedVersion = 0x002B,      ///< supported_versions (RFC 8446 Section 4.2.1)
	PskKeyExchangeModes = 0x002D,   ///< psk_key_exchange_modes (RFC 8446 Section 4.2.9)
	KeyShare = 0x0033,              ///< key_share (RFC 8446 Section 4.2.8)
	RenegotiationInfo = 0xFF01,     ///< renegotiation_info (RFC 5746)
	Last = 0x7FFF                   ///< Sentinel value
};

static FORCE_INLINE VOID AppendU16BE(TlsBuffer &buf, UINT16 val)
{
	buf.Append<INT16>(UINT16SwapByteOrder(val));
}

/// @brief Send packet data over TLS connection
/// @param packetType Type of the TLS packet (e.g., handshake, application data)
/// @param ver Version of TLS to use for the packet
/// @param buf The buffer containing the packet data to send
/// @return Result indicating success or Tls_SendPacketFailed error
/// @see RFC 8446 Section 5.1 — Record Layer
///      https://datatracker.ietf.org/doc/html/rfc8446#section-5.1

Result<void, Error> TlsClient::SendPacket(INT32 packetType, INT32 ver, Span<const CHAR> data)
{
	if (packetType == CONTENT_HANDSHAKE && data.Size() > 0)
	{
		LOG_DEBUG("Sending handshake packet with type: %d, version: %d, size: %d bytes", packetType, ver, (INT32)data.Size());
		crypto.UpdateHash(data);
	}
	LOG_DEBUG("Sending packet with type: %d, version: %d, size: %d bytes", packetType, ver, (INT32)data.Size());

	TlsBuffer tempBuffer;
	tempBuffer.Append<CHAR>(packetType);
	tempBuffer.Append<INT16>(ver);
	INT32 bodySizeIndex = tempBuffer.AppendSize(2); // tls body size

	BOOL keepOriginal = packetType == CONTENT_CHANGECIPHERSPEC || packetType == CONTENT_ALERT;
	INT32 innerContentType = -1;
	if (!keepOriginal && crypto.GetEncoding())
	{
		LOG_DEBUG("Encoding packet with type: %d, size: %d bytes", packetType, (INT32)data.Size());
		(tempBuffer.GetBuffer())[0] = CONTENT_APPLICATION_DATA;
		innerContentType = packetType;
	}
	LOG_DEBUG("Encoding buffer with size: %d bytes, keepOriginal: %d", (INT32)data.Size(), keepOriginal);
	crypto.Encode(tempBuffer, data, keepOriginal, innerContentType);

	tempBuffer.PatchU16BE(bodySizeIndex, tempBuffer.GetSize() - bodySizeIndex - 2);
	auto writeResult = context.Write(tempBuffer.AsSpan());
	if (!writeResult)
	{
		LOG_DEBUG("Failed to write packet to socket");
		return Result<void, Error>::Err(writeResult, Error::Tls_SendPacketFailed);
	}
	LOG_DEBUG("Packet sent successfully, bytesWritten: %d", writeResult.Value());
	return Result<void, Error>::Ok();
}

/// @brief Send a ClientHello message to initiate the TLS handshake with the server
/// @param host The hostname of the server to connect to
/// @return Result indicating success or Tls_ClientHelloFailed error
/// @see RFC 8446 Section 4.1.2 — Client Hello
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4.1.2

Result<void, Error> TlsClient::SendClientHello(PCCHAR host)
{
	LOG_DEBUG("Sending ClientHello for client: %p, host: %s", this, host);

	TlsBuffer msg;

	BOOL hasTls13 = false;

	msg.Append<CHAR>(MSG_CLIENT_HELLO);
	INT32 handshakeSizeIndex = msg.AppendSize(3); // tls handshake body size (24-bit)
	LOG_DEBUG("Appending ClientHello with handshake size index: %d", handshakeSizeIndex);

	msg.Append<INT16>(TLS_VERSION_1_2);
	LOG_DEBUG("Appending ClientHello with version: 0x0303");
	auto clientRand = crypto.CreateClientRand();
	msg.Append(Span<const CHAR>((PCCHAR)clientRand.Data(), clientRand.Size()));
	LOG_DEBUG("Appending ClientHello with client random data");
	msg.Append<CHAR>(0);
	LOG_DEBUG("Client has %d ciphers to append", crypto.GetCipherCount());
	INT32 cipherCountIndex = msg.AppendSize(2);
	LOG_DEBUG("Appending ClientHello with cipher count index: %d", cipherCountIndex);
	for (INT32 i = 0; i < crypto.GetCipherCount(); i++)
	{
		AppendU16BE(msg, (UINT16)TLS_CHACHA20_POLY1305_SHA256);
		hasTls13 = true;
	}
	LOG_DEBUG("Appending ClientHello with %d ciphers", crypto.GetCipherCount());
	msg.PatchU16BE(cipherCountIndex, msg.GetSize() - cipherCountIndex - 2);
	msg.Append<CHAR>(1);
	msg.Append<CHAR>(0);

	INT32 extSizeIndex = msg.AppendSize(2);
	LOG_DEBUG("Appending ClientHello with extension size index: %d", extSizeIndex);

	// server_name extension (RFC 6066 Section 3):
	// ServerNameList: 2 bytes list length + ServerName: 1 byte type + 2 bytes name length + name
	AppendU16BE(msg, (UINT16)TlsExtension::ServerName);
	INT32 hostLen = (INT32)StringUtils::Length(host);
	LOG_DEBUG("Appending ClientHello with host: %s, length: %d", host, hostLen);
	AppendU16BE(msg, hostLen + 5); // ext data length: list_len(2) + type(1) + name_len(2)
	AppendU16BE(msg, hostLen + 3); // server name list length: type(1) + name_len(2)
	msg.Append<CHAR>(0);           // name type: host_name (0)
	AppendU16BE(msg, hostLen);      // host name length
	msg.Append(Span<const CHAR>(host, hostLen));

	AppendU16BE(msg, (UINT16)TlsExtension::SupportedGroups); // ext type
	AppendU16BE(msg, ECC_COUNT * 2 + 2);    // ext size
	AppendU16BE(msg, ECC_COUNT * 2);
	LOG_DEBUG("Appending ClientHello with supported groups, count: %d", ECC_COUNT);
	AppendU16BE(msg, (UINT16)EccGroup::Secp256r1);
	AppendU16BE(msg, (UINT16)EccGroup::Secp384r1);

	if (hasTls13)
	{
		LOG_DEBUG("Appending ClientHello with TLS 1.3 specific extensions");

		// supported_versions extension (RFC 8446 Section 4.2.1)
		AppendU16BE(msg, (UINT16)TlsExtension::SupportedVersion);
		AppendU16BE(msg, 3);    // ext data length: list_len(1) + version(2)
		msg.Append<CHAR>(2);    // version list length: 2 bytes
		AppendU16BE(msg, TLS_VERSION_1_3);

		// signature_algorithms extension (RFC 8446 Section 4.2.3)
		AppendU16BE(msg, (UINT16)TlsExtension::SignatureAlgorithms);
		AppendU16BE(msg, SIG_ALGORITHM_LIST_BYTES + 2); // ext data length
		AppendU16BE(msg, SIG_ALGORITHM_LIST_BYTES);     // algorithm list length
		AppendU16BE(msg, SIG_ECDSA_SECP256R1_SHA256);
		AppendU16BE(msg, SIG_ECDSA_SECP384R1_SHA384);
		AppendU16BE(msg, SIG_ECDSA_SECP521R1_SHA512);
		AppendU16BE(msg, SIG_RSA_PSS_RSAE_SHA256);
		AppendU16BE(msg, SIG_RSA_PSS_RSAE_SHA384);
		AppendU16BE(msg, SIG_RSA_PSS_RSAE_SHA512);
		AppendU16BE(msg, SIG_RSA_PKCS1_SHA256);
		AppendU16BE(msg, SIG_RSA_PKCS1_SHA384);
		AppendU16BE(msg, SIG_RSA_PKCS1_SHA512);
		AppendU16BE(msg, SIG_ECDSA_SHA1);
		AppendU16BE(msg, SIG_RSA_PKCS1_SHA1);

		AppendU16BE(msg, (UINT16)TlsExtension::KeyShare); // ext type
		INT32 shareSize = msg.AppendSize(2);
		msg.AppendSize(2);
		EccGroup eccIanaList[2]{};
		eccIanaList[0] = EccGroup::Secp256r1;
		eccIanaList[1] = EccGroup::Secp384r1;

		for (INT32 i = 0; i < ECC_COUNT; i++)
		{
			UINT16 eccIana = (UINT16)eccIanaList[i];
			AppendU16BE(msg, eccIana);
			INT32 shareSizeSub = msg.AppendSize(2);
			auto r = crypto.ComputePublicKey(i, msg);
			if (!r)
			{
				LOG_DEBUG("Failed to compute public key for ECC group %d", i);
				return Result<void, Error>::Err(r, Error::Tls_ClientHelloFailed);
			}
			LOG_DEBUG("Computed public key for ECC group %d, size: %d bytes", i, msg.GetSize() - shareSizeSub - 2);
			msg.PatchU16BE(shareSizeSub, msg.GetSize() - shareSizeSub - 2);
		}
		msg.PatchU16BE(shareSize, msg.GetSize() - shareSize - 2);
		msg.PatchU16BE(shareSize + 2, msg.GetSize() - shareSize - 4);
	}
	LOG_DEBUG("Appending ClientHello with extensions, size: %d bytes", msg.GetSize() - extSizeIndex - 2);

	msg.PatchU16BE(extSizeIndex, msg.GetSize() - extSizeIndex - 2);
	msg.PatchU24BE(handshakeSizeIndex, msg.GetSize() - handshakeSizeIndex - 3);

	auto r = SendPacket(CONTENT_HANDSHAKE, TLS_VERSION_1_2, msg.AsSpan());
	if (!r)
		return Result<void, Error>::Err(r, Error::Tls_ClientHelloFailed);
	return Result<void, Error>::Ok();
}

/// @brief Send a Client Finished message to complete the TLS handshake
/// @return Result indicating success or Tls_ClientFinishedFailed error
/// @see RFC 8446 Section 4.4.4 — Finished
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4.4.4

Result<void, Error> TlsClient::SendClientFinished()
{
	TlsBuffer verify;
	TlsBuffer msg;
	LOG_DEBUG("Sending Client Finished for client: %p", this);
	auto verifyResult = crypto.ComputeVerify(verify, CIPHER_HASH_SIZE, 0);
	if (!verifyResult)
		return Result<void, Error>::Err(verifyResult, Error::Tls_ClientFinishedFailed);
	LOG_DEBUG("Computed verify data for Client Finished, size: %d bytes", verify.GetSize());
	msg.Append<CHAR>(MSG_FINISHED);
	msg.Append<CHAR>(0);
	msg.Append<INT16>(UINT16SwapByteOrder(verify.GetSize()));
	msg.Append(verify.AsSpan());

	auto r = SendPacket(CONTENT_HANDSHAKE, TLS_VERSION_1_2, msg.AsSpan());
	if (!r)
		return Result<void, Error>::Err(r, Error::Tls_ClientFinishedFailed);
	return Result<void, Error>::Ok();
}

/// @brief Send a Client Key Exchange message to the server during the TLS handshake
/// @return Result indicating success or Tls_ClientExchangeFailed error
/// @see RFC 5246 Section 7.4.7 — Client Key Exchange Message
///      https://datatracker.ietf.org/doc/html/rfc5246#section-7.4.7

Result<void, Error> TlsClient::SendClientExchange()
{
	TlsBuffer msg;
	TlsBuffer &pubkey = crypto.GetPubKey();
	LOG_DEBUG("Sending Client Key Exchange for client: %p, public key size: %d bytes", this, pubkey.GetSize());
	msg.Append<CHAR>(MSG_CLIENT_KEY_EXCHANGE);
	msg.Append<CHAR>(0);
	msg.Append<INT16>(UINT16SwapByteOrder(pubkey.GetSize() + 1));
	msg.Append<CHAR>((pubkey.GetSize())); // tls body size
	msg.Append(pubkey.AsSpan());
	auto r = SendPacket(CONTENT_HANDSHAKE, TLS_VERSION_1_2, msg.AsSpan());
	if (!r)
		return Result<void, Error>::Err(r, Error::Tls_ClientExchangeFailed);
	return Result<void, Error>::Ok();
}

/// @brief Send a Change Cipher Spec message to indicate subsequent messages will be encrypted
/// @return Result indicating success or Tls_ChangeCipherSpecFailed error
/// @see RFC 5246 Section 7.1 — Change Cipher Spec Protocol
///      https://datatracker.ietf.org/doc/html/rfc5246#section-7.1

Result<void, Error> TlsClient::SendChangeCipherSpec()
{
	CHAR ccs = 1;
	auto r = SendPacket(CONTENT_CHANGECIPHERSPEC, TLS_VERSION_1_2, Span<const CHAR>(&ccs, 1));
	if (!r)
		return Result<void, Error>::Err(r, Error::Tls_ChangeCipherSpecFailed);
	return Result<void, Error>::Ok();
}

/// @brief Process the ServerHello message from the server and advance the TLS handshake state
/// @param reader Buffer containing the ServerHello message data
/// @return Result indicating success or Tls_ServerHelloFailed error
/// @see RFC 8446 Section 4.1.3 — Server Hello
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4.1.3

Result<void, Error> TlsClient::OnServerHello(TlsBuffer &reader)
{
	LOG_DEBUG("Processing ServerHello for client: %p", this);
	reader.ReadU24BE();                        // handshake body size (already bounded by TLS record)
	(void)reader.Read<INT16>(); // version
	reader.AdvanceReadPosition(RAND_SIZE);     // skip server random (not used by TLS 1.3 key schedule)
	INT32 sessionLen = reader.Read<INT8>();
	LOG_DEBUG("ServerHello session length: %d", sessionLen);
	reader.AdvanceReadPosition(sessionLen);
	(void)reader.Read<INT16>(); // cur_cipher
	(void)reader.Read<INT8>();
	auto ret = crypto.UpdateServerInfo();
	if (!ret)
	{
		LOG_DEBUG("Failed to update server info for client: %p", this);
		return Result<void, Error>::Err(ret, Error::Tls_ServerHelloFailed);
	}

	if (reader.GetReadPosition() >= reader.GetSize())
	{
		LOG_DEBUG("ServerHello reader has reached the end of buffer, no extensions found");
		return Result<void, Error>::Ok();
	}
	LOG_DEBUG("ServerHello has extensions, processing them");

	INT32 extSize = UINT16SwapByteOrder(reader.Read<INT16>());
	INT32 extStart = reader.GetReadPosition();
	INT32 tlsVer = 0;
	LOG_DEBUG("ServerHello extensions size: %d bytes, start index: %d", extSize, extStart);
	TlsBuffer pubkey;
	EccGroup eccgroup = EccGroup::None;
	while (reader.GetReadPosition() < extStart + extSize)
	{
		TlsExtension type = (TlsExtension)UINT16SwapByteOrder(reader.Read<INT16>());
		if (type == TlsExtension::SupportedVersion)
		{
			LOG_DEBUG("Processing TlsExtension::SupportedVersion extension");
			reader.Read<INT16>();
			tlsVer = UINT16SwapByteOrder(reader.Read<INT16>());
		}
		else if (type == TlsExtension::KeyShare)
		{
			LOG_DEBUG("Processing TlsExtension::KeyShare extension");
			INT32 keyShareLen = UINT16SwapByteOrder(reader.Read<INT16>());
			eccgroup = (EccGroup)UINT16SwapByteOrder(reader.Read<INT16>());
			if (keyShareLen > 4)
			{
				LOG_DEBUG("Reading public key from TlsExtension::KeyShare, size: %d bytes", keyShareLen);
				auto setSizeResult = pubkey.SetSize(UINT16SwapByteOrder(reader.Read<INT16>()));
				if (!setSizeResult)
					return Result<void, Error>::Err(setSizeResult, Error::Tls_ServerHelloFailed);
				reader.Read(Span<CHAR>(pubkey.GetBuffer(), pubkey.GetSize()));
			}
			LOG_DEBUG("TlsExtension::KeyShare processed, ECC group: %d, public key size: %d bytes", (UINT16)eccgroup, pubkey.GetSize());
		}
		else
		{
			// Skip unknown extensions
			INT32 extLen = UINT16SwapByteOrder(reader.Read<INT16>());
			reader.AdvanceReadPosition(extLen);
		}
	}
	if (tlsVer != 0)
	{
		LOG_DEBUG("TLS version from ServerHello: %d", tlsVer);
		if (tlsVer != TLS_VERSION_1_3 || pubkey.GetSize() <= 0 || eccgroup == EccGroup::None)
		{
			LOG_DEBUG("Invalid TLS version or public key size, tlsVer: %d, pubkey.size: %d, eccgroup: %d", tlsVer, pubkey.GetSize(), eccgroup);
			return Result<void, Error>::Err(Error::Tls_ServerHelloFailed);
		}

		LOG_DEBUG("Valid TLS version and public key size, tlsVer: %d, pubkey.size: %d, eccgroup: %d", tlsVer, pubkey.GetSize(), eccgroup);

		auto r = crypto.ComputeKey(eccgroup, pubkey.AsSpan(), Span<CHAR>());
		if (!r)
		{
			LOG_DEBUG("Failed to compute TLS 1.3 key for client: %p, ECC group: %d, public key size: %d", this, eccgroup, pubkey.GetSize());
			return Result<void, Error>::Err(r, Error::Tls_ServerHelloFailed);
		}
		LOG_DEBUG("Computed TLS 1.3 key for client: %p, ECC group: %d, public key size: %d", this, eccgroup, pubkey.GetSize());
		crypto.SetEncoding(true);
	}
	LOG_DEBUG("ServerHello processed successfully for client: %p, ECC group: %d, public key size: %d", this, eccgroup, pubkey.GetSize());
	return Result<void, Error>::Ok();
}

/// @brief Process the ServerHelloDone message from the server and advance the TLS handshake state
/// @return Result indicating success or Tls_ServerHelloDoneFailed error
/// @see RFC 5246 Section 7.4.5 — Server Hello Done
///      https://datatracker.ietf.org/doc/html/rfc5246#section-7.4.5

Result<void, Error> TlsClient::OnServerHelloDone()
{
	auto r = SendClientExchange();
	if (!r)
	{
		LOG_DEBUG("Failed to send Client Key Exchange for client: %p", this);
		return Result<void, Error>::Err(r, Error::Tls_ServerHelloDoneFailed);
	}
	LOG_DEBUG("Client Key Exchange sent successfully for client: %p", this);
	r = SendChangeCipherSpec();
	if (!r)
	{
		LOG_DEBUG("Failed to send Change Cipher Spec for client: %p", this);
		return Result<void, Error>::Err(r, Error::Tls_ServerHelloDoneFailed);
	}
	LOG_DEBUG("Change Cipher Spec sent successfully for client: %p", this);
	crypto.SetEncoding(true);
	r = SendClientFinished();
	if (!r)
	{
		LOG_DEBUG("Failed to send Client Finished for client: %p", this);
		return Result<void, Error>::Err(r, Error::Tls_ServerHelloDoneFailed);
	}
	LOG_DEBUG("Client Finished sent successfully for client: %p", this);

	return Result<void, Error>::Ok();
}

/// @brief Verify the Finished message from the server by comparing the verify data with the expected value computed from the handshake messages
/// @param reader Buffer containing the Finished message data from the server
/// @return Result indicating success or Tls_VerifyFinishedFailed error
/// @see RFC 8446 Section 4.4.4 — Finished
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4.4.4

Result<void, Error> TlsClient::VerifyFinished(TlsBuffer &reader)
{
	INT32 serverFinishedSize = reader.ReadU24BE();
	if (serverFinishedSize < 0 || serverFinishedSize > reader.GetSize() - reader.GetReadPosition())
		return Result<void, Error>::Err(Error::Tls_VerifyFinishedFailed);
	LOG_DEBUG("Verifying Finished for client: %p, size: %d bytes", this, serverFinishedSize);
	TlsBuffer verify;
	auto verifyResult = crypto.ComputeVerify(verify, serverFinishedSize, 1);
	if (!verifyResult)
		return Result<void, Error>::Err(Error::Tls_VerifyFinishedFailed);
	LOG_DEBUG("Computed verify data for Finished, size: %d bytes", verify.GetSize());

	if (Memory::Compare(verify.GetBuffer(), reader.GetBuffer() + reader.GetReadPosition(), serverFinishedSize) != 0)
	{
		LOG_DEBUG("Finished verification failed for client: %p, expected size: %d, actual size: %d", this, verify.GetSize(), serverFinishedSize);
		return Result<void, Error>::Err(Error::Tls_VerifyFinishedFailed);
	}
	LOG_DEBUG("Finished verification succeeded for client: %p", this);
	return Result<void, Error>::Ok();
}

/// @brief Finished message from the server has been received, process it and advance the TLS handshake state to complete the handshake
/// @return Result indicating success or Tls_ServerFinishedFailed error
/// @see RFC 8446 Section 4.4.4 — Finished
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4.4.4

Result<void, Error> TlsClient::OnServerFinished()
{
	LOG_DEBUG("Processing Server Finished for client: %p", this);
	CHAR finishedHash[MAX_HASH_LEN] = {0};
	crypto.GetHash(Span<CHAR>(finishedHash, CIPHER_HASH_SIZE));
	auto ret = SendChangeCipherSpec();

	if (!ret)
	{
		LOG_DEBUG("Failed to send Change Cipher Spec for client: %p", this);
		return Result<void, Error>::Err(ret, Error::Tls_ServerFinishedFailed);
	}
	LOG_DEBUG("Change Cipher Spec sent successfully for client: %p", this);
	auto r = SendClientFinished();
	if (!r)
	{
		LOG_DEBUG("Failed to send Client Finished for client: %p", this);
		return Result<void, Error>::Err(r, Error::Tls_ServerFinishedFailed);
	}
	LOG_DEBUG("Client Finished sent successfully for client: %p", this);
	crypto.ResetSequenceNumber();
	auto r2 = crypto.ComputeKey(EccGroup::None, Span<const CHAR>(), Span<CHAR>(finishedHash, CIPHER_HASH_SIZE));
	if (!r2)
	{
		LOG_DEBUG("Failed to compute TLS 1.3 key for client: %p", this);
		return Result<void, Error>::Err(r2, Error::Tls_ServerFinishedFailed);
	}

	LOG_DEBUG("Server Finished processed successfully for client: %p", this);
	return Result<void, Error>::Ok();
}

/// @brief Handle a TLS handshake message by dispatching to the appropriate handler
/// @param handshakeType Handshake message type (e.g., MSG_SERVER_HELLO, MSG_FINISHED)
/// @param reader Buffer positioned after the handshake type byte
/// @return Result indicating success or Tls_OnPacketFailed error
/// @see RFC 8446 Section 4 — Handshake Protocol
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4

Result<void, Error> TlsClient::HandleHandshakeMessage(INT32 handshakeType, TlsBuffer &reader)
{
	if (handshakeType == MSG_SERVER_HELLO)
	{
		LOG_DEBUG("Processing ServerHello for client: %p", this);
		auto r = OnServerHello(reader);
		if (!r)
		{
			LOG_DEBUG("Failed to process handshake packet for client: %p, handshake type: %d", this, handshakeType);
			(void)Close();
			return Result<void, Error>::Err(r, Error::Tls_OnPacketFailed);
		}
	}
	else if (handshakeType == MSG_CERTIFICATE)
	{
		LOG_DEBUG("Processing Server Certificate for client: %p", this);
	}
	else if (handshakeType == MSG_CERTIFICATE_VERIFY)
	{
		LOG_DEBUG("Processing Server Certificate Verify for client: %p", this);
	}
	else if (handshakeType == MSG_SERVER_HELLO_DONE)
	{
		LOG_DEBUG("Processing Server Hello Done for client: %p", this);
		auto r = OnServerHelloDone();
		if (!r)
		{
			LOG_DEBUG("Failed to process Server Hello Done for client: %p", this);
			return Result<void, Error>::Err(r, Error::Tls_OnPacketFailed);
		}
	}
	else if (handshakeType == MSG_FINISHED)
	{
		LOG_DEBUG("Processing Server Finished for client: %p", this);
		auto r = VerifyFinished(reader);
		if (!r)
		{
			LOG_DEBUG("Failed to verify Finished for client: %p", this);
			return Result<void, Error>::Err(r, Error::Tls_OnPacketFailed);
		}
		LOG_DEBUG("Server Finished verified successfully for client: %p", this);
		crypto.UpdateHash(reader.AsSpan());
		auto r2 = OnServerFinished();
		if (!r2)
		{
			LOG_DEBUG("Failed to process Server Finished for client: %p", this);
			return Result<void, Error>::Err(r2, Error::Tls_OnPacketFailed);
		}
		LOG_DEBUG("Server Finished processed successfully for client: %p", this);
	}
	return Result<void, Error>::Ok();
}

/// @brief Handle a TLS alert message
/// @param reader Buffer containing the alert data
/// @return Result indicating success or Tls_OnPacketFailed error
/// @see RFC 8446 Section 6 — Alert Protocol
///      https://datatracker.ietf.org/doc/html/rfc8446#section-6

Result<void, Error> TlsClient::HandleAlertMessage(TlsBuffer &reader)
{
	LOG_DEBUG("Processing Alert for client: %p", this);
	if (reader.GetSize() >= 2)
	{
		[[maybe_unused]] INT32 level = reader.Read<INT8>();
		[[maybe_unused]] INT32 code = reader.Read<INT8>();
		LOG_ERROR("TLS Alert received for client: %p, level: %d, code: %d", this, level, code);
		return Result<void, Error>::Err(Error::Tls_OnPacketFailed);
	}
	LOG_DEBUG("TLS Alert received for client: %p, but buffer size is less than 2 bytes", this);
	return Result<void, Error>::Ok();
}

/// @brief Handle decrypted TLS application data by appending to the channel buffer
/// @param reader Buffer containing the application data
/// @return Result indicating success
/// @see RFC 8446 Section 5.1 — Record Layer
///      https://datatracker.ietf.org/doc/html/rfc8446#section-5.1

Result<void, Error> TlsClient::HandleApplicationData(TlsBuffer &reader)
{
	LOG_DEBUG("Processing Application Data for client: %p, size: %d bytes", this, reader.GetSize());
	decryptedSize = reader.GetSize();
	decryptedPos = 0;
	return Result<void, Error>::Ok();
}

/// @brief Process incoming TLS packets from the server, handle different packet types and advance the TLS handshake state accordingly
/// @param packetType Type of the incoming TLS packet (e.g., handshake, alert)
/// @param version Version of TLS used in the packet
/// @param tlsReader Buffer containing the packet data to process
/// @return Result indicating success or Tls_OnPacketFailed error
/// @see RFC 8446 Section 5 — Record Protocol
///      https://datatracker.ietf.org/doc/html/rfc8446#section-5

Result<void, Error> TlsClient::OnPacket(INT32 packetType, INT32 version, TlsBuffer &tlsReader)
{
	if (packetType != CONTENT_CHANGECIPHERSPEC && packetType != CONTENT_ALERT)
	{
		LOG_DEBUG("Processing packet with type: %d, version: %d, size: %d bytes", packetType, version, tlsReader.GetSize());
		auto r = crypto.Decode(tlsReader, version);
		if (!r)
		{
			LOG_DEBUG("Failed to Decode packet for client: %p, type: %d, version: %d", this, packetType, version);
			return Result<void, Error>::Err(r, Error::Tls_OnPacketFailed);
		}
		LOG_DEBUG("Packet decoded successfully for client: %p, type: %d, version: %d", this, packetType, version);
		if (crypto.GetEncoding() && tlsReader.GetSize() > 0)
		{
			LOG_DEBUG("Removing last byte from buffer for client: %p, packet type: %d", this, packetType);
			packetType = tlsReader.GetBuffer()[tlsReader.GetSize() - 1];
			auto shrinkResult = tlsReader.SetSize(tlsReader.GetSize() - 1);
			if (!shrinkResult)
				return Result<void, Error>::Err(shrinkResult, Error::Tls_OnPacketFailed);
		}
		LOG_DEBUG("Packet type after processing: %d, buffer size: %d bytes", packetType, tlsReader.GetSize());
	}

	// TLS 1.3 handshake state sequence — initialized once before the segment loop
	TlsState stateSeq[HANDSHAKE_STATE_COUNT]{};
	stateSeq[0] = {CONTENT_HANDSHAKE, MSG_SERVER_HELLO};
	stateSeq[1] = {CONTENT_CHANGECIPHERSPEC, MSG_CHANGE_CIPHER_SPEC};
	stateSeq[2] = {CONTENT_HANDSHAKE, MSG_ENCRYPTED_EXTENSIONS};
	stateSeq[3] = {CONTENT_HANDSHAKE, MSG_CERTIFICATE};
	stateSeq[4] = {CONTENT_HANDSHAKE, MSG_CERTIFICATE_VERIFY};
	stateSeq[5] = {CONTENT_HANDSHAKE, MSG_FINISHED};

	while (tlsReader.GetReadPosition() < tlsReader.GetSize())
	{
		INT32 segSize;
		if (packetType == CONTENT_HANDSHAKE)
		{
			INT32 remaining = tlsReader.GetSize() - tlsReader.GetReadPosition();
			if (remaining < 4)
				return Result<void, Error>::Err(Error::Tls_OnPacketFailed);
			PUCHAR seg = (PUCHAR)(tlsReader.GetBuffer() + tlsReader.GetReadPosition());
			segSize = 4 + (((UINT32)seg[1] << 16) | ((UINT32)seg[2] << 8) | (UINT32)seg[3]);
			if (segSize > remaining)
				return Result<void, Error>::Err(Error::Tls_OnPacketFailed);
		}
		else
		{
			segSize = tlsReader.GetSize();
		}
		TlsBuffer readerSig(Span<CHAR>(tlsReader.GetBuffer() + tlsReader.GetReadPosition(), (USIZE)segSize));

		if (stateIndex < HANDSHAKE_STATE_COUNT && packetType != CONTENT_ALERT)
		{
			LOG_DEBUG("Checking state sequence for client: %p, state index: %d, packet type: %d, handshake type: %d", this, stateIndex, packetType, readerSig.GetBuffer()[0]);
			if (stateSeq[stateIndex].ContentType != packetType || stateSeq[stateIndex].HandshakeType != readerSig.GetBuffer()[0])
			{
				LOG_DEBUG("State sequence mismatch for client: %p, expected type: %d, expected handshake type: %d, actual type: %d, actual handshake type: %d",
						  this, stateSeq[stateIndex].ContentType, stateSeq[stateIndex].HandshakeType, packetType, readerSig.GetBuffer()[0]);
				return Result<void, Error>::Err(Error::Tls_OnPacketFailed);
			}
			LOG_DEBUG("State sequence matches for client: %p, state index: %d, packet type: %d, handshake type: %d", this, stateIndex, packetType, readerSig.GetBuffer()[0]);
			stateIndex++;
		}

		if (packetType == CONTENT_HANDSHAKE && readerSig.GetSize() > 0 && readerSig.GetBuffer()[0] != MSG_FINISHED)
		{
			LOG_DEBUG("Updating hash for client: %p, packet type: %d, size: %d bytes", this, packetType, readerSig.GetSize());
			crypto.UpdateHash(readerSig.AsSpan());
		}
		if (packetType == CONTENT_HANDSHAKE)
		{
			LOG_DEBUG("Processing handshake packet for client: %p, handshake type: %d", this, readerSig.GetBuffer()[0]);
			INT32 handshakeType = readerSig.Read<INT8>();
			LOG_DEBUG("Handshake type: %d", handshakeType);
			auto r = HandleHandshakeMessage(handshakeType, readerSig);
			if (!r)
				return Result<void, Error>::Err(r, Error::Tls_OnPacketFailed);
		}
		else if (packetType == CONTENT_CHANGECIPHERSPEC)
		{
		}
		else if (packetType == CONTENT_ALERT)
		{
			auto r = HandleAlertMessage(readerSig);
			if (!r)
				return Result<void, Error>::Err(r, Error::Tls_OnPacketFailed);
		}
		else if (packetType == CONTENT_APPLICATION_DATA)
		{
			auto r = HandleApplicationData(readerSig);
			if (!r)
				return Result<void, Error>::Err(r, Error::Tls_OnPacketFailed);
		}
		tlsReader.AdvanceReadPosition(segSize);
	}

	return Result<void, Error>::Ok();
}

/// @brief Packet processing — read data from the socket, parse TLS records
/// @return Result indicating success or Tls_ProcessReceiveFailed error
/// @see RFC 8446 Section 5.1 — Record Layer
///      https://datatracker.ietf.org/doc/html/rfc8446#section-5.1

Result<void, Error> TlsClient::ProcessReceive()
{
	LOG_DEBUG("Processing received data for client: %p, current state index: %d", this, stateIndex);
	auto checkResult = recvBuffer.CheckSize(RECV_BUFFER_SIZE);
	if (!checkResult)
		return Result<void, Error>::Err(checkResult, Error::Tls_ProcessReceiveFailed);
	auto readResult = context.Read(Span<CHAR>(recvBuffer.GetBuffer() + recvBuffer.GetSize(), RECV_BUFFER_SIZE));
	if (!readResult || readResult.Value() <= 0)
	{
		LOG_DEBUG("Failed to read data from socket for client: %p", this);
		(void)Close();
		return Result<void, Error>::Err(readResult, Error::Tls_ProcessReceiveFailed);
	}
	INT64 len = readResult.Value();
	if (len > 0x7FFFFFFF)
		return Result<void, Error>::Err(Error::Tls_ProcessReceiveFailed);
	LOG_DEBUG("Received %lld bytes from socket for client: %p", len, this);
	recvBuffer.AppendSize((INT32)len);

	BinaryReader reader(Span<const UINT8>((const UINT8*)recvBuffer.GetBuffer(), (USIZE)recvBuffer.GetSize()));

	while (reader.Remaining() >= 5)
	{
		USIZE headerStart = reader.GetOffset();

		UINT8 contentType = reader.Read<UINT8>();
		UINT16 version = reader.Read<UINT16>(); // native byte order — matches OnPacket signature
		UINT16 packetSize = reader.ReadU16BE();

		if (reader.Remaining() < packetSize)
		{
			reader.SetOffset(headerStart);
			break;
		}

		LOG_DEBUG("Processing packet for client: %p, current index: %d, packet size: %d", this, (INT32)headerStart, packetSize);

		TlsBuffer packetReader(Span<CHAR>(recvBuffer.GetBuffer() + (INT32)reader.GetOffset(), (USIZE)packetSize));

		auto ret = OnPacket(contentType, version, packetReader);
		if (!ret)
		{
			LOG_DEBUG("Failed to process packet for client: %p, current index: %d, packet size: %d", this, (INT32)headerStart, packetSize);
			(void)Close();
			return Result<void, Error>::Err(ret, Error::Tls_ProcessReceiveFailed);
		}
		LOG_DEBUG("Packet processed successfully for client: %p, current index: %d, packet size: %d", this, (INT32)headerStart, packetSize);

		reader.Skip(packetSize);
	}

	INT32 consumed = (INT32)reader.GetOffset();
	Memory::Copy(recvBuffer.GetBuffer(), recvBuffer.GetBuffer() + consumed, recvBuffer.GetSize() - consumed);
	recvBuffer.AppendSize(-consumed);
	return Result<void, Error>::Ok();
}

/// @brief Read and decrypt exactly one TLS record from the socket
/// @return Result indicating success (decryptedSize/decryptedPos updated) or error
/// @details Checks recvBuffer for a complete record before reading from the socket.
///          Processes non-application-data records (alerts, post-handshake) transparently
///          and loops until an application data record is available.
/// @see RFC 8446 Section 5.1 — Record Layer
///      https://datatracker.ietf.org/doc/html/rfc8446#section-5.1

Result<void, Error> TlsClient::ReadNextRecord()
{
	LOG_DEBUG("ReadNextRecord for client: %p", this);
	decryptedSize = 0;
	decryptedPos = 0;

	while (decryptedSize == 0)
	{
		// Try to parse a complete record from recvBuffer
		while (recvBuffer.GetSize() >= 5)
		{
			PUCHAR hdr = (PUCHAR)recvBuffer.GetBuffer();
			UINT8 contentType = hdr[0];
			UINT16 version;
			Memory::Copy(&version, hdr + 1, sizeof(UINT16));
			UINT16 recordLen = ((UINT16)hdr[3] << 8) | (UINT16)hdr[4];

			if (recvBuffer.GetSize() < 5 + (INT32)recordLen)
				break;

			LOG_DEBUG("ReadNextRecord: record type=%d, len=%d", contentType, recordLen);
			TlsBuffer packetReader(Span<CHAR>(recvBuffer.GetBuffer() + 5, (USIZE)recordLen));

			auto ret = OnPacket(contentType, version, packetReader);
			if (!ret)
			{
				(void)Close();
				return Result<void, Error>::Err(ret, Error::Tls_ReadFailed_Receive);
			}

			// Compact recvBuffer — remove the consumed record
			INT32 consumed = 5 + (INT32)recordLen;
			Memory::Copy(recvBuffer.GetBuffer(), recvBuffer.GetBuffer() + consumed, recvBuffer.GetSize() - consumed);
			recvBuffer.AppendSize(-consumed);

			// If we got application data, stop
			if (decryptedSize > 0)
				return Result<void, Error>::Ok();
		}

		// Need more data from socket
		auto checkResult = recvBuffer.CheckSize(RECV_BUFFER_SIZE);
		if (!checkResult)
			return Result<void, Error>::Err(checkResult, Error::Tls_ReadFailed_Receive);

		auto readResult = context.Read(Span<CHAR>(recvBuffer.GetBuffer() + recvBuffer.GetSize(), RECV_BUFFER_SIZE));
		if (!readResult || readResult.Value() <= 0)
		{
			LOG_DEBUG("Failed to read data from socket for client: %p", this);
			(void)Close();
			return Result<void, Error>::Err(readResult, Error::Tls_ReadFailed_Receive);
		}
		INT64 len = readResult.Value();
		if (len > 0x7FFFFFFF)
			return Result<void, Error>::Err(Error::Tls_ReadFailed_Receive);
		LOG_DEBUG("ReadNextRecord: received %lld bytes from socket", len);
		recvBuffer.AppendSize((INT32)len);
	}

	return Result<void, Error>::Ok();
}

/// @brief Open a TLS connection to the server, perform the TLS handshake
/// @return Result indicating whether the TLS connection was opened and the handshake completed successfully
/// @see RFC 8446 Section 4.1 — Handshake Protocol Overview
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4.1

Result<void, Error> TlsClient::Open()
{
	LOG_DEBUG("Connecting to host: %s for client: %p, secure: %d", host, this, secure);

	crypto.Reset();

	auto openResult = context.Open();
	if (!openResult)
	{
		LOG_DEBUG("Failed to connect to host: %s, for client: %p", host, this);
		return Result<void, Error>::Err(openResult, Error::Tls_OpenFailed_Socket);
	}
	LOG_DEBUG("Connected to host: %s, for client: %p", host, this);

	if (!secure)
	{
		LOG_DEBUG("Non-secure connection opened for client: %p", this);
		return Result<void, Error>::Ok();
	}

	auto helloResult = SendClientHello(host);
	if (!helloResult)
	{
		LOG_DEBUG("Failed to send Client Hello for client: %p", this);
		return Result<void, Error>::Err(helloResult, Error::Tls_OpenFailed_Handshake);
	}
	LOG_DEBUG("Client Hello sent successfully for client: %p", this);

	while (stateIndex < HANDSHAKE_STATE_COUNT)
	{
		auto recvResult = ProcessReceive();
		if (!recvResult)
		{
			LOG_DEBUG("Failed to process received data for client: %p", this);
			return Result<void, Error>::Err(recvResult, Error::Tls_OpenFailed_Handshake);
		}
	}

	return Result<void, Error>::Ok();
}

/// @brief Close connection to the server, clean up buffers and cryptographic context
/// @return Result indicating whether the connection was closed successfully
/// @see RFC 8446 Section 6.1 — Closure Alerts
///      https://datatracker.ietf.org/doc/html/rfc8446#section-6.1

Result<void, Error> TlsClient::Close()
{
	stateIndex = 0;
	decryptedPos = 0;
	decryptedSize = 0;

	if (secure)
	{
		recvBuffer.Clear();
		crypto.Destroy();
	}

	LOG_DEBUG("Closing socket for client: %p", this);
	auto closeResult = context.Close();
	if (!closeResult)
	{
		return Result<void, Error>::Err(closeResult, Error::Tls_CloseFailed_Socket);
	}
	return Result<void, Error>::Ok();
}

/// @brief Write data to the TLS channel, encrypting it if the handshake is complete
/// @param buffer Span wrapping the data to send
/// @return Result with the number of bytes written, or an error

Result<UINT32, Error> TlsClient::Write(Span<const CHAR> buffer)
{
	LOG_DEBUG("Sending data for client: %p, size: %d bytes", this, (UINT32)buffer.Size());

	if (!secure)
	{
		auto writeResult = context.Write(buffer);
		if (!writeResult)
		{
			return Result<UINT32, Error>::Err(writeResult, Error::Tls_WriteFailed_Send);
		}
		return Result<UINT32, Error>::Ok(writeResult.Value());
	}

	if (stateIndex < HANDSHAKE_STATE_COUNT)
	{
		LOG_DEBUG("send error, state index is %d", stateIndex);
		return Result<UINT32, Error>::Err(Error::Tls_WriteFailed_NotReady);
	}

	for (UINT32 i = 0; i < (UINT32)buffer.Size();)
	{
		INT32 sendSize = Math::Min((UINT32)buffer.Size() - i, (UINT32)MAX_TLS_SEND_CHUNK);
		auto sendResult = SendPacket(CONTENT_APPLICATION_DATA, TLS_VERSION_1_2, Span<const CHAR>(buffer.Data() + i, (USIZE)sendSize));
		if (!sendResult)
		{
			LOG_DEBUG("Failed to send packet for client: %p, size: %d bytes", this, sendSize);
			return Result<UINT32, Error>::Err(sendResult, Error::Tls_WriteFailed_Send);
		}

		i += sendSize;
	}

	LOG_DEBUG("Data sent successfully for client: %p, total size: %d bytes", this, (UINT32)buffer.Size());
	return Result<UINT32, Error>::Ok((UINT32)buffer.Size());
}

/// @brief Read from the TLS channel, decrypting data if the handshake is complete
/// @param buffer Span wrapping the output buffer for read data
/// @return Result with the number of bytes read, or an error

Result<SSIZE, Error> TlsClient::Read(Span<CHAR> buffer)
{
	if (!secure)
	{
		auto readResult = context.Read(buffer);
		if (!readResult)
		{
			return Result<SSIZE, Error>::Err(readResult, Error::Tls_ReadFailed_Receive);
		}
		return Result<SSIZE, Error>::Ok(readResult.Value());
	}

	if (stateIndex < HANDSHAKE_STATE_COUNT)
	{
		LOG_DEBUG("recv error, state index is %d", stateIndex);
		return Result<SSIZE, Error>::Err(Error::Tls_ReadFailed_NotReady);
	}
	LOG_DEBUG("Reading data for client: %p, requested size: %d", this, (INT32)buffer.Size());

	INT32 totalRead = 0;
	while (totalRead < (INT32)buffer.Size())
	{
		// Serve from current decrypted record
		if (decryptedPos < decryptedSize)
		{
			auto decoded = crypto.GetDecodedData();
			INT32 available = Math::Min((INT32)buffer.Size() - totalRead, decryptedSize - decryptedPos);
			Memory::Copy(buffer.Data() + totalRead, decoded.Data() + decryptedPos, available);
			decryptedPos += available;
			totalRead += available;
			LOG_DEBUG("Served %d bytes from decoded record, pos=%d/%d", available, decryptedPos, decryptedSize);
			continue;
		}

		// Need next record — read and decrypt one TLS record
		auto r = ReadNextRecord();
		if (!r)
		{
			if (totalRead > 0)
				return Result<SSIZE, Error>::Ok((SSIZE)totalRead);
			return Result<SSIZE, Error>::Err(r, Error::Tls_ReadFailed_Receive);
		}
	}

	return Result<SSIZE, Error>::Ok((SSIZE)totalRead);
}

/// @brief Factory method for TlsClient — creates and validates the underlying socket
/// @param host The hostname of the server to connect to
/// @param ipAddress The IP address of the server to connect to
/// @param port The port number of the server to connect to
/// @param secure Whether to use TLS handshake or plain TCP
/// @return Ok(TlsClient) on success, or Err with Tls_CreateFailed on failure

Result<TlsClient, Error> TlsClient::Create(PCCHAR host, const IPAddress &ipAddress, UINT16 port, BOOL secure)
{
	auto socketResult = Socket::Create(ipAddress, port);
	if (!socketResult)
		return Result<TlsClient, Error>::Err(socketResult, Error::Tls_CreateFailed);

	TlsClient client(host, ipAddress, static_cast<Socket &&>(socketResult.Value()), secure);
	return Result<TlsClient, Error>::Ok(static_cast<TlsClient &&>(client));
}
