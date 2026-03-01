#include "runtime/network/tls/tls.h"
#include "core/io/binary_reader.h"
#include "core/memory/memory.h"
#include "platform/system/random.h"
#include "platform/network/socket.h"
#include "core/string/string.h"
#include "platform/platform.h"
#include "platform/io/logger.h"
#include "core/math/math.h"

/// TLS_CHACHA20_POLY1305_SHA256 cipher suite identifier (RFC 8446 Section B.4)
#define TLS_CHACHA20_POLY1305_SHA256 0x1303

/// ChangeCipherSpec content type (RFC 5246 Section 6.2.1 — legacy, used in TLS 1.3 middlebox compat)
#define CONTENT_CHANGECIPHERSPEC 0x14
/// Alert content type (RFC 8446 Section 5.1)
#define CONTENT_ALERT 0x15
/// Handshake content type (RFC 8446 Section 5.1)
#define CONTENT_HANDSHAKE 0x16

/// ClientHello handshake message type (RFC 8446 Section 4.1.2)
#define MSG_CLIENT_HELLO 0x01
/// ServerHello handshake message type (RFC 8446 Section 4.1.3)
#define MSG_SERVER_HELLO 0x02
/// EncryptedExtensions handshake message type (RFC 8446 Section 4.3.1)
#define MSG_ENCRYPTED_EXTENSIONS 0x08
/// Certificate handshake message type (RFC 8446 Section 4.4.2)
#define MSG_CERTIFICATE 0x0B
/// ServerHelloDone handshake message type (legacy TLS 1.2, RFC 5246 Section 7.4.5)
#define MSG_SERVER_HELLO_DONE 0x0E
/// CertificateVerify handshake message type (RFC 8446 Section 4.4.3)
#define MSG_CERTIFICATE_VERIFY 0x0F
/// ClientKeyExchange handshake message type (legacy TLS 1.2, RFC 5246 Section 7.4.7)
#define MSG_CLIENT_KEY_EXCHANGE 0x10
/// Finished handshake message type (RFC 8446 Section 4.4.4)
#define MSG_FINISHED 0x14

/// ChangeCipherSpec message value (RFC 5246 Section 7.1 — legacy compatibility)
#define MSG_CHANGE_CIPHER_SPEC 0x01

/// @brief TLS extension type identifiers
/// @see https://www.iana.org/assignments/tls-extensiontype-values/tls-extensiontype-values.xhtml
/// @see RFC 8446 Section 4.2 — Extensions
///      https://datatracker.ietf.org/doc/html/rfc8446#section-4.2
enum class SslExtension : UINT16
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

Result<void, Error> TlsClient::SendPacket(INT32 packetType, INT32 ver, TlsBuffer &buf)
{
	if (packetType == CONTENT_HANDSHAKE && buf.GetSize() > 0)
	{
		LOG_DEBUG("Sending handshake packet with type: %d, version: %d, size: %d bytes", packetType, ver, buf.GetSize());
		crypto.UpdateHash(Span<const CHAR>(buf.GetBuffer(), buf.GetSize()));
	}
	LOG_DEBUG("Sending packet with type: %d, version: %d, size: %d bytes", packetType, ver, buf.GetSize());

	TlsBuffer tempBuffer;
	tempBuffer.Append<CHAR>(packetType);
	tempBuffer.Append<INT16>(ver);
	INT32 bodySizeIndex = tempBuffer.AppendSize(2); // tls body size

	BOOL keepOriginal = packetType == CONTENT_CHANGECIPHERSPEC || packetType == CONTENT_ALERT;
	if (!keepOriginal && crypto.GetEncoding())
	{
		LOG_DEBUG("Encoding packet with type: %d, size: %d bytes", packetType, buf.GetSize());
		buf.Append<CHAR>(packetType);
		(tempBuffer.GetBuffer())[0] = CONTENT_APPLICATION_DATA;
	}
	LOG_DEBUG("Encoding buffer with size: %d bytes, keepOriginal: %d", buf.GetSize(), keepOriginal);
	crypto.Encode(tempBuffer, Span<const CHAR>(buf.GetBuffer(), buf.GetSize()), keepOriginal);

	*(UINT16 *)(tempBuffer.GetBuffer() + bodySizeIndex) = UINT16SwapByteOrder(tempBuffer.GetSize() - bodySizeIndex - 2);
	auto writeResult = context.Write(Span<const CHAR>(tempBuffer.GetBuffer(), tempBuffer.GetSize()));
	if (!writeResult)
	{
		LOG_DEBUG("Failed to write packet to socket");
		return Result<void, Error>::Err(writeResult, Error::Tls_SendPacketFailed);
	}
	LOG_DEBUG("Packet sent successfully, bytesWritten: %d", writeResult.Value());
	return Result<void, Error>::Ok();
}

