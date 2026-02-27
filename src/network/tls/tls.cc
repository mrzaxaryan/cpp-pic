#include "tls.h"
#include "memory.h"
#include "random.h"
#include "socket.h"
#include "string.h"
#include "platform.h"
#include "logger.h"
#include "math.h"

#define TLS_CHACHA20_POLY1305_SHA256 0x1303

// The following defines SSL 3.0 content types
// CONTENT_APPLICATION_DATA is defined in tls_cipher.h
#define CONTENT_CHANGECIPHERSPEC 0x14
#define CONTENT_ALERT 0x15
#define CONTENT_HANDSHAKE 0x16

// The following defines SSL 3.0/TLS 1.0 Handshake message types
#define MSG_CLIENT_HELLO 0x01
#define MSG_SERVER_HELLO 0x02
#define MSG_ENCRYPTED_EXTENSIONS 0x08 // RFC8446
#define MSG_CERTIFICATE 0x0B
#define MSG_SERVER_HELLO_DONE 0x0E // Not used in TLS1.3
#define MSG_CERTIFICATE_VERIFY 0x0F
#define MSG_CLIENT_KEY_EXCHANGE 0x10 // Not used in TLS1.3
#define MSG_FINISHED 0x14

// This is only used in CONTENT_CHANGECIPHERSPEC content type
#define MSG_CHANGE_CIPHER_SPEC 0x01

// https://www.iana.org/assignments/tls-extensiontype-values/tls-extensiontype-values.xhtml
typedef enum
{
    EXT_SERVER_NAME = 0x0000,          // Type: server_name (0)
    EXT_SUPPORTED_GROUPS = 0x000A,     // Type: supported_groups (10) Y [RFC4492][RFC8422][RFC7748][RFC7919] https://tools.ietf.org/html/rfc8422#section-5.1.1
    EXT_EC_POINT_FORMATS = 0x000B,     // Type: ec_point_formats (11) Y [RFC8422] https://tools.ietf.org/html/rfc8422#section-5.1. https://tools.ietf.org/html/rfc4492#section-5.1.2
    EXT_SIGNATURE_ALGORITHMS = 0x000D, // Type: signature_algorithms (13)

    EXT_ENCRYPT_THEN_MAC = 0x0016,       // Type: encrypt_then_mac(22)
    EXT_EXTENDED_MASTER_SECRET = 0x0017, // Type: extended_master_secret (23)
    EXT_RECORD_SIZE_LIMIT = 0x001C,      // Type: 28	record_size_limit CH, EE Y [RFC8449]
    EXT_SESSIONTICKET_TLS = 0x0023,      // Type: SessionTicket TLS(35)

    EXT_PRESHARED_KEY = 0x0029,          // Type: 41	pre_shared_key CH, SH Y [RFC8446]
    EXT_SUPPORTED_VERSION = 0x002B,      // Type: supported_versions	CH, SH, HRR	Y [RFC8446]
    EXT_PSK_KEY_EXCHANGE_MODES = 0x002D, // Type: psk_key_exchange_modes	CH	Y [RFC8446]
    EXT_KEY_SHARE = 0x0033,              // Type: key_share	CH, SH, HRR	Y [RFC8446]

    EXT_RENEGOTIATION_INFO = 0xFF01, // Type: renegotiation_info(65281)

    EXT_LAST = 0x7FFF
} SSL_EXTENTION;

static FORCE_INLINE VOID AppendU16BE(TlsBuffer &buf, UINT16 val)
{
    buf.Append<INT16>(UINT16SwapByteOrder(val));
}

/// @brief Send packet data over TLS connection
/// @param packetType Type of the TLS packet (e.g., handshake, application data)
/// @param ver Version of TLS to use for the packet
/// @param buf The buffer containing the packet data to send
/// @return Result indicating success or Tls_SendPacketFailed error

