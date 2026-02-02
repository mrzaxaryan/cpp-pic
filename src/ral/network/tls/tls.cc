#include "tls.h"
#include "memory.h"
#include "random.h"
#include "socket.h"
#include "string.h"
#include "pal.h"
#include "logger.h"
#include "math.h"

#define TLS_CHACHA20_POLY1305_SHA256 0x1303

// The following defines SSL 3.0 content types
#define CONTENT_CHANGECIPHERSPEC 0x14
#define CONTENT_ALERT 0x15
#define CONTENT_HANDSHAKE 0x16
#define CONTENT_APPLICATION_DATA 0x17

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

BOOL TLSClient::SendPacket(INT32 packetType, INT32 ver, TlsBuffer *buf)
{
    // PNETWORK pNetwork = GetNetwork();

    if (packetType == CONTENT_HANDSHAKE && buf->GetSize() > 0)
    {
        LOG_DEBUG("Sending handshake packet with type: %d, version: %d, size: %d bytes", packetType, ver, buf->GetSize());
        crypto.UpdateHash(buf->GetBuffer(), buf->GetSize());
    }
    LOG_DEBUG("Sending packet with type: %d, version: %d, size: %d bytes", packetType, ver, buf->GetSize());

    TlsBuffer tempBuffer;
    Memory::Zero(&tempBuffer, sizeof(TlsBuffer));
    tempBuffer.Append((CHAR)packetType);
    tempBuffer.Append((INT16)ver);
    INT32 bodySizeIndex = tempBuffer.AppendSize(2); // tls body size

    BOOL keep_original = packetType == CONTENT_CHANGECIPHERSPEC || packetType == CONTENT_ALERT;
    if (!keep_original && crypto.GetEncoding())
    {
        LOG_DEBUG("Encoding packet with type: %d, size: %d bytes", packetType, buf->GetSize());
        buf->Append((CHAR)packetType);
        (tempBuffer.GetBuffer())[0] = CONTENT_APPLICATION_DATA;
    }
    LOG_DEBUG("Encoding buffer with size: %d bytes, keep_original: %d", buf->GetSize(), keep_original);
    crypto.Encode(&tempBuffer, buf->GetBuffer(), buf->GetSize(), keep_original); //-----------¼ÓÃÜ´úÂë

    *(UINT16 *)(tempBuffer.GetBuffer() + bodySizeIndex) = UINT16SwapByteOrder(tempBuffer.GetSize() - bodySizeIndex - 2);
    UINT32 bytesWritten = 0;
    if ((bytesWritten = context.Write((PCHAR)tempBuffer.GetBuffer(), tempBuffer.GetSize())) <= 0)
    {
        LOG_DEBUG("Failed to write packet to socket, bytesWritten: %d", bytesWritten);
        tempBuffer.Clear();
        return FALSE;
    }
    LOG_DEBUG("Packet sent successfully, bytesWritten: %d", bytesWritten);
    tempBuffer.Clear();
    return TRUE;
}