/// @brief Sent a ClientHello message to initiate the TLS handshake with the server
/// @param host The hostname of the server to connect to
/// @return Result indicating success or Tls_ClientHelloFailed error

Result<void, Error> TlsClient::SendClientHello(const CHAR *host)
{

	LOG_DEBUG("Sending ClientHello for client: %p, host: %s", this, host);

	sendBuffer.Clear();

	BOOL hastls13 = false;

	sendBuffer.Append<CHAR>(MSG_CLIENT_HELLO);
	INT32 handshakeSizeIndex = sendBuffer.AppendSize(3); // tls handshake body size
	LOG_DEBUG("Appending ClientHello with handshake size index: %d", handshakeSizeIndex);

	sendBuffer.Append<INT16>(0x0303);
	LOG_DEBUG("Appending ClientHello with version: 0x0303");
	sendBuffer.Append(Span<const CHAR>((const CHAR *)crypto.CreateClientRand(), RAND_SIZE));
	LOG_DEBUG("Appending ClientHello with client random data");
	sendBuffer.Append<CHAR>(0);
	LOG_DEBUG("Client has %d ciphers to append", crypto.GetCipherCount());
	INT32 cipherCountIndex = sendBuffer.AppendSize(2);
	LOG_DEBUG("Appending ClientHello with cipher count index: %d", cipherCountIndex);
	for (INT32 i = 0; i < crypto.GetCipherCount(); i++)
	{
		AppendU16BE(sendBuffer, (UINT16)TLS_CHACHA20_POLY1305_SHA256);
		hastls13 = true;
	}
	LOG_DEBUG("Appending ClientHello with %d ciphers", crypto.GetCipherCount());
	*(PUINT16)(sendBuffer.GetBuffer() + cipherCountIndex) = UINT16SwapByteOrder(sendBuffer.GetSize() - cipherCountIndex - 2);
	sendBuffer.Append<CHAR>(1);
	sendBuffer.Append<CHAR>(0);

	INT32 extSizeIndex = sendBuffer.AppendSize(2);
	LOG_DEBUG("Appending ClientHello with extension size index: %d", extSizeIndex);
	AppendU16BE(sendBuffer, (UINT16)SslExtension::ServerName);
	INT32 hostLen = (INT32)StringUtils::Length((PCHAR)host);
	LOG_DEBUG("Appending ClientHello with host: %s, length: %d", host, hostLen);
	AppendU16BE(sendBuffer, hostLen + 5);
	AppendU16BE(sendBuffer, hostLen + 3);
	sendBuffer.Append<CHAR>(0);
	AppendU16BE(sendBuffer, hostLen);
	sendBuffer.Append(Span<const CHAR>(host, hostLen));

	AppendU16BE(sendBuffer, (UINT16)SslExtension::SupportedGroups); // ext type
	AppendU16BE(sendBuffer, ECC_COUNT * 2 + 2);    // ext size
	AppendU16BE(sendBuffer, ECC_COUNT * 2);
	LOG_DEBUG("Appending ClientHello with supported groups, count: %d", ECC_COUNT);
	AppendU16BE(sendBuffer, (UINT16)EccGroup::Secp256r1);
	AppendU16BE(sendBuffer, (UINT16)EccGroup::Secp384r1);

	if (hastls13)
	{
		LOG_DEBUG("Appending ClientHello with TLS 1.3 specific extensions");
		AppendU16BE(sendBuffer, (UINT16)SslExtension::SupportedVersion);
		AppendU16BE(sendBuffer, 3);
		sendBuffer.Append<CHAR>(2);
		// tls 1.3 version
		AppendU16BE(sendBuffer, 0x0304);

		AppendU16BE(sendBuffer, (UINT16)SslExtension::SignatureAlgorithms);
		AppendU16BE(sendBuffer, 24);
		AppendU16BE(sendBuffer, 22);
		AppendU16BE(sendBuffer, 0x0403);
		AppendU16BE(sendBuffer, 0x0503);
		AppendU16BE(sendBuffer, 0x0603);
		AppendU16BE(sendBuffer, 0x0804);
		AppendU16BE(sendBuffer, 0x0805);
		AppendU16BE(sendBuffer, 0x0806);
		AppendU16BE(sendBuffer, 0x0401);
		AppendU16BE(sendBuffer, 0x0501);
		AppendU16BE(sendBuffer, 0x0601);
		AppendU16BE(sendBuffer, 0x0203);
		AppendU16BE(sendBuffer, 0x0201);

		AppendU16BE(sendBuffer, (UINT16)SslExtension::KeyShare); // ext type
		INT32 shareSize = sendBuffer.AppendSize(2);
		sendBuffer.AppendSize(2);
		EccGroup ecc_iana_list[2]{};
		ecc_iana_list[0] = EccGroup::Secp256r1;
		ecc_iana_list[1] = EccGroup::Secp384r1;

		for (INT32 i = 0; i < ECC_COUNT; i++)
		{
			UINT16 eccIana = (UINT16)ecc_iana_list[i];
			AppendU16BE(sendBuffer, eccIana);
			INT32 shareSizeSub = sendBuffer.AppendSize(2);
			auto r = crypto.ComputePublicKey(i, sendBuffer);
			if (!r)
			{
				LOG_DEBUG("Failed to compute public key for ECC group %d", i);
				return Result<void, Error>::Err(r, Error::Tls_ClientHelloFailed);
			}
			LOG_DEBUG("Computed public key for ECC group %d, size: %d bytes", i, sendBuffer.GetSize() - shareSizeSub - 2);
			*(UINT16 *)(sendBuffer.GetBuffer() + shareSizeSub) = UINT16SwapByteOrder(sendBuffer.GetSize() - shareSizeSub - 2);
		}
		*(UINT16 *)(sendBuffer.GetBuffer() + shareSize) = UINT16SwapByteOrder(sendBuffer.GetSize() - shareSize - 2);
		*(UINT16 *)(sendBuffer.GetBuffer() + shareSize + 2) = UINT16SwapByteOrder(sendBuffer.GetSize() - shareSize - 4);
	}
	LOG_DEBUG("Appending ClientHello with extensions, size: %d bytes", sendBuffer.GetSize() - extSizeIndex - 2);

	*(UINT16 *)(sendBuffer.GetBuffer() + extSizeIndex) = UINT16SwapByteOrder(sendBuffer.GetSize() - extSizeIndex - 2);
	sendBuffer.GetBuffer()[handshakeSizeIndex] = 0;
	*(UINT16 *)(sendBuffer.GetBuffer() + handshakeSizeIndex + 1) = UINT16SwapByteOrder(sendBuffer.GetSize() - handshakeSizeIndex - 3);

	auto r = SendPacket(CONTENT_HANDSHAKE, 0x303, sendBuffer);
	if (!r)
		return Result<void, Error>::Err(r, Error::Tls_ClientHelloFailed);
	return Result<void, Error>::Ok();
}