Result<void, Error> TLSClient::SendPacket(INT32 packetType, INT32 ver, TlsBuffer &buf)
{
    if (packetType == CONTENT_HANDSHAKE && buf.GetSize() > 0)
    {
        LOG_DEBUG("Sending handshake packet with type: %d, version: %d, size: %d bytes", packetType, ver, buf.GetSize());
        crypto.UpdateHash(buf.GetBuffer(), buf.GetSize());
    }
    LOG_DEBUG("Sending packet with type: %d, version: %d, size: %d bytes", packetType, ver, buf.GetSize());

    TlsBuffer tempBuffer;
    tempBuffer.Append<CHAR>(packetType);
    tempBuffer.Append<INT16>(ver);
    INT32 bodySizeIndex = tempBuffer.AppendSize(2); // tls body size

    BOOL keep_original = packetType == CONTENT_CHANGECIPHERSPEC || packetType == CONTENT_ALERT;
    if (!keep_original && crypto.GetEncoding())
    {
        LOG_DEBUG("Encoding packet with type: %d, size: %d bytes", packetType, buf.GetSize());
        buf.Append<CHAR>(packetType);
        (tempBuffer.GetBuffer())[0] = CONTENT_APPLICATION_DATA;
    }
    LOG_DEBUG("Encoding buffer with size: %d bytes, keep_original: %d", buf.GetSize(), keep_original);
    crypto.Encode(tempBuffer, buf.GetBuffer(), buf.GetSize(), keep_original);

    *(UINT16 *)(tempBuffer.GetBuffer() + bodySizeIndex) = UINT16SwapByteOrder(tempBuffer.GetSize() - bodySizeIndex - 2);
    auto writeResult = context.Write((PCHAR)tempBuffer.GetBuffer(), tempBuffer.GetSize());
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

Result<void, Error> TLSClient::SendClientHello(const CHAR *host)
{

    LOG_DEBUG("Sending ClientHello for client: %p, host: %s", this, host);

    sendBuffer.Clear();

    BOOL hastls13 = false;

    sendBuffer.Append<CHAR>(MSG_CLIENT_HELLO);
    INT32 handshakeSizeIndex = sendBuffer.AppendSize(3); // tls handshake body size
    LOG_DEBUG("Appending ClientHello with handshake size index: %d", handshakeSizeIndex);

    sendBuffer.Append<INT16>(0x0303);
    LOG_DEBUG("Appending ClientHello with version: 0x0303");
    sendBuffer.Append(crypto.CreateClientRand(), RAND_SIZE);
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
    AppendU16BE(sendBuffer, EXT_SERVER_NAME);
    INT32 hostLen = (INT32)String::Length((PCHAR)host);
    LOG_DEBUG("Appending ClientHello with host: %s, length: %d", host, hostLen);
    AppendU16BE(sendBuffer, hostLen + 5);
    AppendU16BE(sendBuffer, hostLen + 3);
    sendBuffer.Append<CHAR>(0);
    AppendU16BE(sendBuffer, hostLen);
    sendBuffer.Append(host, hostLen);

    AppendU16BE(sendBuffer, EXT_SUPPORTED_GROUPS); // ext type
    AppendU16BE(sendBuffer, ECC_COUNT * 2 + 2);    // ext size
    AppendU16BE(sendBuffer, ECC_COUNT * 2);
    LOG_DEBUG("Appending ClientHello with supported groups, count: %d", ECC_COUNT);
    AppendU16BE(sendBuffer, (UINT16)ECC_secp256r1);
    AppendU16BE(sendBuffer, (UINT16)ECC_secp384r1);

    if (hastls13)
    {
        LOG_DEBUG("Appending ClientHello with TLS 1.3 specific extensions");
        AppendU16BE(sendBuffer, EXT_SUPPORTED_VERSION);
        AppendU16BE(sendBuffer, 3);
        sendBuffer.Append<CHAR>(2);
        // tls 1.3 version
        AppendU16BE(sendBuffer, 0x0304);

        AppendU16BE(sendBuffer, EXT_SIGNATURE_ALGORITHMS);
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

        AppendU16BE(sendBuffer, EXT_KEY_SHARE); // ext type
        INT32 shareSize = sendBuffer.AppendSize(2);
        sendBuffer.AppendSize(2);
        UINT16 ecc_iana_list[2]{};
        ecc_iana_list[0] = ECC_secp256r1;
        ecc_iana_list[1] = ECC_secp384r1;

        for (INT32 i = 0; i < ECC_COUNT; i++)
        {
            UINT16 eccIana = ecc_iana_list[i];
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

Result<void, Error> TLSClient::SendClientFinished()
{
    TlsBuffer verify;
    sendBuffer.Clear();
    LOG_DEBUG("Sending Client Finished for client: %p", this);
    crypto.ComputeVerify(verify, CIPHER_HASH_SIZE, 0);
    LOG_DEBUG("Computed verify data for Client Finished, size: %d bytes", verify.GetSize());
    sendBuffer.Append<CHAR>(MSG_FINISHED);
    sendBuffer.Append<CHAR>(0);
    sendBuffer.Append<INT16>(UINT16SwapByteOrder(verify.GetSize()));
    sendBuffer.Append(verify.GetBuffer(), verify.GetSize());

    auto r = SendPacket(CONTENT_HANDSHAKE, 0x303, sendBuffer);
    if (!r)
        return Result<void, Error>::Err(r, Error::Tls_ClientFinishedFailed);
    return Result<void, Error>::Ok();
}

/// @brief Send a Client Key Exchange message to the server during the TLS handshake
/// @return Result indicating success or Tls_ClientExchangeFailed error

Result<void, Error> TLSClient::SendClientExchange()
{
    sendBuffer.Clear();
    TlsBuffer &pubkey = crypto.GetPubKey();
    LOG_DEBUG("Sending Client Key Exchange for client: %p, public key size: %d bytes", this, pubkey.GetSize());
    sendBuffer.Append<CHAR>(MSG_CLIENT_KEY_EXCHANGE);
    sendBuffer.Append<CHAR>(0);
    sendBuffer.Append<INT16>(UINT16SwapByteOrder(pubkey.GetSize() + 1));
    sendBuffer.Append<CHAR>((pubkey.GetSize())); // tls body size
    sendBuffer.Append(pubkey.GetBuffer(), pubkey.GetSize());
    auto r = SendPacket(CONTENT_HANDSHAKE, 0x303, sendBuffer);
    if (!r) 
        return Result<void, Error>::Err(r, Error::Tls_ClientExchangeFailed);
    return Result<void, Error>::Ok();
}

/// @brief Send a Change Cipher Spec message to the server to indicate that subsequent messages will be encrypted
/// @return Result indicating success or Tls_ChangeCipherSpecFailed error

Result<void, Error> TLSClient::SendChangeCipherSpec()
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

Result<void, Error> TLSClient::OnServerHello(TlsBuffer &reader)
{
    CHAR server_rand[RAND_SIZE];

    LOG_DEBUG("Processing ServerHello for client: %p", this);
    reader.ReadU24BE();                        // handshake body size (already bounded by TLS record)
    UINT16SwapByteOrder(reader.Read<INT16>()); // version
    reader.Read(server_rand, sizeof(server_rand));
    INT32 session_len = reader.Read<INT8>();
    LOG_DEBUG("ServerHello session length: %d", session_len);
    reader.AppendReaded(session_len);
    reader.Read<INT16>(); // cur_cipher
    reader.Read<INT8>();
    auto ret = crypto.UpdateServerInfo();
    if (!ret)
    {
        LOG_DEBUG("Failed to update server info for client: %p", this);
        return Result<void, Error>::Err(ret, Error::Tls_ServerHelloFailed);
    }

    if (reader.GetReaded() >= reader.GetSize())
    {
        LOG_DEBUG("ServerHello reader has reached the end of buffer, no extensions found");
        return Result<void, Error>::Ok();
    }
    LOG_DEBUG("ServerHello has extensions, processing them");

    INT32 ext_size = UINT16SwapByteOrder(reader.Read<INT16>());
    INT32 ext_start = reader.GetReaded();
    INT32 tls_ver = 0;
    LOG_DEBUG("ServerHello extensions size: %d bytes, start index: %d", ext_size, ext_start);
    TlsBuffer pubkey;
    ECC_GROUP eccgroup = ECC_NONE;
    while (reader.GetReaded() < ext_start + ext_size)
    {
        SSL_EXTENTION type = (SSL_EXTENTION)UINT16SwapByteOrder(reader.Read<INT16>());
        if (type == EXT_SUPPORTED_VERSION)
        {
            LOG_DEBUG("Processing EXT_SUPPORTED_VERSION extension");
            reader.Read<INT16>();
            tls_ver = UINT16SwapByteOrder(reader.Read<INT16>());
        }
        else if (type == EXT_KEY_SHARE)
        {
            LOG_DEBUG("Processing EXT_KEY_SHARE extension");
            INT32 size = UINT16SwapByteOrder(reader.Read<INT16>());
            eccgroup = (ECC_GROUP)UINT16SwapByteOrder(reader.Read<INT16>());
            if (size > 4)
            {
                LOG_DEBUG("Reading public key from EXT_KEY_SHARE, size: %d bytes", size);
                pubkey.SetSize(UINT16SwapByteOrder(reader.Read<INT16>()));
                reader.Read(pubkey.GetBuffer(), pubkey.GetSize());
            }
            LOG_DEBUG("EXT_KEY_SHARE processed, ECC group: %d, public key size: %d bytes", eccgroup, pubkey.GetSize());
        }
        else
        {
            // Skip unknown extensions
            INT32 extLen = UINT16SwapByteOrder(reader.Read<INT16>());
            reader.AppendReaded(extLen);
        }
    }
    if (tls_ver != 0)
    {
        LOG_DEBUG("TLS version from ServerHello: %d", tls_ver);
        if (tls_ver != 0x0304 || pubkey.GetSize() <= 0 || eccgroup == ECC_NONE)
        {
            LOG_DEBUG("Invalid TLS version or public key size, tls_ver: %d, pubkey.size: %d, eccgroup: %d", tls_ver, pubkey.GetSize(), eccgroup);
            return Result<void, Error>::Err(Error::Tls_ServerHelloFailed);
        }

        LOG_DEBUG("Valid TLS version and public key size, tls_ver: %d, pubkey.size: %d, eccgroup: %d", tls_ver, pubkey.GetSize(), eccgroup);

        auto r = crypto.ComputeKey(eccgroup, pubkey.GetBuffer(), pubkey.GetSize(), nullptr);
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

Result<void, Error> TLSClient::OnServerHelloDone()
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

Result<void, Error> TLSClient::VerifyFinished(TlsBuffer &reader)
{
    INT32 server_finished_size = reader.ReadU24BE();
    if (server_finished_size < 0 || server_finished_size > reader.GetSize() - reader.GetReaded())
        return Result<void, Error>::Err(Error::Tls_VerifyFinishedFailed);
    LOG_DEBUG("Verifying Finished for client: %p, size: %d bytes", this, server_finished_size);
    TlsBuffer verify;
    crypto.ComputeVerify(verify, server_finished_size, 1);
    LOG_DEBUG("Computed verify data for Finished, size: %d bytes", verify.GetSize());

    if (Memory::Compare(verify.GetBuffer(), reader.GetBuffer() + reader.GetReaded(), server_finished_size) != 0)
    {
        LOG_DEBUG("Finished verification failed for client: %p, expected size: %d, actual size: %d", this, verify.GetSize(), server_finished_size);
        return Result<void, Error>::Err(Error::Tls_VerifyFinishedFailed);
    }
    LOG_DEBUG("Finished verification succeeded for client: %p", this);
    return Result<void, Error>::Ok();
}

/// @brief Finished message from the server has been received, process it and advance the TLS handshake state to complete the handshake
/// @return Result indicating success or Tls_ServerFinishedFailed error

Result<void, Error> TLSClient::OnServerFinished()
{
    LOG_DEBUG("Processing Server Finished for client: %p", this);
    CHAR finished_hash[MAX_HASH_LEN] = {0};
    crypto.GetHash(finished_hash);
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
    auto r2 = crypto.ComputeKey(ECC_NONE, nullptr, 0, finished_hash);
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
/// @param TlsReader Buffer containing the packet data to process
/// @return Result indicating success or Tls_OnPacketFailed error

Result<void, Error> TLSClient::OnPacket(INT32 packetType, INT32 version, TlsBuffer &TlsReader)
{
    if (packetType != CONTENT_CHANGECIPHERSPEC && packetType != CONTENT_ALERT)
    {
        LOG_DEBUG("Processing packet with type: %d, version: %d, size: %d bytes", packetType, version, TlsReader.GetSize());
        auto r = crypto.Decode(TlsReader, version);
        if (!r)
        {
            LOG_DEBUG("Failed to Decode packet for client: %p, type: %d, version: %d", this, packetType, version);
            return Result<void, Error>::Err(r, Error::Tls_OnPacketFailed);
        }
        LOG_DEBUG("Packet decoded successfully for client: %p, type: %d, version: %d", this, packetType, version);
        if (crypto.GetEncoding() && TlsReader.GetSize() > 0)
        {
            LOG_DEBUG("Removing last byte from buffer for client: %p, packet type: %d", this, packetType);
            packetType = TlsReader.GetBuffer()[TlsReader.GetSize() - 1];
            TlsReader.SetSize(TlsReader.GetSize() - 1);
        }
        LOG_DEBUG("Packet type after processing: %d, buffer size: %d bytes", packetType, TlsReader.GetSize());
    }

    while (TlsReader.GetReaded() < TlsReader.GetSize())
    {
        INT32 seg_size;
        if (packetType == CONTENT_HANDSHAKE)
        {
            INT32 remaining = TlsReader.GetSize() - TlsReader.GetReaded();
            if (remaining < 4)
                return Result<void, Error>::Err(Error::Tls_OnPacketFailed);
            PUCHAR seg = (PUCHAR)(TlsReader.GetBuffer() + TlsReader.GetReaded());
            seg_size = 4 + (((UINT32)seg[1] << 16) | ((UINT32)seg[2] << 8) | (UINT32)seg[3]);
            if (seg_size > remaining)
                return Result<void, Error>::Err(Error::Tls_OnPacketFailed);
        }
        else
        {
            seg_size = TlsReader.GetSize();
        }
        TlsBuffer reader_sig(TlsReader.GetBuffer() + TlsReader.GetReaded(), seg_size);

        tlsstate state_seq[6]{};

        state_seq[0] = {CONTENT_HANDSHAKE, MSG_SERVER_HELLO};
        state_seq[1] = {CONTENT_CHANGECIPHERSPEC, MSG_CHANGE_CIPHER_SPEC};
        state_seq[2] = {CONTENT_HANDSHAKE, MSG_ENCRYPTED_EXTENSIONS};
        state_seq[3] = {CONTENT_HANDSHAKE, MSG_CERTIFICATE};
        state_seq[4] = {CONTENT_HANDSHAKE, MSG_CERTIFICATE_VERIFY};
        state_seq[5] = {CONTENT_HANDSHAKE, MSG_FINISHED};

        if (stateIndex < 6 && packetType != CONTENT_ALERT)
        {
            LOG_DEBUG("Checking state sequence for client: %p, state index: %d, packet type: %d, handshake type: %d", this, stateIndex, packetType, reader_sig.GetBuffer()[0]);
            if (state_seq[stateIndex].contentType != packetType || state_seq[stateIndex].handshakeType != reader_sig.GetBuffer()[0])
            {
                LOG_DEBUG("State sequence mismatch for client: %p, expected type: %d, expected handshake type: %d, actual type: %d, actual handshake type: %d",
                          this, state_seq[stateIndex].contentType, state_seq[stateIndex].handshakeType, packetType, reader_sig.GetBuffer()[0]);
                return Result<void, Error>::Err(Error::Tls_OnPacketFailed);
            }
            LOG_DEBUG("State sequence matches for client: %p, state index: %d, packet type: %d, handshake type: %d", this, stateIndex, packetType, reader_sig.GetBuffer()[0]);
            stateIndex++;
        }

        if (packetType == CONTENT_HANDSHAKE && reader_sig.GetSize() > 0 && reader_sig.GetBuffer()[0] != MSG_FINISHED)
        {
            LOG_DEBUG("Updating hash for client: %p, packet type: %d, size: %d bytes", this, packetType, reader_sig.GetSize());
            crypto.UpdateHash(reader_sig.GetBuffer(), reader_sig.GetSize());
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
                crypto.UpdateHash(reader_sig.GetBuffer(), reader_sig.GetSize());
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
            channelBuffer.Append(reader_sig.GetBuffer(), reader_sig.GetSize());
        }
        TlsReader.AppendReaded(seg_size);
    }

    return Result<void, Error>::Ok();
}

/// @brief Packet processing - read data from the socket, parse TLS packets
/// @return Result indicating success or Tls_ProcessReceiveFailed error

Result<void, Error> TLSClient::ProcessReceive()
{
    LOG_DEBUG("Processing received data for client: %p, current state index: %d", this, stateIndex);
    recvBuffer.CheckSize(4096 * 4);
    auto readResult = context.Read((PUCHAR)(recvBuffer.GetBuffer() + recvBuffer.GetSize()), 4096 * 4);
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

    INT32 cur_index = 0;
    while (cur_index + 5 <= recvBuffer.GetSize())
    {
        INT32 packet_size = UINT16SwapByteOrder(*(PUINT16)(recvBuffer.GetBuffer() + cur_index + 3));
        if (cur_index + 5 + packet_size > recvBuffer.GetSize())
            break;

        LOG_DEBUG("Processing packet for client: %p, current index: %d, packet size: %d", this, cur_index, packet_size);

        TlsBuffer unnamed(recvBuffer.GetBuffer() + cur_index + 5, packet_size);

        auto ret = OnPacket(*(UINT8 *)(recvBuffer.GetBuffer() + cur_index), *(UINT16 *)(recvBuffer.GetBuffer() + cur_index + 1), unnamed);
        if (!ret)
        {
            LOG_DEBUG("Failed to process packet for client: %p, current index: %d, packet size: %d", this, cur_index, packet_size);
            (void)Close();
            return Result<void, Error>::Err(ret, Error::Tls_ProcessReceiveFailed);
        }
        LOG_DEBUG("Packet processed successfully for client: %p, current index: %d, packet size: %d", this, cur_index, packet_size);

        cur_index += 5 + packet_size;
    }
    Memory::Copy(recvBuffer.GetBuffer(), recvBuffer.GetBuffer() + cur_index, recvBuffer.GetSize() - cur_index);
    recvBuffer.AppendSize(-cur_index);
    return Result<void, Error>::Ok();
}

/// @brief Read data from channel buffer
/// @param out Pointer to the output buffer where the read data will be stored
/// @param size Size of the output buffer and the maximum number of bytes to read from the channel buffer
/// @return The number of bytes read from the channel buffer

INT32 TLSClient::ReadChannel(PCHAR out, INT32 size)
{
    INT32 movesize = Math::Min(size, channelBuffer.GetSize() - channelBytesRead);
    LOG_DEBUG("Reading from channel for client: %p, requested size: %d, available size: %d, readed size: %d",
              this, size, channelBuffer.GetSize() - channelBytesRead, channelBytesRead);
    Memory::Copy(out, channelBuffer.GetBuffer() + channelBytesRead, movesize);
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
    }
    LOG_DEBUG("Returning movesize: %d for client: %p", movesize, this);
    return movesize;
}

/// @brief Open a TLS connection to the server, perform the TLS handshake by sending the ClientHello message and processing the server's responses
/// @return Result indicating whether the TLS connection was opened and the handshake completed successfully

Result<void, Error> TLSClient::Open()
{
    LOG_DEBUG("Connecting to host: %s for client: %p, secure: %d", host, this, secure);

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

Result<void, Error> TLSClient::Close()
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

Result<UINT32, Error> TLSClient::Write(PCVOID buffer, UINT32 bufferLength)
{
    LOG_DEBUG("Sending data for client: %p, size: %d bytes", this, bufferLength);

    if (!secure)
    {
        auto writeResult = context.Write(buffer, bufferLength);
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
    for (UINT32 i = 0; i < bufferLength;)
    {
        INT32 send_size = Math::Min(bufferLength - i, 1024 * 16);
        sendBuffer.SetSize(send_size);
        Memory::Copy(sendBuffer.GetBuffer(), (PCHAR)buffer + i, send_size);
        auto sendResult = SendPacket(CONTENT_APPLICATION_DATA, 0x303, sendBuffer);
        if (!sendResult)
        {
            LOG_DEBUG("Failed to send packet for client: %p, size: %d bytes", this, send_size);
            return Result<UINT32, Error>::Err(sendResult, Error::Tls_WriteFailed_Send);
        }

        i += send_size;
    }

    LOG_DEBUG("Data sent successfully for client: %p, total size: %d bytes", this, bufferLength);
    return Result<UINT32, Error>::Ok(bufferLength);
}

/// @brief Read from the TLS channel, decrypting data if the handshake is complete and the encoding is enabled, and store it in the provided buffer
/// @param buffer Buffer where the read data will be stored
/// @param bufferLength Length of the buffer in bytes, indicating the maximum number of bytes to read from the TLS channel
/// @return Result with the number of bytes read, or an error

Result<SSIZE, Error> TLSClient::Read(PVOID buffer, UINT32 bufferLength)
{
    if (!secure)
    {
        auto readResult = context.Read(buffer, bufferLength);
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
    LOG_DEBUG("Reading data for client: %p, requested size: %d", this, bufferLength);
    while (channelBuffer.GetSize() <= channelBytesRead)
    {
        auto recvResult = ProcessReceive();
        if (!recvResult)
        {
            LOG_DEBUG("recv error, maybe close socket");
            return Result<SSIZE, Error>::Err(recvResult, Error::Tls_ReadFailed_Receive);
        }
    }

    return Result<SSIZE, Error>::Ok(ReadChannel((PCHAR)buffer, bufferLength));
}

/// @brief Constructor for the TLSClient class
/// @param host The hostname of the server to connect to
/// @param ipAddress The IP address of the server to connect to
/// @param port The port number of the server to connect to

TLSClient::TLSClient(PCCHAR host, const IPAddress &ipAddress, UINT16 port, BOOL secure)
    : host(host), ip(ipAddress), context(ipAddress, port), secure(secure), stateIndex(0), channelBytesRead(0)
{
    LOG_DEBUG("Initializing tls_cipher structure for cipher: %p, secure: %d", &crypto, secure);
}