BOOL TLSClient::SendClientHello(const CHAR *host)
{

    LOG_DEBUG("Sending ClientHello for client: %p, host: %s", this, host);
    // tls_version version = tls13;

    sendBuffer.Clear();

    BOOL hastls13 = FALSE;

    sendBuffer.Append((CHAR)MSG_CLIENT_HELLO);
    INT32 handshakeSizeIndex = sendBuffer.AppendSize(3); // tls handshake body size
    LOG_DEBUG("Appending ClientHello with handshake size index: %d", handshakeSizeIndex);

    sendBuffer.Append((INT16)0x0303);
    LOG_DEBUG("Appending ClientHello with version: 0x0303");
    sendBuffer.Append(crypto.CreateClientRand(), RAND_SIZE);
    LOG_DEBUG("Appending ClientHello with client random data");
    sendBuffer.Append((CHAR)0);
    LOG_DEBUG("Client has %d ciphers to append", crypto.GetCipherCount());
    INT32 cipherCountIndex = sendBuffer.AppendSize(2);
    LOG_DEBUG("Appending ClientHello with cipher count index: %d", cipherCountIndex);
    for (INT32 i = 0; i < crypto.GetCipherCount(); i++)
    {
        sendBuffer.Append((INT16)UINT16SwapByteOrder((UINT16)TLS_CHACHA20_POLY1305_SHA256));
        hastls13 = TRUE;
    }
    LOG_DEBUG("Appending ClientHello with %d ciphers", crypto.GetCipherCount());
    *(PUINT16)(sendBuffer.GetBuffer() + cipherCountIndex) = UINT16SwapByteOrder(sendBuffer.GetSize() - cipherCountIndex - 2);
    sendBuffer.Append((CHAR)1);
    sendBuffer.Append((CHAR)0);

    INT32 extSizeIndex = sendBuffer.AppendSize(2);
    LOG_DEBUG("Appending ClientHello with extension size index: %d", extSizeIndex);
    sendBuffer.Append((INT16)UINT16SwapByteOrder(EXT_SERVER_NAME));
    INT32 hostLen = (INT32)String::Length((PCHAR)host);
    LOG_DEBUG("Appending ClientHello with host: %s, length: %d", host, hostLen);
    sendBuffer.Append((INT16)UINT16SwapByteOrder(hostLen + 5));
    sendBuffer.Append((INT16)UINT16SwapByteOrder(hostLen + 3));
    sendBuffer.Append((CHAR)0);
    sendBuffer.Append((INT16)UINT16SwapByteOrder(hostLen));
    sendBuffer.Append(host, hostLen);

    sendBuffer.Append((INT16)UINT16SwapByteOrder(EXT_SUPPORTED_GROUPS)); // ext type ÍÖÔ²ÇúÏß
    sendBuffer.Append((INT16)UINT16SwapByteOrder(ECC_COUNT * 2 + 2));    // ext size
    sendBuffer.Append((INT16)UINT16SwapByteOrder(ECC_COUNT * 2));
    LOG_DEBUG("Appending ClientHello with supported groups, count: %d", ECC_COUNT);
    sendBuffer.Append((INT16)UINT16SwapByteOrder((UINT16)ECC_secp256r1));
    sendBuffer.Append((INT16)UINT16SwapByteOrder((UINT16)ECC_secp384r1));

    if (hastls13)
    {
        LOG_DEBUG("Appending ClientHello with TLS 1.3 specific extensions");
        sendBuffer.Append((INT16)UINT16SwapByteOrder(EXT_SUPPORTED_VERSION));
        sendBuffer.Append((INT16)UINT16SwapByteOrder(3));
        sendBuffer.Append((CHAR)2);
        // tls 1.3 version
        sendBuffer.Append((INT16)UINT16SwapByteOrder(0x0304));

        sendBuffer.Append((INT16)UINT16SwapByteOrder(EXT_SIGNATURE_ALGORITHMS)); //
        sendBuffer.Append((INT16)UINT16SwapByteOrder(24));                       //
        sendBuffer.Append((INT16)UINT16SwapByteOrder(22));                       //
        sendBuffer.Append((INT16)UINT16SwapByteOrder(0x0403));                   //
        sendBuffer.Append((INT16)UINT16SwapByteOrder(0x0503));                   //
        sendBuffer.Append((INT16)UINT16SwapByteOrder(0x0603));                   //
        sendBuffer.Append((INT16)UINT16SwapByteOrder(0x0804));                   //
        sendBuffer.Append((INT16)UINT16SwapByteOrder(0x0805));                   //
        sendBuffer.Append((INT16)UINT16SwapByteOrder(0x0806));                   //
        sendBuffer.Append((INT16)UINT16SwapByteOrder(0x0401));                   //
        sendBuffer.Append((INT16)UINT16SwapByteOrder(0x0501));                   //
        sendBuffer.Append((INT16)UINT16SwapByteOrder(0x0601));                   //
        sendBuffer.Append((INT16)UINT16SwapByteOrder(0x0203));                   //
        sendBuffer.Append((INT16)UINT16SwapByteOrder(0x0201));                   //

        sendBuffer.Append((INT16)UINT16SwapByteOrder(EXT_KEY_SHARE)); // ext type ÍÖÔ²ÇúÏß
        INT32 shareSize = sendBuffer.AppendSize(2);
        sendBuffer.AppendSize(2);
        UINT16 ecc_iana_list[2]{};
        ecc_iana_list[0] = ECC_secp256r1;
        ecc_iana_list[1] = ECC_secp384r1;

        for (INT32 i = 0; i < ECC_COUNT; i++)
        {
            UINT16 eccIana = ecc_iana_list[i];
            sendBuffer.Append((INT16)UINT16SwapByteOrder(eccIana));
            INT32 shareSizeSub = sendBuffer.AppendSize(2);
            if (!crypto.ComputePublicKey(i, &sendBuffer))
            {
                LOG_DEBUG("Failed to compute public key for ECC group %d", i);
                return FALSE;
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

    return SendPacket(CONTENT_HANDSHAKE, 0x303, &sendBuffer);
}

BOOL TLSClient::SendClientFinished()
{
    TlsBuffer verify;
    Memory::Zero(&verify, sizeof(TlsBuffer));
    sendBuffer.Clear();
    LOG_DEBUG("Sending Client Finished for client: %p", this);
    crypto.ComputeVerify(&verify, CIPHER_HASH_SIZE, 0);
    LOG_DEBUG("Computed verify data for Client Finished, size: %d bytes", verify.GetSize());
    sendBuffer.Clear();
    sendBuffer.Append((CHAR)MSG_FINISHED);
    sendBuffer.Append((CHAR)0);
    sendBuffer.Append((INT16)UINT16SwapByteOrder(verify.GetSize()));
    sendBuffer.Append(verify.GetBuffer(), verify.GetSize());

    verify.Clear();

    return SendPacket(CONTENT_HANDSHAKE, 0x303, &sendBuffer);
}

BOOL TLSClient::SendClientExchange()
{
    sendBuffer.Clear();
    TlsBuffer *pubkey = crypto.GetPubKey();
    LOG_DEBUG("Sending Client Key Exchange for client: %p, public key size: %d bytes", this, pubkey->GetSize());
    sendBuffer.Append((CHAR)MSG_CLIENT_KEY_EXCHANGE);
    sendBuffer.Append((CHAR)0);
    sendBuffer.Append((INT16)UINT16SwapByteOrder(pubkey->GetSize() + 1));
    sendBuffer.Append(((CHAR)(pubkey->GetSize()))); // tls body size
    sendBuffer.Append(pubkey->GetBuffer(), pubkey->GetSize());
    return SendPacket(CONTENT_HANDSHAKE, 0x303, &sendBuffer);
}

BOOL TLSClient::SendChangeCipherSpec()
{
    sendBuffer.Clear();
    sendBuffer.Append((CHAR)1);
    return SendPacket(CONTENT_CHANGECIPHERSPEC, 0x303, &sendBuffer);
}

BOOL TLSClient::OnServerHello(TlsBuffer *reader)
{
    CHAR server_rand[RAND_SIZE];

    // generated by chatgpt
    // may be problematic
    LOG_DEBUG("Processing ServerHello for client: %p", this);
    UINT32SwapByteOrder(
        (((UINT32)reader->Read<INT8>()) << 8) |
        (((UINT32)reader->Read<INT16>()) << 16));
    // LOG_DEBUG("ServerHello size: %d bytes", server_hello_size);
    UINT16SwapByteOrder(reader->Read<INT16>());
    // LOG_DEBUG("ServerHello version: %d", ver);
    reader->Read(server_rand, sizeof(server_rand));
    INT32 session_len = reader->Read<INT8>();
    LOG_DEBUG("ServerHello session length: %d", session_len);
    reader->AppendReaded(session_len);
    reader->Read<INT16>(); // cur_cipher
    reader->Read<INT8>();
    BOOL ret = crypto.UpdateServerInfo();
    if (!ret)
    {
        LOG_DEBUG("Failed to update server info for client: %p", this);
        return FALSE;
    }
    // LOG_DEBUG("Updated server info for client: %p, cipher: %d, compress: %d", client, cur_cipher, compress);

    if (reader->GetReaded() >= reader->GetSize())
    {
        LOG_DEBUG("ServerHello reader has reached the end of buffer, no extensions found");
        return TRUE;
    }
    LOG_DEBUG("ServerHello has extensions, processing them");

    INT32 ext_size = UINT16SwapByteOrder(reader->Read<INT16>());
    INT32 ext_start = reader->GetReaded();
    INT32 tls_ver = 0;
    LOG_DEBUG("ServerHello extensions size: %d bytes, start index: %d", ext_size, ext_start);
    TlsBuffer pubkey;
    Memory::Zero(&pubkey, sizeof(TlsBuffer));
    ECC_GROUP eccgroup = ECC_NONE;
    while (reader->GetReaded() < ext_start + ext_size)
    {
        SSL_EXTENTION type = (SSL_EXTENTION)UINT16SwapByteOrder(reader->Read<INT16>());
        if (type == EXT_SUPPORTED_VERSION)
        {
            LOG_DEBUG("Processing EXT_SUPPORTED_VERSION extension");
            reader->Read<INT16>();
            tls_ver = UINT16SwapByteOrder(reader->Read<INT16>());
        }
        else if (type == EXT_KEY_SHARE)
        {
            LOG_DEBUG("Processing EXT_KEY_SHARE extension");
            INT32 size = UINT16SwapByteOrder(reader->Read<INT16>());
            eccgroup = (ECC_GROUP)UINT16SwapByteOrder(reader->Read<INT16>());
            if (size > 4)
            {
                LOG_DEBUG("Reading public key from EXT_KEY_SHARE, size: %d bytes", size);
                pubkey.SetSize(UINT16SwapByteOrder(reader->Read<INT16>()));
                reader->Read(pubkey.GetBuffer(), pubkey.GetSize());
            }
            LOG_DEBUG("EXT_KEY_SHARE processed, ECC group: %d, public key size: %d bytes", eccgroup, pubkey.GetSize());
        }
    }
    if (tls_ver != 0)
    {
        LOG_DEBUG("TLS version from ServerHello: %d", tls_ver);
        if (tls_ver != 0x0304 || pubkey.GetSize() <= 0 || eccgroup == ECC_NONE)
        {
            LOG_DEBUG("Invalid TLS version or public key size, tls_ver: %d, pubkey.size: %d, eccgroup: %d", tls_ver, pubkey.GetSize(), eccgroup);
            pubkey.Clear();
            return FALSE;
        }

        LOG_DEBUG("Valid TLS version and public key size, tls_ver: %d, pubkey.size: %d, eccgroup: %d", tls_ver, pubkey.GetSize(), eccgroup);

        if (!crypto.ComputeKey(eccgroup, pubkey.GetBuffer(), pubkey.GetSize(), 0))
        {
            LOG_DEBUG("Failed to compute TLS 1.3 key for client: %p, ECC group: %d, public key size: %d", this, eccgroup, pubkey.GetSize());
            pubkey.Clear();
            return FALSE;
        }
        LOG_DEBUG("Computed TLS 1.3 key for client: %p, ECC group: %d, public key size: %d", this, eccgroup, pubkey.GetSize());
        crypto.SetEncoding(TRUE);
    }
    LOG_DEBUG("ServerHello processed successfully for client: %p, ECC group: %d, public key size: %d", this, eccgroup, pubkey.GetSize());
    pubkey.Clear();
    return TRUE;
}

BOOL TLSClient::OnServerHelloDone()
{
    if (!SendClientExchange())
    {
        LOG_DEBUG("Failed to send Client Key Exchange for client: %p", this);
        return FALSE;
    }
    LOG_DEBUG("Client Key Exchange sent successfully for client: %p", this);
    if (!SendChangeCipherSpec())
    {
        LOG_DEBUG("Failed to send Change Cipher Spec for client: %p", this);
        return FALSE;
    }
    LOG_DEBUG("Change Cipher Spec sent successfully for client: %p", this);
    crypto.SetEncoding(TRUE);
    if (!SendClientFinished())
    {
        LOG_DEBUG("Failed to send Client Finished for client: %p", this);
        return FALSE;
    }
    LOG_DEBUG("Client Finished sent successfully for client: %p", this);

    return TRUE;
}

BOOL TLSClient::VerifyFinished(TlsBuffer *reader)
{
    INT32 server_finished_size = UINT32SwapByteOrder(reader->Read<INT8>() << 8 | reader->Read<INT16>() << 16);
    LOG_DEBUG("Verifying Finished for client: %p, size: %d bytes", this, server_finished_size);
    TlsBuffer verify;
    Memory::Zero(&verify, sizeof(TlsBuffer));
    crypto.ComputeVerify(&verify, server_finished_size, 1);
    LOG_DEBUG("Computed verify data for Finished, size: %d bytes", verify.GetSize());

    if (Memory::Compare(verify.GetBuffer(), reader->GetBuffer() + reader->GetReaded(), server_finished_size) != 0)
    {
        LOG_DEBUG("Finished verification failed for client: %p, expected size: %d, actual size: %d", this, verify.GetSize(), server_finished_size);
        verify.Clear();
        return FALSE;
    }
    LOG_DEBUG("Finished verification succeeded for client: %p", this);
    verify.Clear();
    return TRUE;
}

BOOL TLSClient::OnServerFinished()
{
    LOG_DEBUG("Processing Server Finished for client: %p", this);
    CHAR finished_hash[MAX_HASH_LEN] = {0};
    crypto.GetHash(finished_hash);
    BOOL ret = SendChangeCipherSpec();

    if (!ret)
    {
        LOG_DEBUG("Failed to send Change Cipher Spec for client: %p", this);
        return FALSE;
    }
    LOG_DEBUG("Change Cipher Spec sent successfully for client: %p", this);
    if (!SendClientFinished())
    {
        LOG_DEBUG("Failed to send Client Finished for client: %p", this);
        return FALSE;
    }
    LOG_DEBUG("Client Finished sent successfully for client: %p", this);
    crypto.ResetSequenceNumber();
    if (!crypto.ComputeKey(ECC_NONE, 0, 0, finished_hash))
    {
        LOG_DEBUG("Failed to compute TLS 1.3 key for client: %p", this);
        return FALSE;
    }

    LOG_DEBUG("Server Finished processed successfully for client: %p", this);
    return TRUE;
}

BOOL TLSClient::OnPacket(INT32 packetType, INT32 version, TlsBuffer *TlsReader)
{
    BOOL ret = 0;
    if (packetType != CONTENT_CHANGECIPHERSPEC && packetType != CONTENT_ALERT)
    {
        LOG_DEBUG("Processing packet with type: %d, version: %d, size: %d bytes", packetType, version, TlsReader->GetSize());
        if (!crypto.Decode(TlsReader, version))
        {
            LOG_DEBUG("Failed to Decode packet for client: %p, type: %d, version: %d", this, packetType, version);
            return FALSE;
        }
        LOG_DEBUG("Packet decoded successfully for client: %p, type: %d, version: %d", this, packetType, version);
        if (crypto.GetEncoding() && TlsReader->GetSize() > 0)
        {
            LOG_DEBUG("Removing last byte from buffer for client: %p, packet type: %d", this, packetType);
            packetType = TlsReader->GetBuffer()[TlsReader->GetSize() - 1];
            TlsReader->SetSize(TlsReader->GetSize() - 1);
        }
        LOG_DEBUG("Packet type after processing: %d, buffer size: %d bytes", packetType, TlsReader->GetSize());
    }

    ret = TRUE;

    while (TlsReader->GetReaded() < TlsReader->GetSize())
    {
        INT32 seg_size = packetType == CONTENT_HANDSHAKE ? 1 + 3 + UINT32SwapByteOrder(TlsReader->GetBuffer()[TlsReader->GetReaded() + 1] << 8 | *(PUINT16)(TlsReader->GetBuffer() + TlsReader->GetReaded() + 2) << 16) : TlsReader->GetSize();
        TlsBuffer reader_sig(TlsReader->GetBuffer() + TlsReader->GetReaded(), seg_size);

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
                return FALSE;
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
                ret = OnServerHello(&reader_sig);
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
                if (!OnServerHelloDone())
                {
                    LOG_DEBUG("Failed to process Server Hello Done for client: %p", this);
                    return FALSE;
                }
            }
            else if (handshakeType == MSG_FINISHED)
            {
                LOG_DEBUG("Processing Server Finished for client: %p", this);
                if (!VerifyFinished(&reader_sig))
                {
                    LOG_DEBUG("Failed to verify Finished for client: %p", this);
                    return FALSE;
                }
                LOG_DEBUG("Server Finished verified successfully for client: %p", this);
                crypto.UpdateHash(reader_sig.GetBuffer(), reader_sig.GetSize());
                if (!(OnServerFinished()))
                {
                    LOG_DEBUG("Failed to process Server Finished for client: %p", this);
                    return FALSE;
                }
                LOG_DEBUG("Server Finished processed successfully for client: %p", this);
            }
            if (!ret)
            {
                LOG_DEBUG("Failed to process handshake packet for client: %p, handshake type: %d", this, handshakeType);
                Close();
                return FALSE;
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
                return FALSE;
            }
            LOG_DEBUG("TLS Alert received for client: %p, but buffer size is less than 2 bytes", this);
        }
        else if (packetType == CONTENT_APPLICATION_DATA)
        {
            LOG_DEBUG("Processing Application Data for client: %p, size: %d bytes", this, reader_sig.GetSize());
            channelBuffer.Append(TlsReader->GetBuffer(), reader_sig.GetSize());
        }
        TlsReader->AppendReaded(seg_size);
    }

    return TRUE;
}

BOOL TLSClient::ProcessReceive()
{
    LOG_DEBUG("Processing received data for client: %p, current state index: %d", this, stateIndex);
    recvBuffer.CheckSize(recvBuffer.GetSize() + 4096 * 4);
    INT64 len = context.Read((PUCHAR)(recvBuffer.GetBuffer() + recvBuffer.GetSize()), 4096 * 4);
    LOG_DEBUG("Read %lld bytes from socket for client: %p", len, this);
    if (len <= 0)
    {
        LOG_DEBUG("Failed to read data from socket for client: %p, bytes read: %lld", this, len);
        Close();
        return FALSE;
    }
    LOG_DEBUG("Received %lld bytes from socket for client: %p", len, this);
    recvBuffer.AppendSize(len);

    INT32 cur_index = 0;
    while (cur_index + 5 <= recvBuffer.GetSize())
    {
        INT32 packet_size = UINT16SwapByteOrder(*(PUINT16)(recvBuffer.GetBuffer() + cur_index + 3));
        if (cur_index + 5 + packet_size > recvBuffer.GetSize())
            break;

        LOG_DEBUG("Processing packet for client: %p, current index: %d, packet size: %d", this, cur_index, packet_size);

        TlsBuffer unnamed(recvBuffer.GetBuffer() + cur_index + 5, packet_size);

        BOOL ret = OnPacket(*(UINT8 *)(recvBuffer.GetBuffer() + cur_index), *(UINT16 *)(recvBuffer.GetBuffer() + cur_index + 1), &unnamed);
        if (!ret)
        {
            LOG_DEBUG("Failed to process packet for client: %p, current index: %d, packet size: %d", this, cur_index, packet_size);
            Close();
            return FALSE;
        }
        LOG_DEBUG("Packet processed successfully for client: %p, current index: %d, packet size: %d", this, cur_index, packet_size);

        cur_index += 5 + packet_size;
    }
    Memory::Copy(recvBuffer.GetBuffer(), recvBuffer.GetBuffer() + cur_index, recvBuffer.GetSize() - cur_index);
    recvBuffer.AppendSize(-cur_index);
    return TRUE;
}

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

BOOL TLSClient::Open()
{
    LOG_DEBUG("Connecting to host: %s for client: %p", host, this);

    if (!context.Open())
    {
        LOG_DEBUG("Failed to connect to host: %s, for client: %p", host, this);
        return FALSE;
    }
    LOG_DEBUG("Connected to host: %s,  for client: %p", host,this);

    if (!(SendClientHello(host)))
    {
        LOG_DEBUG("Failed to send Client Hello for client: %p", this);
        return FALSE;
    }
    LOG_DEBUG("Client Hello sent successfully for client: %p", this);

    while (stateIndex < 6)
    {
        if (!(ProcessReceive()))
        {
            LOG_DEBUG("Failed to process received data for client: %p", this);
            return FALSE;
        }
    }

    return TRUE;
}
BOOL TLSClient::Close()
{
    // PNETWORK pNetwork = GetNetwork();

    stateIndex = 0;
    recvBuffer.Clear();
    channelBuffer.Clear();
    sendBuffer.Clear();

    channelBytesRead = 0;
    crypto.Destroy();

    LOG_DEBUG("Closing socket for client: %p", this);
    return context.Close();

    return TRUE;
}

UINT32 TLSClient::Write(PCVOID buffer, UINT32 bufferLength)
{
    LOG_DEBUG("Sending data for client: %p, size: %d bytes", this, bufferLength);
    if (stateIndex < 6)
    {
        LOG_DEBUG("send error, state index is %d", stateIndex);
        return 0;
    }

    sendBuffer.Clear();
    for (UINT32 i = 0; i < bufferLength;)
    {
        INT32 send_size = Math::Min(bufferLength - i, 1024 * 16);
        sendBuffer.SetSize(send_size);
        Memory::Copy(sendBuffer.GetBuffer(), (PCHAR)buffer + i, send_size);
        if (!SendPacket(CONTENT_APPLICATION_DATA, 0x303, &sendBuffer))
        {
            LOG_DEBUG("Failed to send packet for client: %p, size: %d bytes", this, send_size);
            return 0;
        }

        i += send_size;
    }

    LOG_DEBUG("Data sent successfully for client: %p, total size: %d bytes", this, bufferLength);
    return bufferLength;
}
SSIZE TLSClient::Read(PVOID buffer, UINT32 bufferLength)
{
    if (stateIndex < 6)
    {
        LOG_DEBUG("recv error, state index is %d", stateIndex);
        return 0;
    }
    LOG_DEBUG("Reading data for client: %p, requested size: %d", this, bufferLength);
    while (channelBuffer.GetSize() <= channelBytesRead)
    {
        BOOL ret = ProcessReceive();
        if (!ret)
        {
            LOG_DEBUG("recv error, maybe close socket");
            return 0;
        }
    }

    return ReadChannel((PCHAR)buffer, bufferLength);
}
TLSClient::TLSClient(PCCHAR host, const IPAddress &ipAddress, UINT16 port)
    : host(host), ip(ipAddress), context(ipAddress, port)
{
    stateIndex = 0;
    channelBytesRead = 0;
    Memory::Zero(&recvBuffer, sizeof(TlsBuffer));
    Memory::Zero(&channelBuffer, sizeof(TlsBuffer));
    Memory::Zero(&sendBuffer, sizeof(TlsBuffer));

    LOG_DEBUG("Initializing tls_cipher structure for cipher: %p", &crypto);
}