/// @brief Send a Client Finished message to complete the TLS handshake
/// @return Result indicating success or Tls_ClientFinishedFailed error

Result<void, Error> TlsClient::SendClientFinished()
{
	TlsBuffer verify;
	sendBuffer.Clear();
	LOG_DEBUG("Sending Client Finished for client: %p", this);
	crypto.ComputeVerify(verify, CIPHER_HASH_SIZE, 0);
	LOG_DEBUG("Computed verify data for Client Finished, size: %d bytes", verify.GetSize());
	sendBuffer.Append<CHAR>(MSG_FINISHED);
	sendBuffer.Append<CHAR>(0);
	sendBuffer.Append<INT16>(UINT16SwapByteOrder(verify.GetSize()));
	sendBuffer.Append(Span<const CHAR>(verify.GetBuffer(), verify.GetSize()));

	auto r = SendPacket(CONTENT_HANDSHAKE, 0x303, sendBuffer);
	if (!r)
		return Result<void, Error>::Err(r, Error::Tls_ClientFinishedFailed);
	return Result<void, Error>::Ok();
}

/// @brief Send a Client Key Exchange message to the server during the TLS handshake
/// @return Result indicating success or Tls_ClientExchangeFailed error

Result<void, Error> TlsClient::SendClientExchange()
{
	sendBuffer.Clear();
	TlsBuffer &pubkey = crypto.GetPubKey();
	LOG_DEBUG("Sending Client Key Exchange for client: %p, public key size: %d bytes", this, pubkey.GetSize());
	sendBuffer.Append<CHAR>(MSG_CLIENT_KEY_EXCHANGE);
	sendBuffer.Append<CHAR>(0);
	sendBuffer.Append<INT16>(UINT16SwapByteOrder(pubkey.GetSize() + 1));
	sendBuffer.Append<CHAR>((pubkey.GetSize())); // tls body size
	sendBuffer.Append(Span<const CHAR>(pubkey.GetBuffer(), pubkey.GetSize()));
	auto r = SendPacket(CONTENT_HANDSHAKE, 0x303, sendBuffer);
	if (!r) 
		return Result<void, Error>::Err(r, Error::Tls_ClientExchangeFailed);
	return Result<void, Error>::Ok();
}

/// @brief Send a Change Cipher Spec message to the server to indicate that subsequent messages will be encrypted
/// @return Result indicating success or Tls_ChangeCipherSpecFailed error

Result<void, Error> TlsClient::SendChangeCipherSpec()
{
	sendBuffer.Clear();
	sendBuffer.Append<CHAR>(1);
	auto r = SendPacket(CONTENT_CHANGECIPHERSPEC, 0x303, sendBuffer);
	if (!r)
		return Result<void, Error>::Err(r, Error::Tls_ChangeCipherSpecFailed);
	return Result<void, Error>::Ok();
}

/// @brief Process the ServerHello message from the server and advances the TLS handshake state
/// @param reader Buffer containing the ServerHello message data
/// @return Result indicating success or Tls_ServerHelloFailed error

Result<void, Error> TlsClient::OnServerHello(TlsBuffer &reader)
{
	CHAR serverRand[RAND_SIZE];

	LOG_DEBUG("Processing ServerHello for client: %p", this);
	reader.ReadU24BE();                        // handshake body size (already bounded by TLS record)
	UINT16SwapByteOrder(reader.Read<INT16>()); // version
	reader.Read(Span<CHAR>(serverRand, sizeof(serverRand)));
	INT32 sessionLen = reader.Read<INT8>();
	LOG_DEBUG("ServerHello session length: %d", sessionLen);
	reader.AdvanceReadPosition(sessionLen);
	reader.Read<INT16>(); // cur_cipher
	reader.Read<INT8>();
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
		SslExtension type = (SslExtension)UINT16SwapByteOrder(reader.Read<INT16>());
		if (type == SslExtension::SupportedVersion)
		{
			LOG_DEBUG("Processing SslExtension::SupportedVersion extension");
			reader.Read<INT16>();
			tlsVer = UINT16SwapByteOrder(reader.Read<INT16>());
		}
		else if (type == SslExtension::KeyShare)
		{
			LOG_DEBUG("Processing SslExtension::KeyShare extension");
			INT32 size = UINT16SwapByteOrder(reader.Read<INT16>());
			eccgroup = (EccGroup)UINT16SwapByteOrder(reader.Read<INT16>());
			if (size > 4)
			{
				LOG_DEBUG("Reading public key from SslExtension::KeyShare, size: %d bytes", size);
				pubkey.SetSize(UINT16SwapByteOrder(reader.Read<INT16>()));
				reader.Read(Span<CHAR>(pubkey.GetBuffer(), pubkey.GetSize()));
			}
			LOG_DEBUG("SslExtension::KeyShare processed, ECC group: %d, public key size: %d bytes", (UINT16)eccgroup, pubkey.GetSize());
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
		if (tlsVer != 0x0304 || pubkey.GetSize() <= 0 || eccgroup == EccGroup::None)
		{
			LOG_DEBUG("Invalid TLS version or public key size, tlsVer: %d, pubkey.size: %d, eccgroup: %d", tlsVer, pubkey.GetSize(), eccgroup);
			return Result<void, Error>::Err(Error::Tls_ServerHelloFailed);
		}

		LOG_DEBUG("Valid TLS version and public key size, tlsVer: %d, pubkey.size: %d, eccgroup: %d", tlsVer, pubkey.GetSize(), eccgroup);

		auto r = crypto.ComputeKey(eccgroup, Span<const CHAR>(pubkey.GetBuffer(), pubkey.GetSize()), Span<CHAR>());
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

/// @brief Process the ServerHelloDone message from the server and advances the TLS handshake state
/// @return Result indicating success or Tls_ServerHelloDoneFailed error

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

Result<void, Error> TlsClient::VerifyFinished(TlsBuffer &reader)
{
	INT32 server_finished_size = reader.ReadU24BE();
	if (server_finished_size < 0 || server_finished_size > reader.GetSize() - reader.GetReadPosition())
		return Result<void, Error>::Err(Error::Tls_VerifyFinishedFailed);
	LOG_DEBUG("Verifying Finished for client: %p, size: %d bytes", this, server_finished_size);
	TlsBuffer verify;
	crypto.ComputeVerify(verify, server_finished_size, 1);
	LOG_DEBUG("Computed verify data for Finished, size: %d bytes", verify.GetSize());

	if (Memory::Compare(verify.GetBuffer(), reader.GetBuffer() + reader.GetReadPosition(), server_finished_size) != 0)
	{
		LOG_DEBUG("Finished verification failed for client: %p, expected size: %d, actual size: %d", this, verify.GetSize(), server_finished_size);
		return Result<void, Error>::Err(Error::Tls_VerifyFinishedFailed);
	}
	LOG_DEBUG("Finished verification succeeded for client: %p", this);
	return Result<void, Error>::Ok();
}

/// @brief Finished message from the server has been received, process it and advance the TLS handshake state to complete the handshake
/// @return Result indicating success or Tls_ServerFinishedFailed error

Result<void, Error> TlsClient::OnServerFinished()
{
	LOG_DEBUG("Processing Server Finished for client: %p", this);
	CHAR finished_hash[MAX_HASH_LEN] = {0};
	crypto.GetHash(Span<CHAR>(finished_hash, CIPHER_HASH_SIZE));
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
	auto r2 = crypto.ComputeKey(EccGroup::None, Span<const CHAR>(), Span<CHAR>(finished_hash, CIPHER_HASH_SIZE));
	if (!r2)
	{
		LOG_DEBUG("Failed to compute TLS 1.3 key for client: %p", this);
		return Result<void, Error>::Err(r2, Error::Tls_ServerFinishedFailed);
	}

	LOG_DEBUG("Server Finished processed successfully for client: %p", this);
	return Result<void, Error>::Ok();
}

/// @brief Process incoming TLS packets from the server, handle different packet types and advance the TLS handshake state accordingly
/// @param packetType Type of the incoming TLS packet (e.g., handshake, alert)
/// @param version Version of TLS used in the packet
/// @param tlsReader Buffer containing the packet data to process
/// @return Result indicating success or Tls_OnPacketFailed error

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
			tlsReader.SetSize(tlsReader.GetSize() - 1);
		}
		LOG_DEBUG("Packet type after processing: %d, buffer size: %d bytes", packetType, tlsReader.GetSize());
	}

	while (tlsReader.GetReadPosition() < tlsReader.GetSize())
	{
		INT32 seg_size;
		if (packetType == CONTENT_HANDSHAKE)
		{
			INT32 remaining = tlsReader.GetSize() - tlsReader.GetReadPosition();
			if (remaining < 4)
				return Result<void, Error>::Err(Error::Tls_OnPacketFailed);
			PUCHAR seg = (PUCHAR)(tlsReader.GetBuffer() + tlsReader.GetReadPosition());
			seg_size = 4 + (((UINT32)seg[1] << 16) | ((UINT32)seg[2] << 8) | (UINT32)seg[3]);
			if (seg_size > remaining)
				return Result<void, Error>::Err(Error::Tls_OnPacketFailed);
		}
		else
		{
			seg_size = tlsReader.GetSize();
		}
		TlsBuffer reader_sig(Span<CHAR>(tlsReader.GetBuffer() + tlsReader.GetReadPosition(), (USIZE)seg_size));

		TlsState state_seq[6]{};

		state_seq[0] = {CONTENT_HANDSHAKE, MSG_SERVER_HELLO};
		state_seq[1] = {CONTENT_CHANGECIPHERSPEC, MSG_CHANGE_CIPHER_SPEC};
		state_seq[2] = {CONTENT_HANDSHAKE, MSG_ENCRYPTED_EXTENSIONS};
		state_seq[3] = {CONTENT_HANDSHAKE, MSG_CERTIFICATE};
		state_seq[4] = {CONTENT_HANDSHAKE, MSG_CERTIFICATE_VERIFY};
		state_seq[5] = {CONTENT_HANDSHAKE, MSG_FINISHED};

		if (stateIndex < 6 && packetType != CONTENT_ALERT)
		{
			LOG_DEBUG("Checking state sequence for client: %p, state index: %d, packet type: %d, handshake type: %d", this, stateIndex, packetType, reader_sig.GetBuffer()[0]);
			if (state_seq[stateIndex].ContentType != packetType || state_seq[stateIndex].HandshakeType != reader_sig.GetBuffer()[0])
			{
				LOG_DEBUG("State sequence mismatch for client: %p, expected type: %d, expected handshake type: %d, actual type: %d, actual handshake type: %d",
						  this, state_seq[stateIndex].ContentType, state_seq[stateIndex].HandshakeType, packetType, reader_sig.GetBuffer()[0]);
				return Result<void, Error>::Err(Error::Tls_OnPacketFailed);
			}
			LOG_DEBUG("State sequence matches for client: %p, state index: %d, packet type: %d, handshake type: %d", this, stateIndex, packetType, reader_sig.GetBuffer()[0]);
			stateIndex++;
		}

		if (packetType == CONTENT_HANDSHAKE && reader_sig.GetSize() > 0 && reader_sig.GetBuffer()[0] != MSG_FINISHED)
		{
			LOG_DEBUG("Updating hash for client: %p, packet type: %d, size: %d bytes", this, packetType, reader_sig.GetSize());
			crypto.UpdateHash(Span<const CHAR>(reader_sig.GetBuffer(), reader_sig.GetSize()));
		}
		if (packetType == CONTENT_HANDSHAKE)
		{
			LOG_DEBUG("Processing handshake packet for client: %p, handshake type: %d", this, reader_sig.GetBuffer()[0]);
			INT32 handshakeType = reader_sig.Read<INT8>();
			LOG_DEBUG("Handshake type: %d", handshakeType);
			if (handshakeType == MSG_SERVER_HELLO)
			{
				LOG_DEBUG("Processing ServerHello for client: %p", this);
				auto r = OnServerHello(reader_sig);
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
				auto r = VerifyFinished(reader_sig);
				if (!r)
				{
					LOG_DEBUG("Failed to verify Finished for client: %p", this);
					return Result<void, Error>::Err(r, Error::Tls_OnPacketFailed);
				}
				LOG_DEBUG("Server Finished verified successfully for client: %p", this);
				crypto.UpdateHash(Span<const CHAR>(reader_sig.GetBuffer(), reader_sig.GetSize()));
				auto r2 = OnServerFinished();
				if (!r2)
				{
					LOG_DEBUG("Failed to process Server Finished for client: %p", this);
					return Result<void, Error>::Err(r2, Error::Tls_OnPacketFailed);
				}
				LOG_DEBUG("Server Finished processed successfully for client: %p", this);
			}
		}
		else if (packetType == CONTENT_CHANGECIPHERSPEC)
		{
		}
		else if (packetType == CONTENT_ALERT)
		{
			LOG_DEBUG("Processing Alert for client: %p", this);
			if (reader_sig.GetSize() >= 2)
			{
				[[maybe_unused]] INT32 level = reader_sig.Read<INT8>();
				[[maybe_unused]] INT32 code = reader_sig.Read<INT8>();
				LOG_ERROR("TLS Alert received for client: %p, level: %d, code: %d", this, level, code);
				return Result<void, Error>::Err(Error::Tls_OnPacketFailed);
			}
			LOG_DEBUG("TLS Alert received for client: %p, but buffer size is less than 2 bytes", this);
		}
		else if (packetType == CONTENT_APPLICATION_DATA)
		{
			LOG_DEBUG("Processing Application Data for client: %p, size: %d bytes", this, reader_sig.GetSize());
			channelBuffer.Append(Span<const CHAR>(reader_sig.GetBuffer(), reader_sig.GetSize()));
		}
		tlsReader.AdvanceReadPosition(seg_size);
	}

	return Result<void, Error>::Ok();
}

/// @brief Packet processing - read data from the socket, parse TLS packets
/// @return Result indicating success or Tls_ProcessReceiveFailed error

Result<void, Error> TlsClient::ProcessReceive()
{
	LOG_DEBUG("Processing received data for client: %p, current state index: %d", this, stateIndex);
	recvBuffer.CheckSize(4096 * 4);
	auto readResult = context.Read(Span<CHAR>(recvBuffer.GetBuffer() + recvBuffer.GetSize(), 4096 * 4));
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

		TlsBuffer unnamed(Span<CHAR>((PCHAR)reader.Current(), (USIZE)packetSize));

		auto ret = OnPacket(contentType, version, unnamed);
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

/// @brief Read data from channel buffer
/// @param output Span wrapping the output buffer
/// @return Ok(bytes read) on success, or Err when channel is empty

Result<INT32, Error> TlsClient::ReadChannel(Span<CHAR> output)
{
	INT32 movesize = Math::Min((INT32)output.Size(), channelBuffer.GetSize() - channelBytesRead);
	LOG_DEBUG("Reading from channel for client: %p, requested size: %d, available size: %d, readed size: %d",
			  this, (INT32)output.Size(), channelBuffer.GetSize() - channelBytesRead, channelBytesRead);
	Memory::Copy(output.Data(), channelBuffer.GetBuffer() + channelBytesRead, movesize);
	channelBytesRead += movesize;
	if (((channelBytesRead > (channelBuffer.GetSize() >> 2) * 3) && (channelBuffer.GetSize() > 1024 * 1024)) || (channelBytesRead >= channelBuffer.GetSize()))
	{
		LOG_DEBUG("Clearing recv channel for client: %p, readed size: %d, total size: %d",
				  this, channelBytesRead, channelBuffer.GetSize());
		Memory::Copy(channelBuffer.GetBuffer(), channelBuffer.GetBuffer() + channelBytesRead, channelBuffer.GetSize() - channelBytesRead);
		channelBuffer.AppendSize(-channelBytesRead);
		channelBytesRead = 0;
	}
	LOG_DEBUG("Read %d bytes from channel for client: %p, new readed size: %d, total size: %d",
			  movesize, this, channelBytesRead, channelBuffer.GetSize());
	if (movesize == 0)
	{
		LOG_ERROR("recv channel size is 0, maybe error");
		return Result<INT32, Error>::Err(Error::Tls_ReadFailed_Channel);
	}
	LOG_DEBUG("Returning movesize: %d for client: %p", movesize, this);
	return Result<INT32, Error>::Ok(movesize);
}

/// @brief Open a TLS connection to the server, perform the TLS handshake by sending the ClientHello message and processing the server's responses
/// @return Result indicating whether the TLS connection was opened and the handshake completed successfully

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

	while (stateIndex < 6)
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

Result<void, Error> TlsClient::Close()
{
	stateIndex = 0;
	channelBytesRead = 0;

	if (secure)
	{
		recvBuffer.Clear();
		channelBuffer.Clear();
		sendBuffer.Clear();
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

/// @brief Write data to the TLS channel, encrypting it if the handshake is complete and the encoding is enabled
/// @param buffer Pointer to the input buffer containing the data to be sent to the server
/// @param bufferLength Length of the input buffer in bytes
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

	if (stateIndex < 6)
	{
		LOG_DEBUG("send error, state index is %d", stateIndex);
		return Result<UINT32, Error>::Err(Error::Tls_WriteFailed_NotReady);
	}

	sendBuffer.Clear();
	for (UINT32 i = 0; i < (UINT32)buffer.Size();)
	{
		INT32 sendSize = Math::Min((UINT32)buffer.Size() - i, 1024 * 16);
		sendBuffer.SetSize(sendSize);
		Memory::Copy(sendBuffer.GetBuffer(), buffer.Data() + i, sendSize);
		auto sendResult = SendPacket(CONTENT_APPLICATION_DATA, 0x303, sendBuffer);
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

/// @brief Read from the TLS channel, decrypting data if the handshake is complete and the encoding is enabled, and store it in the provided buffer
/// @param buffer Buffer where the read data will be stored
/// @param bufferLength Length of the buffer in bytes, indicating the maximum number of bytes to read from the TLS channel
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

	if (stateIndex < 6)
	{
		LOG_DEBUG("recv error, state index is %d", stateIndex);
		return Result<SSIZE, Error>::Err(Error::Tls_ReadFailed_NotReady);
	}
	LOG_DEBUG("Reading data for client: %p, requested size: %d", this, (INT32)buffer.Size());
	while (channelBuffer.GetSize() <= channelBytesRead)
	{
		auto recvResult = ProcessReceive();
		if (!recvResult)
		{
			LOG_DEBUG("recv error, maybe close socket");
			return Result<SSIZE, Error>::Err(recvResult, Error::Tls_ReadFailed_Receive);
		}
	}

	auto channelResult = ReadChannel(buffer);
	if (!channelResult)
		return Result<SSIZE, Error>::Err(channelResult, Error::Tls_ReadFailed_Channel);
	return Result<SSIZE, Error>::Ok((SSIZE)channelResult.Value());
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
